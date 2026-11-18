#include "global.h"

#include "hardware.h"

#include "hacks/test_probe.h"

#if CONFIG_TEST_MENU

/* EWRAM 布局见 lds/ldscript.txt 的 ewram_test 段（固定 0x0203FF00） */
struct TestMenuProbe EWRAM_DATA gTestMenuProbe;

typedef char test_probe_size_check[(sizeof(struct TestMenuProbe) == 0x20) ? 1 : -1];

void test_probe_reset(void)
{
	int i;

	gTestMenuProbe.magic = TEST_PROBE_MAGIC;
	gTestMenuProbe.menu = TEST_MENU_ID_NONE;
	gTestMenuProbe.state = TEST_MENU_STATE_NONE;
	gTestMenuProbe.category = TEST_CATEGORY_NONE;
	gTestMenuProbe.case_id = TEST_CASE_NONE;
	gTestMenuProbe.step = 0;
	gTestMenuProbe.flags = 0;
	gTestMenuProbe.frame = GetGameClock();

	for (i = 0; i < 4; i++)
		gTestMenuProbe.value[i] = 0;
}

void test_probe_set_menu(u8 menu, u8 state)
{
	gTestMenuProbe.menu = menu;
	gTestMenuProbe.state = state;
	gTestMenuProbe.frame = GetGameClock();
}

void test_probe_set_case(u8 category, u8 case_id)
{
	gTestMenuProbe.menu = TEST_MENU_ID_CASE;
	gTestMenuProbe.state = TEST_MENU_STATE_CASE_RUNNING;
	gTestMenuProbe.category = category;
	gTestMenuProbe.case_id = case_id;
	gTestMenuProbe.step = 0;
	gTestMenuProbe.frame = GetGameClock();
}

void test_probe_set_state(u8 state)
{
	gTestMenuProbe.state = state;
	gTestMenuProbe.frame = GetGameClock();
}

void test_probe_set_value(int index, u32 value)
{
	if (index >= 0 && index < 4)
		gTestMenuProbe.value[index] = value;
}

void test_probe_set_flags(u16 flags)
{
	gTestMenuProbe.flags = flags;
}

void test_probe_finish(bool pass)
{
	gTestMenuProbe.state = pass ? TEST_MENU_STATE_CASE_DONE
				    : TEST_MENU_STATE_CASE_FAILED;
	gTestMenuProbe.frame = GetGameClock();
}

#endif /* CONFIG_TEST_MENU */
