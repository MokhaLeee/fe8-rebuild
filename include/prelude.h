#ifndef PRELUDE_H

// 项目级编译开关（CONFIG_* 宏）。放在最前面，保证所有源文件在解析任何条件编译前
// 都能看到它们；开关默认值见 configs/configs.h。
#include "configs/configs.h"

// HACK: applying this macro seems to allow for bitpacked structs to have the same layout in agbcc and modern GCC
#define ALIGN(m) __attribute__((aligned (m)))
#define BITPACKED __attribute__((aligned(4), packed))

// this is for denoting objects that *should* be const, but weren't in the original source (resulting in them being emitted in the .data section)
#define SECTION(name) __attribute__((section(name)))
#define CONST_DATA SECTION(".data")
#define EWRAM_OVERLAY(id) SECTION("ewram_overlay_" # id)

// this is for denoting objects that *should* be const, but need to not be for functions to match.
#define SHOULD_BE_CONST

#define NAKEDFUNC __attribute__((naked))

#endif // PRELUDE_H
