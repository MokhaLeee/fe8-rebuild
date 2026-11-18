#ifndef GUARD_HACKS_TEST_SCENE_H
#define GUARD_HACKS_TEST_SCENE_H

#include "prelude.h"

#if CONFIG_TEST_MENU

/*
 * 场景辅助：把游戏切到指定章节。
 *
 * test_scene_start_chapter() 走的是「读档后进地图」那条路径
 * （RestartGameAndGoto8），因此章节开场事件会照常执行：测试章节的
 * beginningSceneEvents 会加载双方单位，并在需要时打开准备画面。
 */
void test_scene_start_chapter(int chapter);

/* 测试章节：复用第一章地图 + 准备画面 */
void test_scene_start_test_chapter(void);

/* 结束 testmenu 残留的 mural，再切章节 */
void test_scene_end_mural(void);

#endif /* CONFIG_TEST_MENU */

#endif /* GUARD_HACKS_TEST_SCENE_H */
