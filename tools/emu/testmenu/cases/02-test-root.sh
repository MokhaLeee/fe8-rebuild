#!/usr/bin/env bash
# case 02：Test 第二层菜单，以及 Back 回顶层。
#
# 按键脚本：
#   120 帧  DOWN  选中 Test
#   180 帧  A     进 Test 第二层
#   320 帧  B     回顶层
#
# 断言探针 menu：TOP(1) → ROOT(2) → TOP(1)。

case_02_test_root() {
	gb 02-root 480 \
		--hold 120:DOWN:6 --hold 180:A:6 --hold 320:B:6 \
		--stats 150 --stats 250 --stats 420 --shot 250

	assert_no_crash 02-root "菜单导航不崩溃"
	assert_probe 02-root 150 2 1 "进第二层前 menu = TOP"
	assert_probe 02-root 250 2 2 "进第二层后 menu = TEST_ROOT"
	assert_probe 02-root 250 3 1 "第二层 state = MENU"
	assert_probe 02-root 420 2 1 "Back 之后 menu = TOP"
	assert_screen 02-root 250 8 "第二层画面不是黑屏"
}
