#include "global.h"

#include "hardware.h"
#include "proc.h"
#include "uimenu.h"
#include "uiutils.h"

#include "constants/video-global.h"

#include "hacks/testmenu_common.h"

#if CONFIG_TEST_MENU

/* 菜单占用的两块图层：BG2 画边框（调色板 1），BG0 画文字（调色板 0）。 */
enum
{
	TESTMENU_BG_FRAME = 2,
	TESTMENU_BG_TEXT = 0,
};

static const struct MenuDef *sPendingMenu;

static void testmenu_open_deferred(struct Proc *proc);

const struct ProcCmd CONST_DATA ProcScr_TestMenuOpen[] =
{
	PROC_SLEEP(1),
	PROC_CALL(testmenu_open_deferred),
	PROC_END,
};

void testmenu_clear_ui(void)
{
	BG_Fill(gBG0TilemapBuffer, 0);
	BG_Fill(gBG2TilemapBuffer, 0);
	BG_EnableSyncByMask(BG0_SYNC_BIT | BG2_SYNC_BIT);
}

static void testmenu_open_menu_core(const struct MenuDef *def)
{
	testmenu_clear_ui();

	StartMenuCore(def, def->rect, TESTMENU_BG_FRAME,
		      TILEREF(BGCHR_WINDOW_FRAME, BGPAL_WINDOW_FRAME),
		      TESTMENU_BG_TEXT, 0, NULL);
}

void testmenu_open_menu_now(const struct MenuDef *def)
{
	testmenu_open_menu_core(def);
}

void testmenu_defer_menu(const struct MenuDef *def)
{
	sPendingMenu = def;
	Proc_Start(ProcScr_TestMenuOpen, PROC_TREE_3);
}

/* 当前菜单 proc 在回调返回后才会退出，等一帧再开下一个，避免重入。 */
static void testmenu_open_deferred(struct Proc *proc)
{
	const struct MenuDef *def = sPendingMenu;

	if (def == NULL)
		return;

	sPendingMenu = NULL;
	testmenu_open_menu_core(def);
}

#endif /* CONFIG_TEST_MENU */
