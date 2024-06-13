/* SPDX-License-Identifier: GPL-2.0
 *
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file acmbitops.h
 * @brief ACM Driver Bit Access Helpers
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_BITOPS_H_
#define ACM_BITOPS_H_

#include <linux/types.h>
#include <linux/bitops.h>

/**
 * @brief get lower 16bits of a 32bit word
 */
#define low_16bits(x)  ((x) & 0xFFFF)
/**
 * @brief get higher 16bits of a 32bit word
 */
#define high_16bits(x) (((x) & 0xFFFF0000) >> 16)

/**
 * @brief read value determined by bit mask within 32bit value
 */
static inline u32 read_bitmask(const u32 *from, u32 mask)
{
	if (mask == 0)
		return 0;

	return (*from & mask) >> __ffs(mask);
}

/**
 * @brief write value determined by bit mask into 32bit value
 */
static inline void write_bitmask(u32 val, u32 *to, u32 mask)
{
	if (mask == 0)
		return;

	*to = (*to & ~mask) | ((val << __ffs(mask)) & mask);
}

/**
 * @brief read value determined by bit mask within 16bit value
 */
static inline u16 read_bitmask16(const u16 *from, u16 mask)
{
	if (mask == 0)
		return 0;

	return (*from & mask) >> __ffs(mask);
}

/**
 * @brief write value determined by bit mask into 16bit value
 */
static inline void write_bitmask16(u16 val, u16 *to, u16 mask)
{
	if (mask == 0)
		return;

	*to = (*to & ~mask) | ((val << __ffs(mask)) & mask);
}

/**
 * @brief read value determined by bit mask within 64bit value
 */
static inline u64 read_bitmask64(const u64 *from, u64 mask)
{
	if (mask == 0)
		return 0;

	return (*from & mask) >> __ffs64(mask);
}


#endif /* ACM_BITOPS_H_ */
