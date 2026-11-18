#ifndef GUARD_HACKS_TESTMENU_H
#define GUARD_HACKS_TESTMENU_H

#include "prelude.h"

#if CONFIG_TEST_MENU

/* 测试构建的开机入口，被 src/Boot/main.c 的 AgbMain() 调用。 */
void StartGame_TestMenu(void);

#endif /* CONFIG_TEST_MENU */

#endif /* GUARD_HACKS_TESTMENU_H */
