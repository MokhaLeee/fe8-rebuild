#!/usr/bin/env bash
# 按仓库根 .clang-format 格式化指定文件。
#
#   tools/style/format.sh src/rng.c src/time.c ...
#   tools/style/format.sh --check src/rng.c      # 只报告需要格式化的文件，不改
#
# 只接受显式列出的文件，不递归全库：M3 是分批任务，每批都要过字节门禁。
set -euo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
cd "$ROOT"

CHECK=0
if [ "${1:-}" = "--check" ]; then
	CHECK=1
	shift
fi

if [ $# -eq 0 ]; then
	echo "usage: $0 [--check] <file>..." >&2
	exit 2
fi

bad=0
for f in "$@"; do
	case "$f" in
	*.c | *.h) ;;
	*)
		echo "skip (not C/H): $f" >&2
		continue
		;;
	esac
	[ -f "$f" ] || { echo "missing: $f" >&2; exit 2; }
	if [ $CHECK -eq 1 ]; then
		if ! clang-format --style=file "$f" | diff -q - "$f" >/dev/null; then
			echo "needs format: $f"
			bad=1
		fi
	else
		clang-format --style=file -i "$f"
		echo "formatted: $f"
	fi
done

exit $bad
