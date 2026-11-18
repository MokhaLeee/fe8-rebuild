#!/usr/bin/env bash
# testmenu 测试 runner。
#
#   ./tools/emu/testmenu-tests.sh                # 跑全部 case
#   ./tools/emu/testmenu-tests.sh --list         # 列出 case
#   ./tools/emu/testmenu-tests.sh --case 2       # 只跑 case 2
#   ./tools/emu/testmenu-tests.sh --clean        # 清输出
#   ./tools/emu/testmenu-tests.sh other.gba      # 测指定 ROM（同目录找 .sym）
#
# 约定：先 `make TEST=1` 产出 fireemblem8-test.gba / .sym。
set -uo pipefail

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)

. "$HERE/testmenu/lib.sh"

. "$HERE/testmenu/cases/01-top-menu.sh"
. "$HERE/testmenu/cases/02-test-root.sh"
. "$HERE/testmenu/cases/03-build-info.sh"
. "$HERE/testmenu/cases/04-start-game.sh"

CASE_KEYS=(1 2 3 4)
CASE_FUNCS=(
	case_01_top_menu
	case_02_test_root
	case_03_build_info
	case_04_start_game
)
CASE_TITLES=(
	"top-menu           开机 → 顶层菜单（探针 magic/menu/state、画面非黑）"
	"test-root          顶层 → Test → Back → 顶层"
	"build-info         Test → System → Build Info → 一路 Back 回顶层"
	"start-game         Start Game → 正常开局流程（探针清零）"
)

usage() {
	cat <<EOF
用法: $0 [--list] [--case N] [--clean] [ROM]

  --list       列出 case
  --case N     只跑第 N 个 case（用 --list 查看）
  --clean      只清掉输出目录
  ROM          默认 fireemblem8-test.gba
EOF
}

CLEAN=0
LIST=0
ONLY=""
ROM=""

while [ $# -gt 0 ]; do
	case "$1" in
	--clean)
		CLEAN=1
		shift
		;;
	--list)
		LIST=1
		shift
		;;
	--case)
		[ $# -ge 2 ] || { usage >&2; exit 2; }
		ONLY=$2
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	-*)
		usage >&2
		exit 2
		;;
	*)
		ROM=$1
		shift
		;;
	esac
done

if [ "$CLEAN" = "1" ]; then
	rm -rf "$OUT"
	echo "已清理测试输出：$OUT"
	exit 0
fi

if [ "$LIST" = "1" ]; then
	for i in "${!CASE_KEYS[@]}"; do
		printf '%s  %s\n' "${CASE_KEYS[$i]}" "${CASE_TITLES[$i]}"
	done
	exit 0
fi

ROM=${ROM:-$ROOT/fireemblem8-test.gba}
SYM="${ROM%.gba}.sym"

testmenu_init "$ROM" "$SYM"

echo "== ROM: $ROM"
echo "== SYM: $SYM"

MATCHED=0
for i in "${!CASE_KEYS[@]}"; do
	if [ -z "$ONLY" ] || [ "$ONLY" = "${CASE_KEYS[$i]}" ]; then
		"${CASE_FUNCS[$i]}"
		MATCHED=1
	fi
done

if [ "$MATCHED" = "0" ]; then
	echo "未知 case: $ONLY" >&2
	exit 2
fi

testmenu_summary
