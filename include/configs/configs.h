#ifndef GUARD_CONFIGS_H
#define GUARD_CONFIGS_H

/*
 * 项目级编译配置。
 *
 * 约定：
 * - 默认值指向「与原版行为一致」，所有开关默认关闭，保证 M1–M4 期间的字节门禁不受影响。
 * - 需要开启时改这里（或改 configs/<name>.h），不要散落在源码里加 #ifdef。
 * - 改了本文件后必须重跑字节门禁（见 AGENTS.md §0.1）。
 */

#define CONFIG_VERSION "v0.1.0"

/* debug 构建总开关；为 1 时包含 configs/config-debug.h */
#define CONFIG_USE_DEBUG 0

/*
 * testmenu：debug 构建开机直接进入测试菜单（替代正常开机流程）。
 * 依赖 CONFIG_USE_DEBUG=1。实现随 M5 落地到 src/Test/。
 */
#define CONFIG_TEST_MENU 0

/* 原版 debug 菜单（bmdebug.c / menu_def.c 已存在） */
#define CONFIG_DEBUG_MENU 0

/* 控制台输出：mGBA 调试打印 / no$gba 打印 */
#define CONFIG_MGBA_PRINT_EN 0
#define CONFIG_NCGB_PRINT_EN 0

#if CONFIG_USE_DEBUG
#include "configs/config-debug.h"
#endif

#endif /* GUARD_CONFIGS_H */
