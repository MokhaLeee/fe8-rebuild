#include "global.h"

#include "hardware.h"
#include "uimenu.h"

#include "hacks/test_probe.h"
#include "hacks/test_scene.h"
#include "hacks/testmenu_common.h"

#if CONFIG_TEST_MENU

static void testmenu_test_on_init(struct MenuProc *menu);
static u8 testmenu_test_on_select_game(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_test_on_select_system(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_test_on_back(struct MenuProc *menu, struct MenuItemProc *item);

static u8 testmenu_game_on_select_test_chapter(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_game_on_back(struct MenuProc *menu, struct MenuItemProc *item);

static void testmenu_system_on_init(struct MenuProc *menu);
static u8 testmenu_system_on_select_build_info(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_system_on_back(struct MenuProc *menu, struct MenuItemProc *item);

static void testmenu_build_info_on_init(struct MenuProc *menu);
static u8 testmenu_build_info_on_back(struct MenuProc *menu, struct MenuItemProc *item);

/* Test 第二层：分类入口。新分类在这里加一条，再实现对应的子菜单。 */
static const struct MenuItemDef sTestRootItems[] =
{
	{
		.name = " Game",
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_test_on_select_game,
	},
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

/* Game：目前只有统一的测试章节，后续的章节/存档/战斗 case 都挂在它下面。 */
static const struct MenuItemDef sGameItems[] =
{
	{
		.name = " Test Chapter",
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_game_on_select_test_chapter,
	},
	{
		.name = " Back",
		.color = TEXT_COLOR_SYSTEM_GREEN,
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_game_on_back,
	},
	MenuItemsEnd,
};

static const struct MenuDef sGameMenu =
{
	.rect = { 9, 4, 16, 0 },
	.style = 0,
	.menuItems = sGameItems,
	.onBPress = testmenu_game_on_back,
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
 * 自检 case：只把版本号画出来，证明「分类 → case → 返回」这条链路和探针
 * 状态机是通的。
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

static u8 testmenu_test_on_select_game(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&sGameMenu);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
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

/*
 * 进测试章节：菜单先正常结束（MENU_ACT_END + CLEAR），章节要等下一帧再切，
 * 否则 StartBattleMap 会在菜单 proc 还活着的时候把主循环换掉。
 * 这里复用 testmenu 的延迟打开机制：把「切换章节」也做成一个 proc 调用不可行，
 * 所以直接用一个一次性 proc 延迟一帧。
 */
static void testmenu_game_start_chapter(struct Proc *proc);
static struct ProcCmd CONST_DATA sProc_TestMenuStartChapter[] =
{
	PROC_SLEEP(1),
	PROC_CALL(testmenu_game_start_chapter),
	PROC_END,
};

static void testmenu_game_start_chapter(struct Proc *proc)
{
	test_scene_start_test_chapter();
}

static u8 testmenu_game_on_select_test_chapter(struct MenuProc *menu, struct MenuItemProc *item)
{
	Proc_Start(sProc_TestMenuStartChapter, PROC_TREE_3);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

static u8 testmenu_game_on_back(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&MenuInfo_TestRoot);

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
