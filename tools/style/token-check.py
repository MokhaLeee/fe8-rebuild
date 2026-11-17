#!/usr/bin/env python3
"""比较两个版本的 C 源码，确认只有空白/大括号位置变化。

和「剔除空白后比较」不同：本脚本先切 token 再比较，因此
`a >> b` 与 `a > > b`、`x++ + y` 与 `x+ ++y` 这类会被判为不同。
这正是 clang-format 曾把 `i < sp08 >> 0x13` 改成 `i<sp08> > 0x13` 的地方。

    tools/style/token-check.py src/foo.c src/bar.c     # 与 HEAD 比
    tools/style/token-check.py --rev ef9018f src/foo.c
"""

import re
import subprocess
import sys

# 多字符运算符要和单字符分开匹配，否则 >> 会被拆成两个 >
_MULTI = (
    "<<=", ">>=", "...",
    "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=",
    "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
    "##",
)

_TOKEN = re.compile(
    r"""
      (?P<ws>\s+)
    | (?P<comment>//[^\n]*|/\*[\s\S]*?\*/)
    | (?P<str>"(?:[^"\\]|\\.)*")
    | (?P<chr>'(?:[^'\\]|\\.)*')
    | (?P<ident>[A-Za-z_]\w*)
    | (?P<num>(?:0[xX][0-9A-Fa-f]+|\d+)[uUlL]*)
    | (?P<op>"""
    + "|".join(re.escape(o) for o in _MULTI)
    + r""")
    | (?P<other>.)
    """,
    re.VERBOSE,
)


def tokens(text: str, drop_braces: bool = False):
    out = []
    for m in _TOKEN.finditer(text):
        kind = m.lastgroup
        # 空白与注释不参与比较；注释在词法阶段整体吃掉，
        # 不能靠"读到换行为止"来剥离（换行本身是空白 token）。
        if kind in ("ws", "comment"):
            continue
        if drop_braces and m.group() in ("{", "}"):
            continue
        out.append((kind, m.group()))
    return out





def show_diff(a, b, path):
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            lo = max(0, i - 3)
            print("  %s: token %d differs" % (path, i))
            print("    old: %s" % " ".join(t[1] for t in a[lo:i + 4]))
            print("    new: %s" % " ".join(t[1] for t in b[lo:i + 4]))
            return
    if len(a) != len(b):
        longer, tag = (a, "old") if len(a) > len(b) else (b, "new")
        print("  %s: %s has %d extra tokens: %s"
              % (path, tag, len(longer) - n, " ".join(t[1] for t in longer[n:n + 8])))
        return
    print("  %s: token streams differ" % path)


def main(argv):
    rev = "HEAD"
    drop_braces = False
    files = []
    i = 1
    while i < len(argv):
        if argv[i] == "--rev":
            rev = argv[i + 1]
            i += 2
        elif argv[i] == "--drop-braces":
            # 允许「单语句去大括号」这类风格改动；真正的语义校验靠 make check
            drop_braces = True
            i += 1
        else:
            files.append(argv[i])
            i += 1

    if not files:
        print("usage: %s [--rev REV] [--drop-braces] FILE..." % argv[0], file=sys.stderr)
        return 2

    bad = 0
    for path in files:
        try:
            old = subprocess.check_output(
                ["git", "show", "%s:%s" % (rev, path)], text=True
            )
        except subprocess.CalledProcessError:
            print("skip (not in %s): %s" % (rev, path))
            continue
        with open(path, encoding="utf-8") as f:
            new = f.read()

        a, b = tokens(old, drop_braces), tokens(new, drop_braces)
        if a == b:
            print("OK    %s" % path)
        else:
            bad = 1
            show_diff(a, b, path)

    return bad


if __name__ == "__main__":
    sys.exit(main(sys.argv))
