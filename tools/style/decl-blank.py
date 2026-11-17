#!/usr/bin/env python3
"""在局部变量声明块之后补一个空行（checkpatch 的
`Missing a blank line after declarations`）。

只处理最明显的形态：连续若干行「类型 + 名字 ;」声明，
紧跟第一行非声明语句时插入一个空行。保守规则：

- 行首类型必须是已知类型关键字，或以 `struct` / `enum` / `union` 开头
- 声明行必须以 `;` 结尾（跨行声明跳过，留给人工）
- 已经空行时不重复插入
- 只在函数体内部（花括号深度 >= 1）生效
- `#` 预处理行跳过

    tools/style/decl-blank.py FILE...
    tools/style/decl-blank.py --diff FILE...
"""

import re
import sys

TYPE_WORDS = {
    "u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64",
    "bool", "bool8", "int", "unsigned", "signed", "char", "short", "long",
    "float", "double", "void", "size_t", "uintptr_t", "intptr_t",
    "ProcPtr", "vu8", "vu16", "vu32", "vs8", "vs16", "vs32",
}

DECL = re.compile(
    r"^(?P<ind>[\t ]*)(?P<type>struct\s+\w+|enum\s+\w+|union\s+\w+|\w+)"
    r"(?P<rest>.*?)\s*;\s*$"
)


def is_decl(line):
    s = line.strip()
    if not s or s.startswith("#") or s.startswith("//") or s.startswith("/*"):
        return False
    if not s.endswith(";"):
        return False
    m = DECL.match(line.rstrip("\n"))
    if not m:
        return False
    if m.group("type") not in TYPE_WORDS and not re.match(
        r"^(struct|enum|union)\s", m.group("type")
    ):
        return False
    # `foo;` 这种单 token 不是声明
    if not m.group("rest").strip():
        return False
    return True


FIELD = re.compile(
    r"^(?P<ind>[\t ]*)"
    r"(?P<cpre>/\* [0-9A-Fa-fx]+ \*/ )?"
    r"(?P<quals>(?:const|volatile)\s+)*"
    r"(?P<type>struct\s+\w+|enum\s+\w+|union\s+\w+|\w+)"
    r"\s*"
    r"(?P<stars>\*+)"
    r"\s*"
    r"(?P<name>[A-Za-z_]\w*)"
    r"\s*(?P<arr>\[[^\]]*\])?"
    r"\s*;"
)


def fix_field_pointer(line):
    """结构体字段的指针声明：`const char* unk;` -> `const char *unk;`。"""
    raw = line.rstrip("\n")
    m = FIELD.match(raw)
    if not m:
        return line
    base = m.group("type").split()[-1]
    if base not in TYPE_WORDS and not re.match(r"^(struct|enum|union)\s", m.group("type")):
        return line
    rebuilt = "%s%s%s%s %s%s%s;" % (
        m.group("ind"),
        m.group("cpre") or "",
        m.group("quals") or "",
        m.group("type"),
        m.group("stars"),
        m.group("name"),
        m.group("arr") or "",
    )
    return rebuilt + raw[m.end():] + "\n"


def convert_text(text):
    lines = text.splitlines(keepends=True)
    out = []
    depth = 0
    in_block = in_str = in_chr = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        # 深度跟踪（粗粒度：忽略字符串/注释里的花括号）
        j, n = 0, len(line)
        add = 0
        while j < n:
            ch = line[j]
            if in_block:
                if ch == "*" and j + 1 < n and line[j + 1] == "/":
                    in_block = False
                    j += 2
                    continue
                j += 1
                continue
            if in_str or in_chr:
                if ch == "\\":
                    j += 2
                    continue
                if in_str and ch == '"':
                    in_str = False
                elif in_chr and ch == "'":
                    in_chr = False
                j += 1
                continue
            if ch == "/" and j + 1 < n:
                if line[j + 1] == "/":
                    break
                if line[j + 1] == "*":
                    in_block = True
                    j += 2
                    continue
            if ch == '"':
                in_str = True
            elif ch == "'":
                in_chr = True
            elif ch == "{":
                add += 1
            elif ch == "}":
                add -= 1
            j += 1

        prev_depth = depth
        depth += add

        # 结构体字段行：`/* 2C */ const char* unk;` -> `const char *unk;`
        line = fix_field_pointer(line)

        # 函数签名里的参数同样处理：`f(struct Trap* t)` -> `f(struct Trap *t)`
        sig = re.match(r"^([\w \t\*]+)\((.*)\)(\s*\{)?\s*$", line.rstrip("\n"))
        if sig and "*" in sig.group(2) and not re.match(r"^(if|for|while|switch|return)\b", sig.group(1).strip()):
            params = re.sub(r"([\w\]\)])\s*\*(?!\*)", r"\1 *", sig.group(2))
            params = re.sub(r"\*\s+([A-Za-z_])", r"*\1", params)
            line = "%s(%s)%s\n" % (sig.group(1), params, sig.group(3) or "")

        # 声明行里的指针星号：贴近变量名、与类型之间留一个空格
        # `u16 * pal` / `struct Trap* trap` -> `u16 *pal` / `struct Trap *trap`
        # 只处理 `=` 之前的部分，避免动到初始化表达式里的乘法
        if is_decl(line):
            head, sep, tail = line.partition("=")
            if "*" in head:
                head = re.sub(r"([\w\]\)])\s*\*(?!\*)", r"\1 *", head)
                head = re.sub(r"\*\s+([A-Za-z_])", r"*\1", head)
            if head != line.partition("=")[0]:
                line = head + sep + tail

        # 上一行是声明、本行不是声明也不是空行、同一缩进 -> 插入空行
        if (
            out
            and prev_depth >= 1
            and not stripped == ""
            and not is_decl(line)
            and not stripped.startswith(("#", "}", "{"))
            and is_decl(out[-1])
            and len(out[-1]) - len(out[-1].lstrip()) == len(line) - len(line.lstrip())
        ):
            out.append("\n")

        out.append(line)

    return "".join(out)


def main(argv):
    show = False
    files = []
    for a in argv[1:]:
        if a == "--diff":
            show = True
        else:
            files.append(a)
    if not files:
        print("usage: %s [--diff] FILE..." % argv[0], file=sys.stderr)
        return 2
    for path in files:
        src = open(path, encoding="utf-8").read()
        out = convert_text(src)
        if out == src:
            continue
        if show:
            import difflib
            sys.stdout.writelines(
                difflib.unified_diff(src.splitlines(True), out.splitlines(True),
                                     path, path + " (decl-blank)", n=1)
            )
        else:
            open(path, "w", encoding="utf-8").write(out)
            print("blanked: %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
