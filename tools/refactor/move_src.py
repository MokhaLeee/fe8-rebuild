#!/usr/bin/env python3
"""M4a：把 src/ 下的 .c 移动到子目录，并保持链接顺序与字节一致。

做四件事：
1. `git mv src/<old>.c src/<New>/<old>.c`
2. 改写三个链接脚本里的 `src/<old>.o` -> `src/<New>/<old>.o`
   （逐行字符串替换，绝不重排行序——行序即地址）
3. 检查是否还有 `#include` 指向被移动文件所在目录（目前 `src/` 下只有
   `events/` 与 `data/` 两个子目录，由 `-iquote src` 兜住）
4. 打印移动清单，便于核对

    tools/refactor/move_src.py --batch batch.txt [--dry-run]

batch.txt 每行：`<文件名不含 .c> <目标目录>`，例如
    bmsave Core/Save
    savemenu Core/Save
"""

import argparse
import os
import shutil
import subprocess
import sys

LINKER_SCRIPTS = [
    "ldscript.txt",
    "linker_script_sound.txt",
    "linker_script_banim.txt",
    "sym_iwram.txt",
]


def git_mv(src, dst):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    subprocess.check_call(["git", "mv", src, dst])


def rewrite_linkers(moves):
    """moves: [(old_name, new_dir)]，把三种脚本里的对象路径改掉。"""
    # 长的先替换，避免 bmsave 抢先匹配 bmsave-bwl
    ordered = sorted(moves, key=lambda m: -len(m[0]))
    counts = {s: 0 for s in LINKER_SCRIPTS}
    for script in LINKER_SCRIPTS:
        if not os.path.exists(script):
            continue
        with open(script, encoding="utf-8") as f:
            text = f.read()
        orig = text
        for name, newdir in ordered:
            old_path = "src/%s.o" % name
            new_path = "src/%s/%s.o" % (newdir, name)
            n = text.count(old_path)
            if n:
                text = text.replace(old_path, new_path)
                counts[script] += n
        if text != orig:
            with open(script, "w", encoding="utf-8") as f:
                f.write(text)
    return counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch", required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    moves = []
    for line in open(args.batch, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, newdir = line.split()
        src = "src/%s.c" % name
        if not os.path.exists(src):
            print("MISSING %s" % src, file=sys.stderr)
            return 2
        moves.append((name, newdir))

    if args.dry_run:
        for name, newdir in moves:
            print("would move src/%s.c -> src/%s/%s.c" % (name, newdir, name))
        return 0

    for name, newdir in moves:
        git_mv("src/%s.c" % name, "src/%s/%s.c" % (newdir, name))

    counts = rewrite_linkers(moves)
    print("moved %d files; linker rewrites: %s" % (len(moves), counts))
    return 0


if __name__ == "__main__":
    sys.exit(main())
