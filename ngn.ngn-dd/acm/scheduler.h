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
 * @file scheduler.h
 * @brief ACM Driver Scheduler Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_SCHEDULER_H_
#define ACM_SCHEDULER_H_

/**
 * @addtogroup hwaccsched ACM IP Scheduler
 * @{
 */
#include <linux/kernel.h>
#include <linux/ktime.h>

/**
 * @name Scheduler IP Section Offsets
 * @brief Register offsets of the respective ACM Scheduler Sections
 */
/**@{*/
#define ACM_SCHEDULER_COMMON	0x00000000
#define ACM_SCHEDULER_GENERICS	0x00004000
#define ACM_SCHEDULER_SCHED_0	0x00010000
#define ACM_SCHEDULER_SCHED_1	0x00011000
/**@}*/

/**
 * @brief access scheduler per index
 */
#define ACM_SCHEDULER_SCHED(i)		\
	(ACM_SCHEDULER_SCHED_0 + (i) *	\
		(ACM_SCHEDULER_SCHED_1 - ACM_SCHEDULER_SCHED_0))

/**
 * @brief helper for easy scheduler common access
 */
#define SCHED_COMMON(s, name) ((s)->base + ACM_SCHEDULER_COMMON + \
	ACM_SCHEDULER_COMMON_##name)

/**
 * @brief helper for easy scheduler generic register access
 */
#define SCHED_GENERICS(s, name) ((s)->base + ACM_SCHEDULER_GENERICS + \
	ACM_SCHEDULER_GENERICS_##name)

/**
 * @name Scheduler IP Common Register Offsets
 * @brief Register offsets and definitions for Scheduler Common Registers
 */
/**@{*/
#define ACM_SCHEDULER_COMMON_DEV_ID0			0x0000
#define ACM_SCHEDULER_COMMON_DEV_ID1			0x0002
#define ACM_SCHEDULER_COMMON_INT_ID0			0x0004
#define ACM_SCHEDULER_COMMON_INT_ID1			0x0006

#define ACM_SCHEDULER_COMMON_ROW_ACCESS_CMD0		0x1000
#define CMD0_SCHEDULER					GENMASK(2, 0)
#define CMD0_TABLE					GENMASK(8, 8)
#define CMD0_ACCESS_ERROR				GENMASK(13, 13)
#define CMD0_WRITE					GENMASK(14, 14)
#define CMD0_TRANSFER					GENMASK(15, 15)

#define ACM_SCHEDULER_COMMON_ROW_ACCESS_CMD1		0x1002
#define CMD1_ROW_NUMBER					GENMASK(9, 0)

#define ACM_SCHEDULER_COMMON_ROW_ACCESS_DATA0		0x1010
#define ACM_SCHEDULER_COMMON_ROW_ACCESS_DATA1		0x1012
#define ACM_SCHEDULER_COMMON_ROW_ACCESS_DATA2		0x1014
#define ACM_SCHEDULER_COMMON_ROW_ACCESS_DATA3		0x1016
#define ACM_SCHEDULER_COMMON_ROW_ACCESS_DATA4		0x1018
#define ACM_SCHEDULER_COMMON_ROW_ACCESS_INT_MASK	0x1100
#define ACM_SCHEDULER_COMMON_ROW_ACCESS_INT_STTAUS	0x1102
/**@}*/

/**
 * @name Scheduler IP Generics Register Offsets
 * @brief Register offsets and definitions for Scheduler Common Registers
 */
/**@{*/
#define ACM_SCHEDULER_GENERICS_SCHEDULERS	0x0000
#define ACM_SCHEDULER_GENERICS_OUTPUTS		0x0002
#define ACM_SCHEDULER_GENERICS_TABLE_ROWS	0x0004
#define ACM_SCHEDULER_GENERICS_CLK_FREQ		0x0006
#define ACM_SCHEDULER_GENERICS_SCHEDULERS	0x0000
/**@}*/

/**
 * @name Scheduler IP Scheduler Register Offsets
 * @brief Register offsets and definitions for Scheduler Registers
 */
/**@{*/
#define ACM_SCHEDULER_SCHED_CTRL	0x000
#define ACM_SCHEDULER_SCHED_TAB_0	0x800
#define ACM_SCHEDULER_SCHED_TAB_1	0x900
#define ACM_SCHEDULER_SCHED_TAB(i)		\
	(ACM_SCHEDULER_SCHED_TAB_0 + (i) *	\
		(ACM_SCHEDULER_SCHED_TAB_1 - ACM_SCHEDULER_SCHED_TAB_0))

