#!/usr/bin/env bash
# testmenu 测试 runner 的公共部分。被 testmenu-tests.sh source，不单独执行。
#
# 依赖：testmenu_init() 之前设置 HERE / ROOT，以及 ROM / SYM 两个变量。

BUILD="$HERE/.build"
RUN="$BUILD/gbarun"
OUT="$BUILD/out-testmenu"

PASSES=0
FAILS=()

ok() {
	PASSES=$((PASSES + 1))
	printf '  \033[32mPASS\033[0m %s\n' "$1"
}

bad() {
	FAILS+=("$1")
	printf '  \033[31mFAIL\033[0m %s\n' "$1"
}

# 探针字段顺序（与 gb() 里 --probe 的顺序一致）
#   1 magic(u32)  2 menu  3 state  4 category  5 case_id
PROBE_ARGS=(
	--probe "gTestMenuProbe+0x0:32"
	--probe "gTestMenuProbe+0x4:8"
	--probe "gTestMenuProbe+0x5:8"
	--probe "gTestMenuProbe+0x6:8"
	--probe "gTestMenuProbe+0x7:8"
)

testmenu_init() {
	local rom=$1
	local sym=$2

	[ -x "$RUN" ] || { echo "先运行 ./tools/emu/build.sh" >&2; exit 1; }
	[ -f "$rom" ] || { echo "找不到 ROM: $rom（先 make TEST=1）" >&2; exit 1; }
	[ -f "$sym" ] || { echo "找不到符号表: $sym（先 make TEST=1）" >&2; exit 1; }

	rm -rf "$OUT"
	mkdir -p "$OUT"
}

# gb <名字> <帧数> [额外的 gbarun 参数...]
# 每个 case 的日志和截图都落在 $OUT 下，失败时方便回看。
gb() {
	local name=$1
	local frames=$2

	shift 2

	"$RUN" "$ROM" --quiet --frames "$frames" --sym "$SYM" \
		"${PROBE_ARGS[@]}" --out "$OUT" "$@" > "$OUT/$name.log" 2>&1
}

# 取某次运行、某一帧的第 n 个探针字段（1 基）
probe_at() {
	local name=$1
	local frame=$2
	local n=$3

	grep "^PROBE frame=$frame " "$OUT/$name.log" |
		sed -n 's/.*value=\(-*[0-9]*\).*/\1/p' |
		sed -n "${n}p"
}

# assert_probe <名字> <帧> <字段号> <期望> <说明>
assert_probe() {
	local name=$1 frame=$2 n=$3 want=$4 what=$5
	local got

	got=$(probe_at "$name" "$frame" "$n")

	if [ -z "$got" ]; then
		bad "$what（第 $frame 帧没有探针数据）"
	elif [ "$got" = "$want" ]; then
		ok "$what"
	else
		bad "$what（期望 $want，实际 $got）"
	fi
}

# 断言画面不是黑屏：统计行里的 colors 至少 min 种
assert_screen() {
	local name=$1 frame=$2 min=$3 what=$4
	local colors

	colors=$(sed -n "s/^STATS frame=$frame .*colors=\([0-9]*\).*/\1/p" "$OUT/$name.log" | head -1)

	if [ -z "$colors" ]; then
		bad "$what（第 $frame 帧没有画面统计）"
	elif [ "$colors" -ge "$min" ]; then
		ok "$what"
	else
		bad "$what（颜色数 $colors < $min，可能黑屏）"
	fi
}

# 断言没有崩溃
assert_no_crash() {
	local name=$1 what=$2

	if grep -q '^CRASH' "$OUT/$name.log"; then
		bad "$what（$(grep -m1 '^CRASH' "$OUT/$name.log")）"
	else
		ok "$what"
	fi
}

testmenu_summary() {
	echo
	echo "== 结果：$PASSES 通过，${#FAILS[@]} 失败"
	if [ ${#FAILS[@]} -ne 0 ]; then
		printf '  - %s\n' "${FAILS[@]}"
		exit 1
	fi
	exit 0
}
