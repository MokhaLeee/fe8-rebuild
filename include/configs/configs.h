#ifndef GUARD_CONFIGS_H
#define GUARD_CONFIGS_H

/*
 * 项目级编译配置。
 *
 * 约定：
 * - 默认值指向「与原版行为一致」，所有开关默认关闭，保证 M1–M4 期间的字节门禁不受影响。
 * - 需要开启时改这里（或改 configs/<name>.h），不要散落在源码里加 #ifdef。
 * - 本文件被 include/prelude.h 包含，因此每个 .c 都能看到这些宏。
 * - 改动后必须重跑字节门禁（见 AGENTS.md §0.1）。
 *
 * 命令行覆盖：
 *   make TEST=1 会追加 -DCONFIG_USE_DEBUG=1，打开 configs/config-debug.h 这一层。
 *   因此下面几个总开关用 #ifndef 保护，命令行 -D 优先于本文件的默认值。
 */

#ifndef CONFIG_VERSION
#define CONFIG_VERSION "v0.1.0"
#endif

/* debug 构建总开关；为 1 时包含 configs/config-debug.h */
#ifndef CONFIG_USE_DEBUG
#define CONFIG_USE_DEBUG 0
#endif

/*
 * testmenu：debug 构建开机直接进入测试菜单（替代正常开机流程）。
 * 依赖 CONFIG_USE_DEBUG=1。实现在 src/Test/testmenu/。
 */
#ifndef CONFIG_TEST_MENU
#define CONFIG_TEST_MENU 0
#endif

/* 原版 debug 菜单（bmdebug.c / menu_def.c 已存在） */
#ifndef CONFIG_DEBUG_MENU
#define CONFIG_DEBUG_MENU 0
#endif

/* 控制台输出：mGBA 调试打印 / no$gba 打印 */
#define CONFIG_MGBA_PRINT_EN 0
#define CONFIG_NCGB_PRINT_EN 0

#if CONFIG_USE_DEBUG
#include "configs/config-debug.h"
#endif

#endif /* GUARD_CONFIGS_H */
