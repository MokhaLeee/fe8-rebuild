#include "global.h"

#include "bm.h"
#include "face.h"
#include "fontgrp.h"
#include "gamecontrol.h"
#include "hardware.h"
#include "proc.h"
#include "soundwrapper.h"
#include "uimenu.h"
#include "uiutils.h"

#include "constants/faces.h"
#include "constants/songs.h"
#include "constants/video-global.h"

#include "hacks/test_probe.h"
#include "hacks/testmenu.h"
#include "hacks/testmenu_common.h"

#if CONFIG_TEST_MENU

/* 标题文字的专用字库，避免和菜单项共用 gDefaultFont 的 chr_counter。 */
enum
{
	BGCHR_TESTMENU_TITLE = 0x180,
};

EWRAM_DATA static struct Font sTestMenuTitleFont = { 0 };
EWRAM_DATA static struct Text sTestMenuTitleText = { 0 };

static ProcPtr sTestMuralProc;

static void testmenu_top_on_init(struct MenuProc *menu);
static void testmenu_top_on_end(struct MenuProc *menu);
static u8 testmenu_on_select_start_game(struct MenuProc *menu, struct MenuItemProc *item);
static u8 testmenu_on_select_test(struct MenuProc *menu, struct MenuItemProc *item);
static void testmenu_draw_title(void);
static void testmenu_start_game(struct Proc *proc);

static const struct ProcCmd ProcScr_TestMenuStartGame[] =
{
	PROC_SLEEP(1),
	PROC_CALL(testmenu_start_game),
	PROC_END,
};

static const struct MenuItemDef sTestTopItems[] =
{
	{
		.name = " Start Game",
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_on_select_start_game,
	},
	{
		.name = " Test",
		.color = TEXT_COLOR_SYSTEM_GREEN,
		.isAvailable = MenuAlwaysEnabled,
		.onSelected = testmenu_on_select_test,
	},
	MenuItemsEnd,
};

const struct MenuDef MenuInfo_TestTop =
{
	.rect = { 9, 4, 12, 0 },
	.style = 0,
	.menuItems = sTestTopItems,
	.onInit = testmenu_top_on_init,
	.onEnd = testmenu_top_on_end,
};

static void testmenu_draw_title(void)
{
	InitTextFont(&sTestMenuTitleFont, (void *)(VRAM + BGCHR_TESTMENU_TITLE * 0x20),
		     BGCHR_TESTMENU_TITLE, BGPAL_TEXT_DEFAULT);
	InitText(&sTestMenuTitleText, 20);
	Text_DrawString(&sTestMenuTitleText, "TEST MENU");
	PutText(&sTestMenuTitleText, TILEMAP_LOCATED(gBG0TilemapBuffer, 2, 1));
	SetTextFont(NULL);

	BG_EnableSyncByMask(BG0_SYNC_BIT);
}

static void testmenu_top_on_init(struct MenuProc *menu)
{
	test_probe_set_menu(TEST_MENU_ID_TOP, TEST_MENU_STATE_MENU);

	StartFace(0, FID_EIRIKA, 32, 80,
		  FACE_DISP_KIND(FACE_96x80_FLIPPED) | FACE_DISP_HLAYER(4));
	StartFace(1, FID_EPHRAIM, DISPLAY_WIDTH - 32, 80,
		  FACE_DISP_KIND(FACE_96x80) | FACE_DISP_HLAYER(4));

	testmenu_draw_title();
}

static void testmenu_top_on_end(struct MenuProc *menu)
{
	EndFaceById(0);
	EndFaceById(1);
}

/* 延迟一帧再进游戏：让顶层菜单的 proc、face 都先退干净。 */
static void testmenu_start_game(struct Proc *proc)
{
	if (sTestMuralProc != NULL)
	{
		Proc_End(sTestMuralProc);
		sTestMuralProc = NULL;
	}

	StartGame();
}

static u8 testmenu_on_select_start_game(struct MenuProc *menu, struct MenuItemProc *item)
{
	test_probe_set_menu(TEST_MENU_ID_NONE, TEST_MENU_STATE_NONE);

	Proc_Start(ProcScr_TestMenuStartGame, PROC_TREE_3);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

static u8 testmenu_on_select_test(struct MenuProc *menu, struct MenuItemProc *item)
{
	testmenu_defer_menu(&MenuInfo_TestRoot);

	return MENU_ACT_SKIPCURSOR | MENU_ACT_END | MENU_ACT_SND6A | MENU_ACT_CLEAR;
}

void StartTestMenuTop(void)
{
	testmenu_open_menu_now(&MenuInfo_TestTop);
}

void StartGame_TestMenu(void)
{
	SetMainUpdateRoutine(OnMain);
	SetInterrupt_LCDVBlank(OnVBlank);

	SetupBackgrounds(NULL);
	LoadUiFrameGraphics();
	ResetText();

	test_probe_reset();

	sTestMuralProc = StartMuralBackground(PROC_TREE_3, NULL, -1);

	StartTestMenuTop();
}

#endif /* CONFIG_TEST_MENU */