#define SCHED_TABLE(t, name) ((t)->base + ACM_SCHEDULER_SCHED_TAB_##name)

#define ACM_SCHEDULER_SCHED_CTRL_SCH_GEN		0x00
#define ACM_SCHEDULER_SCHED_CTRL_DWNCNTR_SPD		0x02
#define DWNCNTR_SPD_200MHZ				(0x4 << 6)
#define DWNCNTR_SPD_125MHZ				(0x4 << 6)
#define DWNCNTR_SPD_100MHZ				(0x5 << 6)
#define ACM_SCHEDULER_SCHED_CTRL_EME_DIS_CTRL		0x20
#define EME_DIS_MUX					BIT(0)
#define ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(i)	(0x30 + (i) * 2)

#define ACM_SCHEDULER_SCHED_TAB_TBL_GEN			0x00
#define TBL_GEN_CAN_BE_TAKEN				BIT(0)
#define TBL_GEN_IN_USE					BIT(1)
#define TBL_GEN_LAST_CYC_NR_EN				BIT(8)
#define TBL_GEN_LAST_CYC_REACHED			BIT(9)
#define TBL_GEN_UPDATE					BIT(15)

#define ACM_SCHEDULER_SCHED_TAB_STARTTIME_NS_LOW	0x14
#define ACM_SCHEDULER_SCHED_TAB_STARTTIME_NS_HIGH	0x16
#define ACM_SCHEDULER_SCHED_TAB_STARTTIME_SEC		0x18
#define ACM_SCHEDULER_SCHED_TAB_CYCLETIME_SUBNS_LOW	0x20
#define ACM_SCHEDULER_SCHED_TAB_CYCLETIME_SUBNS_HIGH	0x22
#define ACM_SCHEDULER_SCHED_TAB_CYCLETIME_NS_LOW	0x24
#define ACM_SCHEDULER_SCHED_TAB_CYCLETIME_NS_HIGH	0x26
#define ACM_SCHEDULER_SCHED_TAB_CYCLE_TS_NS_LOW		0x34
#define ACM_SCHEDULER_SCHED_TAB_CYCLE_TS_NS_HIGH	0x36
#define ACM_SCHEDULER_SCHED_TAB_CYCLE_TS_SEC		0x38
#define ACM_SCHEDULER_SCHED_TAB_CYCLE_CNT		0x40
#define ACM_SCHEDULER_SCHED_TAB_LAST_CYCLE		0x44
/**@}*/

struct acm;
int __must_check scheduler_init(struct acm *acm);
void scheduler_exit(struct acm *acm);

u16 scheduler_read_sched_down_counter(struct scheduler *scheduler,
				      int sched_id);
void scheduler_write_sched_down_counter(struct scheduler *scheduler,
					int sched_id, u16 cnt);

int __must_check scheduler_read_table_row(struct scheduler *scheduler,
	int sched_id, int tab_id, int row_id,
	struct acmdrv_sched_tbl_row *row);
int __must_check scheduler_write_table_row(struct scheduler *scheduler,
	int sched_id, int tab_id, int row_id,
	const struct acmdrv_sched_tbl_row *row);

u16 scheduler_read_table_status(struct scheduler *scheduler, int sched_id,
				int tab_id);

void scheduler_read_cycle_time(struct scheduler *scheduler, int sched_id,
		int tab_id, struct acmdrv_sched_cycle_time *time);
void scheduler_write_cycle_time(struct scheduler *scheduler, int sched_id,
		int tab_id, const struct acmdrv_sched_cycle_time *time);

int scheduler_read_start_time(struct scheduler *scheduler, int sched_id,
		int tab_id, struct timespec64 *time);
int scheduler_write_start_time(struct scheduler *scheduler, int sched_id,
		int tab_id, const struct timespec64 *time,
		bool start);

u16 scheduler_read_emergency_disable(struct scheduler *scheduler, int sched_id);
void scheduler_write_emergency_disable(struct scheduler *scheduler,
				       int sched_id, u16 emerg_disable);

void scheduler_set_active(struct scheduler *scheduler, int i, bool active);
bool scheduler_get_active(struct scheduler *scheduler, int i);

void scheduler_cleanup(struct scheduler *scheduler);

ktime_t scheduler_ktime_get_ptp(struct scheduler *scheduler);

/**@} hwaccsched */

#endif /* ACM_SCHED_H_ */
