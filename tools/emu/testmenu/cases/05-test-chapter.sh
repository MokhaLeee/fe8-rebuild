#!/usr/bin/env bash
# case 05：Test -> Game -> Test Chapter，进入测试章节地图。
#
# 按键脚本：
#   120 帧  DOWN   选中 Test
#   180 帧  A      进 Test 第二层（Game 是第一项）
#   300 帧  A      进 Game
#   420 帧  A      选 Test Chapter，切到 CHAPTER_TEST
#
# 断言：
#   - 探针 category=GAME、case=TEST_CHAPTER
#   - gPlaySt.chapterIndex == 0x4F（CHAPTER_TEST，章节表尾）
#   - 画面不是黑屏（地图 + UI）

case_05_test_chapter() {
	gb 05-chapter 1200 \
		--hold 120:DOWN:6 --hold 180:A:6 --hold 300:A:6 --hold 420:A:6 \
		--stats 450 --stats 900 --stats 1200 --shot 1200 \
		--probe "gPlaySt+0xE:8"

	assert_no_crash 05-chapter "进测试章节不崩溃"
	assert_probe 05-chapter 450 2 4 "menu = CASE"
	assert_probe 05-chapter 450 4 1 "category = GAME"
	assert_probe 05-chapter 450 5 1 "case_id = TEST_CHAPTER"
	assert_probe 05-chapter 900 6 79 "gPlaySt.chapterIndex = CHAPTER_TEST"
	assert_screen 05-chapter 1200 8 "章节地图不是黑屏"
}
