#ifndef GUARD_CONFIGS_CONFIG_DEBUG_H
#define GUARD_CONFIGS_CONFIG_DEBUG_H

/*
 * debug 构建的配置覆盖。
 * 只在 include/configs/configs.h 里 CONFIG_USE_DEBUG=1 时被包含。
 */

/* debug 构建默认进 testmenu（M5 落地后可开启） */
#undef CONFIG_TEST_MENU
#define CONFIG_TEST_MENU 1

#undef CONFIG_DEBUG_MENU
#define CONFIG_DEBUG_MENU 1

#endif /* GUARD_CONFIGS_CONFIG_DEBUG_H */
