#!/usr/bin/env bash
# case 04：Start Game 离开 testmenu，回到正常开局流程。
#
# 120 帧按 A 选 Start Game；之后 StartGame() 接手，探针被清零。
# 画面统计与原生 ROM 对齐（原生同帧也是 OP 动画初期，颜色很少）。

case_04_start_game() {
	gb 04-start 900 --hold 120:A:6 --stats 300 --stats 600 --stats 800

	assert_no_crash 04-start "Start Game 不崩溃"
	assert_probe 04-start 300 2 0 "menu = NONE"
	assert_probe 04-start 300 3 0 "state = NONE"
	assert_probe 04-start 800 2 0 "8 秒后仍在正常开局流程（menu = NONE）"
}
