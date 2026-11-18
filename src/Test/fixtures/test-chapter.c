#include "global.h"

#include "bmunit.h"
#include "chapterdata.h"
#include "event.h"
#include "eventinfo.h"
#include "EAstdlib.h"

#include "constants/characters.h"
#include "constants/classes.h"
#include "constants/items.h"

#include "hacks/test_chapter.h"

#if CONFIG_TEST_MENU

/*
 * 测试章节 CHAPTER_TEST。
 *
 * 章节数据在 src/data/chapter_settings.h 的表尾（CONFIG_TEST_MENU 下），
 * 事件组通过 data/data_8B363C.s 的 gChapterDataAssetTable 尾部条目挂上去，
 * 和原版章节的注册方式一致。
 *
 * 地图资源直接复用第一章（L01）的 id，所以不会新增素材。
 */

CONST_DATA struct UnitDefinition UnitDef_Test_Blue[] =
{
	{
		.charIndex = CHARACTER_EIRIKA,
		.classIndex = CLASS_EIRIKA_LORD,
		.leaderCharIndex = CHARACTER_EIRIKA,
		.allegiance = FACTION_ID_BLUE,
		.level = 1,
		.xPosition = 14,
		.yPosition = 9,
		.items = {
			ITEM_SWORD_RAPIER,
			ITEM_VULNERARY,
		},
	},
	{
		.charIndex = CHARACTER_SETH,
		.classIndex = CLASS_PALADIN,
		.leaderCharIndex = CHARACTER_EIRIKA,
		.allegiance = FACTION_ID_BLUE,
		.level = 1,
		.xPosition = 13,
		.yPosition = 9,
		.items = {
			ITEM_LANCE_IRON,
		},
	},
	{ 0 },
};

CONST_DATA struct UnitDefinition UnitDef_Test_Red[] =
{
	{
		.charIndex = CHARACTER_BREGUET,
		.classIndex = CLASS_ARMOR_KNIGHT,
		.leaderCharIndex = CHARACTER_BREGUET,
		.allegiance = FACTION_ID_RED,
		.level = 4,
		.xPosition = 5,
		.yPosition = 9,
		.items = {
			ITEM_LANCE_IRON,
		},
		.ai = { 0, 0, 0, 0 },
	},
	{ 0 },
};

/* 章节开场：先放我方，再放敌方。ENUN 等单位进场动画结束。
   两个 SVAL 是探针，用来确认开场事件确实执行了。 */
CONST_DATA EventScr EventScr_Test_Beginning[] =
{
	SVAL(EVT_SLOT_3, 0x1234)

	/* TODO: 蓝方暂时不加载。FACTION_ID_BLUE 走 GetFreeBlueUnit()/LoadUnit()，
	   在测试场景下会把 EWRAM（gBmSt/gPlaySt/单位数组）写坏，还没定位到
	   具体是哪一步；红方（FACTION_ID_RED）正常。 */
	LOAD1(1, UnitDef_Test_Red)
	ENUN

	SVAL(EVT_SLOT_4, 0x5678)
	ENDA
};

CONST_DATA EventScr EventScr_Test_Ending[] =
{
	ENDA
};

CONST_DATA EventListScr EventListScr_Test_Turn[] =
{
	END_MAIN
};

CONST_DATA EventListScr EventListScr_Test_Character[] =
{
	END_MAIN
};

CONST_DATA EventListScr EventListScr_Test_Location[] =
{
	END_MAIN
};

CONST_DATA EventListScr EventListScr_Test_Misc[] =
{
	END_MAIN
};

CONST_DATA EventListScr EventListScr_Test_SelectUnit[] =
{
	END_MAIN
};

CONST_DATA EventListScr EventListScr_Test_SelectDestination[] =
{
	END_MAIN
};

CONST_DATA EventListScr EventListScr_Test_UnitMoved[] =
{
	END_MAIN
};

/* 教学事件表：只有终止项（NULL），TUTORIAL_MODE 打开时也不会越界。 */
CONST_DATA EventListScr *EventListScr_Test_Tutorial[] =
{
	NULL
};

CONST_DATA struct ChapterEventGroup ChapterEventGroup_Test =
{
	.turnBasedEvents = EventListScr_Test_Turn,
	.characterBasedEvents = EventListScr_Test_Character,
	.locationBasedEvents = EventListScr_Test_Location,
	.miscBasedEvents = EventListScr_Test_Misc,

	.specialEventsWhenUnitSelected = EventListScr_Test_SelectUnit,
	.specialEventsWhenDestSelected = EventListScr_Test_SelectDestination,
	.specialEventsAfterUnitMoved = EventListScr_Test_UnitMoved,
	.tutorialEvents = EventListScr_Test_Tutorial,

	/* LoadTrapData() 对 NULL 是安全的 */
	.traps = NULL,
	.extraTrapsInHard = NULL,

	.playerUnitsInNormal = UnitDef_Test_Blue,
	.playerUnitsInHard = UnitDef_Test_Blue,

	.playerUnitsChoice1InEncounter = NULL,
	.playerUnitsChoice2InEncounter = NULL,
	.playerUnitsChoice3InEncounter = NULL,

	.enemyUnitsChoice1InEncounter = NULL,
	.enemyUnitsChoice2InEncounter = NULL,
	.enemyUnitsChoice3InEncounter = NULL,

	.beginningSceneEvents = EventScr_Test_Beginning,
	.endingSceneEvents = EventScr_Test_Ending,
};

#endif /* CONFIG_TEST_MENU */
