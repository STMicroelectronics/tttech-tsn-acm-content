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
 * @file redundancy.h
 * @brief ACM Driver Redundancy Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_REDUN_H_
#define ACM_REDUN_H_

#include <linux/kernel.h>

/**
 * @addtogroup hwaccredund
 * @{
 */

/**
 * @name Redundancy Section Offsets
 * @brief Register offsets of the respective ACM Redundancy Sections
 */
/**@{*/
#define ACM_REDUN_COMMON	0x00000000
#define ACM_REDUN_CTRLTAB_0	0x00000200
#define ACM_REDUN_CTRLTAB_1	0x00000300
#define ACM_REDUN_STATUSTAB	0x00000400
#define ACM_REDUN_INTSEQNNUMTAB	0x00000600
/**@}*/

/**
 * @brief redundancy control table access helper
 */
#define ACM_REDUN_CTRLTAB(i)	\
	(ACM_REDUN_CTRLTAB_0 +	\
	(i) * (ACM_REDUN_CTRLTAB_1 - ACM_REDUN_CTRLTAB_0))

struct acm;

int __must_check redundancy_init(struct acm *acm);
void redundancy_exit(struct acm *acm);
void redundancy_cleanup(struct redundancy *redundancy);
u32 redundancy_area_read(struct redundancy *redundancy, off_t area,
			 off_t offset);
void redundancy_area_write(struct redundancy *redundancy, off_t area,
			   off_t offset, u32 value);
void redundancy_block_read(struct redundancy *redundancy, void *dest,
			   off_t offset, size_t size);
void redundancy_block_write(struct redundancy *redundancy, const void *src,
			    off_t offset, const size_t size);
int __must_check redundancy_get_individual_recovery_timeout(
	struct redundancy *redund, unsigned int module, unsigned int rule,
	u32 *timeout);
int __must_check redundancy_set_individual_recovery_timeout(
	struct redundancy *redund, unsigned int module, unsigned int rule,
	u32 timeout);
int __must_check redundancy_get_base_recovery_timeout(
	struct redundancy *redund, unsigned int idx, u32 *timeout);
int __must_check redundancy_set_base_recovery_timeout(
	struct redundancy *redund, unsigned int idx, u32 timeout);
unsigned int redundancy_get_redund_frames_produced(struct redundancy *redund,
	unsigned int module);
/**@} hwaccredund */

#endif /* ACM_REDUN_H_ */
