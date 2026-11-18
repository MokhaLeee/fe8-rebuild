#!/usr/bin/env bash
# case 03：System -> Build Info -> 一路 Back 回顶层。
#
# Test 第二层现在的顺序是 Game / System / Back，所以进第二层后要多按一次 DOWN。
#
# 按键脚本：
#   120 帧  DOWN      选中 Test
#   180 帧  A         进 Test 第二层（默认选 Game）
#   260 帧  DOWN      选中 System
#   320 帧  A         进 System
#   460 帧  A         进 Build Info（第一个 case）
#   600 帧  B         回 System
#   720 帧  B         回 Test
#   860 帧  B         回顶层
#
# 期望探针序列：TOP → ROOT → CATEGORY → CASE(DONE) → CATEGORY → ROOT → TOP。

case_03_build_info() {
	gb 03-case 1100 \
		--hold 120:DOWN:6 --hold 180:A:6 \
		--hold 260:DOWN:6 --hold 320:A:6 --hold 460:A:6 \
		--hold 600:B:6 --hold 720:B:6 --hold 860:B:6 \
		--stats 250 --stats 400 --stats 550 --stats 700 --stats 800 --stats 1000 \
		--shot 550

	assert_no_crash 03-case "case 流程不崩溃"
	assert_probe 03-case 250 2 2 "menu = TEST_ROOT"
	assert_probe 03-case 400 2 3 "menu = CATEGORY(System)"
	assert_probe 03-case 550 2 4 "menu = CASE"
	assert_probe 03-case 550 3 3 "case state = CASE_DONE"
	assert_probe 03-case 550 4 2 "category = SYSTEM"
	assert_probe 03-case 550 5 2 "case_id = BUILD_INFO"
	assert_screen 03-case 550 8 "Build Info 画面不是黑屏"
	assert_probe 03-case 700 2 3 "Back 回 System"
	assert_probe 03-case 800 2 2 "Back 回 Test 第二层"
	assert_probe 03-case 1000 2 1 "Back 回顶层"
}
