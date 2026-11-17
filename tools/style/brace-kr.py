#!/usr/bin/env python3
"""把 Allman 大括号改成 K&R（只是移动 `{` / `}` 字符的位置）。

和 clang-format 的区别：本脚本不重排缩进、不折行、不增删 token，
只做三件事：

1. 控制语句的 `{` 并入上一行：`if (x)` + `{`  ->  `if (x) {`
2. `}` 与 `else` 合并：`}` + `else` + `{`  ->  `} else {`
3. do-while：`}` + `while (x);`  ->  `} while (x);`

**函数定义不并**（左大括号独占一行），靠**花括号深度**判断：
深度 0 处的 `{` 是函数体，深度 >= 1 处的是控制块。
多行条件的 `{`（上一行是 `b)`）也能正确处理，因为深度已经 >= 1。

字符串和注释里的花括号不计入深度（有词法状态机）。

    tools/style/brace-kr.py FILE...
    tools/style/brace-kr.py --diff FILE...   # 只打印将要做的改动
"""

import re
import sys

CONTROL = re.compile(r"^(if|for|while|switch|else|do)\b")


def scan(line, in_block, in_str, in_chr):
    """返回 (行尾状态, 本行净花括号增量)。"""
    depth = 0
    i, n = 0, len(line)
    while i < n:
        ch = line[i]
        if in_block:
            if ch == "*" and i + 1 < n and line[i + 1] == "/":
                in_block = False
                i += 2
                continue
            i += 1
            continue
        if in_str or in_chr:
            if ch == "\\":
                i += 2
                continue
            if in_str and ch == '"':
                in_str = False
            elif in_chr and ch == "'":
                in_chr = False
            i += 1
            continue
        if ch == "/" and i + 1 < n:
            if line[i + 1] == "/":
                break
            if line[i + 1] == "*":
                in_block = True
                i += 2
                continue
        if ch == '"':
            in_str = True
        elif ch == "'":
            in_chr = True
        elif ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        i += 1
    return (in_block, in_str, in_chr), depth


def indent_of(line):
    return line[: len(line) - len(line.lstrip())]


BRACE_IF = re.compile(r"^(\t*)(if|for|while) \((.*)\) \{$")
ELSE_TAIL = re.compile(r"^(\t*)\} else \{$")


def _single_stmt(lines, start, end):
    """lines[start:end] 是否为「恰好一条简单语句」。"""
    body = [l for l in lines[start:end] if l.strip()]
    if len(body) != 1:
        return None
    s = body[0].strip()
    if not s.endswith(";"):
        return None
    if "{" in s or "}" in s:
        return None
    if re.match(r"^(if|for|while|switch|do|else)\b", s):
        return None
    return body[0]


def strip_single_braces(lines):
    """单语句分支去掉多余大括号（kernel style）。

    处理两种形态：
      if (c) {           ->  if (c)
          stmt;                 stmt;
      }
      if (c) { stmt; } else { stmt; }   ->  同样去掉两侧括号与 else 括号
    """
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        m = BRACE_IF.match(line.rstrip("\n"))

        # 形态一：`if (c) {` + 单语句 + `}`（无 else）
        if m:
            ind, kw, cond = m.groups()
            j = i + 1
            while j < n and not lines[j].strip().startswith("}"):
                j += 1
            if j < n and lines[j].rstrip("\n") == ind + "}":
                body = _single_stmt(lines, i + 1, j)
                if body is not None:
                    out.append("%s%s (%s)\n" % (ind, kw, cond))
                    out.append(body)
                    i = j + 1
                    continue
            # 形态二：`if (c) {` + 单语句 + `} else {` + 单语句 + `}`
            # 保留两侧大括号：agbcc 的 -Wparentheses 会对没有大括号的
            # if/else 报 "suggest explicit braces to avoid ambiguous else"
            e = ELSE_TAIL.match(lines[j].rstrip("\n")) if j < n else None
            if e:
                out.append(line)
                i += 1
                continue

        out.append(line)
        i += 1
    return out


