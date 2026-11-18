#!/usr/bin/env bash
# case 01：开机直接进 testmenu 顶层。
#
# 断言：
#   - 探针 magic 正确（说明 test_probe_reset() 跑了）
#   - menu = TOP、state = MENU（说明顶层菜单 proc 起来了）
#   - 画面不是黑屏（mural + 菜单边框 + 两张脸）
#   - 不崩

case_01_top_menu() {
	gb 01-top 240 --stats 200 --shot 200

	assert_no_crash 01-top "开机不崩溃"
	assert_probe 01-top 200 1 1414747216 "探针 magic = TSTP"
	assert_probe 01-top 200 2 1 "menu = TOP"
	assert_probe 01-top 200 3 1 "state = MENU"
	assert_screen 01-top 200 8 "顶层画面不是黑屏"
}
