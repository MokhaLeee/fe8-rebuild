#include "global.h"

#include "bm.h"
#include "bmio.h"
#include "bmunit.h"
#include "bmcontainer.h"
#include "chapterdata.h"
#include "eventinfo.h"
#include "gamecontrol.h"
#include "hardware.h"
#include "proc.h"

#include "constants/chapters.h"

#include "hacks/test_chapter.h"
#include "hacks/test_probe.h"
#include "hacks/test_scene.h"

#if CONFIG_TEST_MENU

/*
 * 切到指定章节。
 *
 * 直接走 StartBattleMap()，而不是 RestartGameAndGoto8()：
 * 后者会进入 GameControl 的「章节结束」状态机（label 9 -> PostChapterSwitch），
 * 在测试场景下会把章节切回标题/存档菜单。
 *
 * StartBattleMap() 之后地图 proc 在 PROC_TREE_2 里跑，章节开场事件由
 * BmMain_StartIntroFx（测试章节跳过）之后的 CallBeginningEvents() 触发，
 * 单位则来自 test chapter 的 beginningSceneEvents。
 */
void test_scene_start_chapter(int chapter)
{
	const struct ROMChapterData *info;
	struct GameCtrlProc *gameCtrl;

	/* 先结束 testmenu 的 mural proc，否则它会一直改 BG3 的滚动位置 */
	EndMuralBackground();

	InitPlayConfig(0, 0);

	gPlaySt.chapterIndex = chapter;
	gPlaySt.chapterModeIndex = 0;
	gPlaySt.chapterStateBits = 0;
	gPlaySt.faction = FACTION_BLUE;
	gPlaySt.chapterTurnNumber = 1;
	gPlaySt.time_saved = 0;
	gPlaySt.time_chapter_started = 0;
	gPlaySt.partyGoldAmount = 0;
	gPlaySt.gameSaveSlot = 0;

	info = GetROMChapterStruct(chapter);
	gPlaySt.xCursor = info->initialPosX;
	gPlaySt.yCursor = info->initialPosY;

	InitUnits();
	ClearSupplyItems();
	ResetPermanentFlags();
	ResetChapterFlags();

	test_probe_set_case(TEST_CATEGORY_GAME, TEST_CASE_TEST_CHAPTER);
	test_probe_set_value(0, chapter);

	Proc_EndEach(gProcScr_GameControl);
	gameCtrl = Proc_Start(gProcScr_GameControl, PROC_TREE_3);
	gameCtrl->nextAction = 0;
	gameCtrl->nextChapter = 0;

	StartBattleMap(gameCtrl);


}

void test_scene_start_test_chapter(void)
{
	test_scene_start_chapter(CHAPTER_TEST);
}

#endif /* CONFIG_TEST_MENU */
