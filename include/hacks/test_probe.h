#ifndef GUARD_HACKS_TEST_PROBE_H
#define GUARD_HACKS_TEST_PROBE_H

#include "prelude.h"

/*
 * 测试探针：testmenu 与 emu 自动化测试之间唯一的状态通道。
 *
 * 每个字段都在 testmenu / case 代码里被显式赋值，emu 从固定地址读取，
 * 因此不要依赖未初始化的内容，也不要往里塞指针。
 */

#if CONFIG_TEST_MENU

/* 按 u32 比较，不看内存字节序 */
#define TEST_PROBE_MAGIC 0x54535450u

enum test_probe_menu
{
	TEST_MENU_ID_NONE = 0,
	TEST_MENU_ID_TOP,      /* Start Game / Test */
	TEST_MENU_ID_ROOT,     /* Test 第二层 */
	TEST_MENU_ID_CATEGORY, /* 某分类的子菜单 */
	TEST_MENU_ID_CASE,     /* 某个 case 正在跑 */
};

enum test_probe_state
{
	TEST_MENU_STATE_NONE = 0,
	TEST_MENU_STATE_MENU,
	TEST_MENU_STATE_CASE_RUNNING,
	TEST_MENU_STATE_CASE_DONE,
	TEST_MENU_STATE_CASE_FAILED,
};

enum test_probe_category
{
	TEST_CATEGORY_NONE = 0,
	TEST_CATEGORY_SYSTEM,
};

enum test_probe_case
{
	TEST_CASE_NONE = 0,
	TEST_CASE_BUILD_INFO,
};

struct TestMenuProbe
{
	/* 00 */ u32 magic;
	/* 04 */ u8 menu;      /* enum test_probe_menu */
	/* 05 */ u8 state;     /* enum test_probe_state */
	/* 06 */ u8 category;  /* enum test_probe_category */
	/* 07 */ u8 case_id;   /* enum test_probe_case */
	/* 08 */ u16 step;     /* 多步 case 的步骤号 */
	/* 0A */ u16 flags;    /* case 专用标志位 */
	/* 0C */ u32 value[4]; /* case 专用数值 */
	/* 1C */ u32 frame;    /* 最近一次状态变化时的 GetGameClock() */
}; /* size = 0x20 */

extern struct TestMenuProbe EWRAM_DATA gTestMenuProbe;

void test_probe_reset(void);
void test_probe_set_menu(u8 menu, u8 state);
void test_probe_set_case(u8 category, u8 case_id);
void test_probe_set_state(u8 state);
void test_probe_set_value(int index, u32 value);
void test_probe_set_flags(u16 flags);
void test_probe_finish(bool pass);

#endif /* CONFIG_TEST_MENU */

#endif /* GUARD_HACKS_TEST_PROBE_H */
