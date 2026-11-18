#ifndef GUARD_HACKS_TESTMENU_COMMON_H
#define GUARD_HACKS_TESTMENU_COMMON_H

#include "prelude.h"
#include "uimenu.h"

#if CONFIG_TEST_MENU

/*
 * testmenu 公共外壳。
 *
 * 菜单切换一律走「延迟打开」：当前菜单 proc 还在回调里，直接 StartMenu 会让两个
 * 菜单 proc 同时存在并抢输入，所以先记下目标菜单，等下一次主循环再打开。
 */

/* 立即打开（只在开机入口用，此时没有别的菜单存在） */
void testmenu_open_menu_now(const struct MenuDef *def);

/* 延迟一帧打开（菜单项回调里用） */
void testmenu_defer_menu(const struct MenuDef *def);

/* 清掉菜单占用的两块图层（文字层 BG0 / 边框层 BG2） */
void testmenu_clear_ui(void);

/* 顶层菜单（Start Game / Test），供子菜单返回时重开 */
void StartTestMenuTop(void);

extern const struct MenuDef MenuInfo_TestTop;
extern const struct MenuDef MenuInfo_TestRoot;

#endif /* CONFIG_TEST_MENU */

#endif /* GUARD_HACKS_TESTMENU_COMMON_H */
