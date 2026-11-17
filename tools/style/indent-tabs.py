#!/usr/bin/env python3
"""只把行首缩进从空格转成 Tab，其余一字不动。

为什么存在：clang-format 在这份代码上会改坏语义（把 `i < x >> 2` 拆成模板参数、
把 GCC 语句表达式 `({...})` 加空格导致 agbcc 拒绝），所以不能整段自动重排。
本脚本只动「代码行的行首空白」，不重排大括号、不折行、不碰行内空格，
因此不可能改变 token —— 由 tools/style/token-check.py 复核。

大括号位置、声明后的空行仍由人工处理。

两个必须处理好的细节：

1. **源缩进单位**：本代码库 265 个文件用 4 空格缩进、1 个用 2 空格、其余已是 Tab。
   直接按 8 列换算会把「4 空格 = 1 层」算成「4 空格 = 半层」，导致层与层之间不一致。
   所以先把源缩进宽度除以「该文件的最小正缩进」，得到层数，再输出层数个 Tab。

2. **多行字符串**：`asm("\\n\\` 这类续行的行首空格属于字符串内容。
   用词法状态机跟踪 `"`、`'`、`//`、`/* */`，字符串/注释内部的行一个字符都不动。

    tools/style/indent-tabs.py FILE...
    tools/style/indent-tabs.py --check FILE...
    tools/style/indent-tabs.py --dry-run FILE...
    tools/style/indent-tabs.py --unit 2 FILE...     # 强制指定源缩进单位
"""

import re
import sys

TABSTOP = 8


def detect_unit(text: str):
    """推断源文件的缩进单位。

    不能用「最小正缩进」：有些文件存在对齐用的 1~3 个空格（续行对齐、
    注释对齐），会把单位误判成 1，结果每层输出多个 Tab。
    改用出现次数最多的缩进宽度，它通常就是第一层。
    """
    from collections import Counter

    indents = []
    in_block = False
    for line in text.splitlines():
        if not line.strip():
            continue
        stripped = line.lstrip(" \t")
        if in_block and stripped.startswith("*"):
            if "*/" in line:
                in_block = False
            continue
        if stripped.startswith("/*"):
            if "*/" not in line:
                in_block = True
            continue
        m = re.match(r"^[ ]+", line)
        if not m:
            continue
        indents.append(len(m.group()))
    if not indents:
        return None
    counts = Counter(indents)
    unit = counts.most_common(1)[0][0]
    if unit < 2:
        # 退化情况：最小公因数兜底
        g = indents[0]
        for n in indents[1:]:
            g = __import__("math").gcd(g, n)
        unit = g if g >= 1 else 4
    return unit


def scan_state(line, in_block_comment, in_string, in_char):
    i, n = 0, len(line)
    while i < n:
        ch = line[i]
        if in_block_comment:
            if ch == "*" and i + 1 < n and line[i + 1] == "/":
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_string or in_char:
            if ch == "\\":
                i += 2
                continue
            if in_string and ch == '"':
                in_string = False
            elif in_char and ch == "'":
                in_char = False
            i += 1
            continue
        if ch == "/" and i + 1 < n:
            if line[i + 1] == "/":
                break
            if line[i + 1] == "*":
                in_block_comment = True
                i += 2
                continue
        if ch == '"':
            in_string = True
        elif ch == "'":
            in_char = True
        i += 1
    return in_block_comment, in_string, in_char


def convert_line(line: str, unit: int) -> str:
    n = 0
    col = 0
    for ch in line:
        if ch == " ":
            col += 1
        elif ch == "\t":
            return line  # 已经是 Tab 缩进，不动
        else:
            break
        n += 1

    rest = line[n:]
    if not rest or not rest.strip():
        return line

    body = rest.rstrip("\n")
    newline = rest[len(body):]

    level, spaces = divmod(col, unit)
    if level == 0:
        return line
    # 余数：不足一层的部分保留为空格，尽量维持相对对齐
    return "\t" * level + " " * spaces + body + newline


def convert_text(text: str, unit=None):
    if unit is None:
        unit = detect_unit(text)
    if unit is None or unit <= 0:
        return text

    out = []
    in_block_comment = in_string = in_char = False
    for line in text.splitlines(keepends=True):
        if in_block_comment or in_string or in_char:
            out.append(line)
        else:
            out.append(convert_line(line, unit))
        in_block_comment, in_string, in_char = scan_state(
            line, in_block_comment, in_string, in_char
        )
    return "".join(out)


def main(argv):
    mode = "write"
    unit = None
    files = []
    i = 1
    while i < len(argv):
        a = argv[i]
        if a == "--check":
            mode = "check"
        elif a == "--dry-run":
            mode = "dry-run"
        elif a == "--unit":
            unit = int(argv[i + 1])
            i += 1
        else:
            files.append(a)
        i += 1

    if not files:
        print("usage: %s [--check|--dry-run] [--unit N] FILE..." % argv[0], file=sys.stderr)
        return 2

    changed = 0
    for path in files:
        with open(path, encoding="utf-8") as f:
            src = f.read()
        u = unit if unit is not None else detect_unit(src)
        out = convert_text(src, u)
        if out == src:
            continue
        changed += 1
        if mode == "check":
            print("needs indent: %s (unit=%s)" % (path, u))
        elif mode == "dry-run":
            print("would reformat: %s (unit=%s)" % (path, u))
        else:
            with open(path, "w", encoding="utf-8") as f:
                f.write(out)
            print("indented: %s (unit=%s)" % (path, u))

    if mode == "check" and changed:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
