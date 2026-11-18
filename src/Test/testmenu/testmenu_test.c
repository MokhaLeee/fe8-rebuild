#include "global.h"

#include "hardware.h"
#include "uimenu.h"

#include "hacks/test_probe.h"
#include "hacks/testmenu_common.h"

#if CONFIG_TEST_MENU

static void testmenu_test_on_init(struct MenuProc *menu);
static u8 testmenu_test_on_select_system(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_test_on_back(struct MenuProc *menu, struct MenuItemProc *item);

static void testmenu_system_on_init(struct MenuProc *menu);
static u8 testmenu_system_on_select_build_info(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_system_on_back(struct MenuProc *menu, struct MenuItemProc *item);

static void testmenu_build_info_on_init(struct MenuProc *menu);
static u8 testmenu_build_info_on_back(struct MenuProc *menu, struct MenuItemProc *item);

/* Test 第二层：分类入口。新分类在这里加一条，再实现对应的子菜单。 */
static const struct MenuItemDef sTestRootItems[] =
{
	{
		.name = " System",
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_test_on_select_system,
	},
	{
		.name = " Back",
		.color = TEXT_COLOR_SYSTEM_GREEN,
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_test_on_back,
	},
	MenuItemsEnd,
};

const struct MenuDef MenuInfo_TestRoot =
{
	.rect = { 9, 4, 12, 0 },
	.style = 0,
	.menuItems = sTestRootItems,
	.onInit = testmenu_test_on_init,
	.onBPress = testmenu_test_on_back,
};

static const struct MenuItemDef sSystemItems[] =
{
	{
		.name = " Build Info",
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_system_on_select_build_info,
	},
	{
		.name = " Back",
		.color = TEXT_COLOR_SYSTEM_GREEN,
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_system_on_back,
	},
	MenuItemsEnd,
};

static const struct MenuDef sSystemMenu =
{
	.rect = { 9, 4, 14, 0 },
	.style = 0,
	.menuItems = sSystemItems,
	.onInit = testmenu_system_on_init,
	.onBPress = testmenu_system_on_back,
};

/*
 * 第一个自检 case：只把版本号画出来，证明「分类 → case → 返回」这条链路和探针
 * 状态机是通的。真正的游戏流程 case（进章节 / 存档 / 战斗）后续再加。
 */
static const struct MenuItemDef sBuildInfoItems[] =
{
	{
		.name = " " CONFIG_VERSION,
		.isAvailable = MenuAlwaysDisabled,
	},
	{
		.name = " " __DATE__,
		.isAvailable = MenuAlwaysDisabled,
	},
	{
		.name = " " __TIME__,
		.isAvailable = MenuAlwaysDisabled,
	},
	{
		.name = " Back",
		.color = TEXT_COLOR_SYSTEM_GREEN,
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_build_info_on_back,
	},
	MenuItemsEnd,
};

static const struct MenuDef sBuildInfoMenu =
{
	.rect = { 5, 5, 18, 0 },
	.style = 0,
	.menuItems = sBuildInfoItems,
	.onInit = testmenu_build_info_on_init,
	.onBPress = testmenu_build_info_on_back,
};

static void testmenu_test_on_init(struct MenuProc *menu)
{
	test_probe_set_menu(TEST_MENU_ID_ROOT, TEST_MENU_STATE_MENU);
}

static u8 testmenu_test_on_select_system(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&sSystemMenu);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

static u8 testmenu_test_on_back(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&MenuInfo_TestTop);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6B | MENU_ACT_CLEAR;
}

static void testmenu_system_on_init(struct MenuProc *menu)
{
	test_probe_set_menu(TEST_MENU_ID_CATEGORY, TEST_MENU_STATE_MENU);
}

static u8 testmenu_system_on_select_build_info(struct MenuProc *menu, struct MenuItemProc *item)
{
	test_probe_set_case(TEST_CATEGORY_SYSTEM, TEST_CASE_BUILD_INFO);
	testmenu_defer_menu(&sBuildInfoMenu);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

static u8 testmenu_system_on_back(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&MenuInfo_TestRoot);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6B | MENU_ACT_CLEAR;
}

static void testmenu_build_info_on_init(struct MenuProc *menu)
{
	test_probe_finish(TRUE);
}

static u8 testmenu_build_info_on_back(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&sSystemMenu);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6B | MENU_ACT_CLEAR;
}

#endif /* CONFIG_TEST_MENU */
