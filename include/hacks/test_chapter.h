#ifndef GUARD_HACKS_TEST_CHAPTER_H
#define GUARD_HACKS_TEST_CHAPTER_H

#include "prelude.h"

#if CONFIG_TEST_MENU

/* 测试章节的单位表（src/Test/fixtures/test-chapter.c）。
   准备画面用 UnitDef_Test_Blue，开场事件再加载红方。 */
extern CONST_DATA struct UnitDefinition UnitDef_Test_Blue[];
extern CONST_DATA struct UnitDefinition UnitDef_Test_Red[];

#endif /* CONFIG_TEST_MENU */

#endif /* GUARD_HACKS_TEST_CHAPTER_H */
