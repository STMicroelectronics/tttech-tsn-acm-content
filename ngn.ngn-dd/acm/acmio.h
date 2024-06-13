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
 * @file acmio.h
 * @brief ACM Driver IO Helpers
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_ACMIO_H_
#define ACM_ACMIO_H_

#include <linux/io.h>

/**
 * @brief check if @p to, @p from and @p size are word aligned
 */
static inline bool is_aligned32(void *to, const void *from, size_t size)
{
	return IS_ALIGNED((long)to, sizeof(u32)) &&
	       IS_ALIGNED((long)from, sizeof(u32)) &&
	       IS_ALIGNED((long)size, sizeof(u32));
}

/**
 * @brief copy memory block to io using 32 bit access only
 */
static inline void acm_iowrite32_copy(void __iomem *to, const void *from,
				      size_t size)
{
//	pr_debug("%s(0x%p, 0x%p, %zu)", __func__, to, from, size);
	if (!is_aligned32(to, from, size)) {
		pr_err("%s: wrong alignment: to=%p, from=%p, count=%zu\n",
		       __func__, to, from, size);
		return;
	}
	__iowrite32_copy(to, from, size / sizeof(u32));
}

/**
 * @brief copy io block to memory using 32 bit access only
 */
static inline void acm_ioread32_copy(void *to, const void __iomem *from,
				     size_t size)
{
//	pr_debug("%s(0x%p, 0x%p, %zu)", __func__, to, from, size);
	if (!is_aligned32(to, from, size)) {
		pr_err("%s: wrong alignment: to=%p, from=%p, count=%zu\n",
		       __func__, to, from, size);
		return;
	}
	__ioread32_copy(to, from, size / sizeof(u32));
}

/**
 * @brief fill io block using 32 bit access only
 */
static inline void acm_iowrite32_set(void __iomem *to, u32 val, size_t size)
{
	u32 *at, *till;

	if (!is_aligned32(to, 0, size)) {
		pr_err("%s: wrong alignment: to=%p, count=%zu\n", __func__, to,
		       size);
		return;
	}

	for (at = to, till = to + size; at < till; )
		*at++ = val;
}

#endif /* ACM_ACMIO_H_ */