def convert_text(text):
    lines = text.splitlines(keepends=True)
    out = []
    depth = 0
    in_block = in_str = in_chr = False
    # `#if` 每个分支可能各自开/闭大括号，朴素累加会重复计数。
    # 栈元素：(进入 #if 时的深度, #else 分支结束时的深度或 None)
    pp_stack = []

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        (in_block, in_str, in_chr), delta = scan(line, in_block, in_str, in_chr)

        # 预处理条件：单独记账，不参与合并
        if re.match(r"^#\s*(if|ifdef|ifndef)\b", stripped):
            pp_stack.append([depth, None])
            out.append(line)
            i += 1
            continue
        if re.match(r"^#\s*else\b", stripped):
            if pp_stack:
                pp_stack[-1][1] = depth
                depth = pp_stack[-1][0]
            out.append(line)
            i += 1
            continue
        if re.match(r"^#\s*endif\b", stripped):
            if pp_stack:
                base, else_depth = pp_stack.pop()
                depth = max(depth, else_depth) if else_depth is not None else depth
            out.append(line)
            i += 1
            continue

        # 0) 深度 0 处的函数定义，左大括号要独占一行：`void f(void) {` -> 两行
        if depth == 0 and stripped.endswith("{") and "=" not in stripped:
            body = stripped[:-1].rstrip()
            if body.endswith(")") and re.match(r"^[A-Za-z_][\w \t\*]*\(", body):
                ind = indent_of(line)
                out.append(ind + body + "\n")
                out.append(ind + "{\n")
                depth += delta
                i += 1
                continue

        # 1) 单独一行的 `{`
        if stripped == "{" and out:
            prev = out[-1]
            prev_stripped = prev.strip()
            # 预处理指令（#if / #else / #endif）后面不能并 `{`
            if prev_stripped.startswith("#"):
                out.append(line)
                depth += delta
                i += 1
                continue
            # 注释行（`// ...`）后面不能并 `{`，否则 `{` 会变成注释内容
            if prev_stripped.startswith("//") or prev_stripped.startswith("/*") or prev_stripped.endswith("*/"):
                out.append(line)
                depth += delta
                i += 1
                continue
            # do { ... } while 的 do 也是深度 >=1；函数体在深度 0
            if depth >= 1 and prev_stripped and not prev_stripped.endswith(";"):
                out[-1] = prev.rstrip("\n") + " {\n"
                depth += delta
                i += 1
                continue
            # 深度 0：函数体或结构体/数组初始化，保持独占一行
            out.append(line)
            depth += delta
            i += 1
            continue

        # 2) `}` 后接 `else` / `while`
        if stripped == "}" and i + 1 < len(lines):
            nxt = lines[i + 1].strip()
            if re.match(r"^else\b", nxt) or re.match(r"^while\s*\(", nxt):
                # 把 `} else {` 或 `} while (...) ;` 合成一行
                merged = line.rstrip("\n")
                if nxt.endswith("{"):
                    # `else` / `else if (...)` 左括号在下一行的情况
                    if i + 2 < len(lines) and lines[i + 2].strip() == "{":
                        merged = merged + " " + nxt + " {\n"
                        i += 3
                        # 合并行 `} else {` 净变化为 0：delta(-1) + 开括号(+1)
                        depth += delta + 1
                        out.append(merged)
                        continue
                    # `} else {`：delta(-1) + 开括号(+1) = 0
                    merged = merged + " " + nxt + "\n"
                    i += 2
                    depth += delta + 1
                    out.append(merged)
                    continue
                # `} while (...);`：只有闭括号
                merged = merged + " " + nxt + "\n"
                i += 2
                depth += delta
                out.append(merged)
                continue

        out.append(line)
        depth += delta
        i += 1

    return "".join(strip_single_braces(out))


def main(argv):
    show_diff = False
    files = []
    for a in argv[1:]:
        if a == "--diff":
            show_diff = True
        else:
            files.append(a)
    if not files:
        print("usage: %s [--diff] FILE..." % argv[0], file=sys.stderr)
        return 2

    for path in files:
        with open(path, encoding="utf-8") as f:
            src = f.read()
        out = convert_text(src)
        if out == src:
            continue
        if show_diff:
            import difflib
            sys.stdout.writelines(
                difflib.unified_diff(
                    src.splitlines(keepends=True),
                    out.splitlines(keepends=True),
                    fromfile=path, tofile=path + " (kr)", n=1,
                )
            )
        else:
            with open(path, "w", encoding="utf-8") as f:
                f.write(out)
            print("braced: %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
