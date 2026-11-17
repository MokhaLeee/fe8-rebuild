#ifndef GUARD_HACKS_KLIB_H
#define GUARD_HACKS_KLIB_H

/*
 * klib —— 无 __aeabi_ 依赖的运行期辅助例程。
 *
 * 为什么存在：tools/agbcc/lib/libgcc.a 只提供 __udivsi3 系列，没有 GCC 会引用的
 * __aeabi_uidiv / __aeabi_uidivmod，所以 M6（迁移到 arm-none-eabi-gcc）之后
 * 直接写 / 和 % 会在链接期失败。规则见 AGENTS.md §0.2。
 *
 * 当前状态：只有接口，没有实现。实现随 M6 一起落地到
 * src/Common/klib/math.c；在此之前本头文件不被任何代码包含，不影响构建。
 */

#include "types.h"

/* 除法 / 取模（无符号 32 位） */
u32 k_udiv(u32 a, u32 b);	/* a / b，b 为 0 时返回 0 */
u32 k_umod(u32 a, u32 b);	/* a % b，b 为 0 时返回 0 */
u32 k_udiv10(u32 n);		/* n / 10 */
u32 k_umod10(u32 n);		/* n % 10 */

/* 除法 / 取模（无符号 64 位） */
u64 k_u64div(u64 a, u64 b);
u64 k_u64mod(u64 a, u64 b);

/*
 * 有符号除法：klib 只提供无符号接口，符号在调用点处理。
 * 负数不要直接传给无符号接口。
 */
static inline s32 k_sdiv(s32 a, s32 b)
{
	return a >= 0 ? (s32)k_udiv(a, b) : -(s32)k_udiv(-a, b);
}

static inline s32 k_smod(s32 a, s32 b)
{
	return a >= 0 ? (s32)k_umod(a, b) : -(s32)k_umod(-a, b);
}

/* 百分比：num 的 perc% */
static inline u32 perc_of(u32 num, u32 perc)
{
	return k_udiv(num * perc, 100);
}

/* 位操作 */
#define BIT(n) (1UL << (n))
#define GENMASK_32(h, l) \
	(((0xffffffff) << (l)) & (0xffffffff >> (32 - 1 - (h))))

static inline void set_bit(void *bits, u32 idx)
{
	u8 *_bits = bits;

	_bits[idx >> 3] |= 1 << (idx & 0x7);
}

static inline void clear_bit(void *bits, u32 idx)
{
	u8 *_bits = bits;

	_bits[idx >> 3] &= ~(1 << (idx & 0x7));
}

static inline bool check_bit(void *bits, u32 idx)
{
	u8 *_bits = bits;

	return !!(_bits[idx >> 3] & (1 << (idx & 0x7)));
}

#endif /* GUARD_HACKS_KLIB_H */
