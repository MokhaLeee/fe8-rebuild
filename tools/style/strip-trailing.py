#!/usr/bin/env python3
"""去掉行尾空白，但跳过字符串/注释内部的行。

行尾空白不会改变 token，去掉是安全的；唯一风险是多行字符串里的行尾空格
属于字符串内容，所以沿用 indent-tabs.py 的词法状态机做保护。

    tools/style/strip-trailing.py FILE...
    tools/style/strip-trailing.py --check FILE...
"""

import sys

TABSTOP = 8


def scan_state(line, in_block, in_str, in_chr):
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
        i += 1
    return in_block, in_str, in_chr


def convert_text(text):
    out = []
    in_block = in_str = in_chr = False
    for line in text.splitlines(keepends=True):
        # 只保护字符串内部；块注释里的行尾空格可以去掉
        protected = in_str or in_chr
        if not protected:
            body = line.rstrip("\n")
            newline = line[len(body):]
            out.append(body.rstrip() + newline)
        else:
            out.append(line)
        in_block, in_str, in_chr = scan_state(line, in_block, in_str, in_chr)
    return "".join(out)


def main(argv):
    check = "--check" in argv
    files = [a for a in argv[1:] if not a.startswith("--")]
    if not files:
        print("usage: %s [--check] FILE..." % argv[0], file=sys.stderr)
        return 2
    changed = 0
    for p in files:
        src = open(p, encoding="utf-8").read()
        out = convert_text(src)
        if out == src:
            continue
        changed += 1
        if check:
            print("trailing ws: %s" % p)
        else:
            open(p, "w", encoding="utf-8").write(out)
            print("stripped: %s" % p)
    return 1 if (check and changed) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
