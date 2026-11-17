#!/usr/bin/env python3
"""M4c 步骤 A：把 graphics/<dir> 搬到 contents/<dir>，并改写所有引用。

做四件事：
1. `git mv graphics/<dir> contents/<dir>`
2. 改写 data/ sound/ 下 .s 文件里的 `incbin "graphics/..."`
3. 改写规则文件与 Makefile 里的 `graphics/<dir>`
4. 报告改写点数量，便于核对

    tools/refactor/move_graphics.py --dry-run cg chap_title ...
    tools/refactor/move_graphics.py cg chap_title ...
"""

import argparse
import os
import subprocess
import sys

# 需要扫描并改写路径引用的文件
REF_GLOBS = [
    ("data", ".s"),
    ("sound", ".s"),
    (".", ".mk"),
    (".", "Makefile"),
]

TEXT_FILES = [
    "graphics_file_rules.mk",
    "songs.mk",
    "json_data_rules.mk",
    "Makefile",
    "graphics/banim/assets/img/banim_img_rules.mk",
]


def collect_referers():
    out = []
    for root in ("data", "sound"):
        for dirpath, _, files in os.walk(root):
            for f in files:
                if f.endswith(".s"):
                    out.append(os.path.join(dirpath, f))
    for f in TEXT_FILES:
        if os.path.exists(f):
            out.append(f)
    return out


def rewrite(paths, renames, dry):
    """renames: [(old_prefix, new_prefix)]，长前缀优先。"""
    ordered = sorted(renames, key=lambda r: -len(r[0]))
    total = 0
    for p in paths:
        try:
            text = open(p, encoding="utf-8").read()
        except UnicodeDecodeError:
            continue
        orig = text
        hits = 0
        for old, new in ordered:
            n = text.count(old)
            if n:
                text = text.replace(old, new)
                hits += n
        if text != orig:
            total += hits
            print("  %-42s %d 处" % (p, hits))
            if not dry:
                open(p, "w", encoding="utf-8").write(text)
    return total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dirs", nargs="+")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    renames = []
    for d in args.dirs:
        old = "graphics/%s" % d
        if not os.path.isdir(old):
            print("MISSING %s" % old, file=sys.stderr)
            return 2
        renames.append((old, "contents/%s" % d))

    if not args.dry_run:
        for d in args.dirs:
            dst = "contents/%s" % d
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            subprocess.check_call(["git", "mv", "graphics/%s" % d, dst])
            print("moved graphics/%s -> %s" % (d, dst))

    referers = collect_referers()
    print("扫描 %d 个引用文件，改写：" % len(referers))
    total = rewrite(referers, renames, args.dry_run)
    print("合计 %d 处%s" % (total, "（dry-run，未写盘）" if args.dry_run else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
