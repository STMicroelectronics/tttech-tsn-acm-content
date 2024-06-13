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
 * @file bypass.h
 * @brief ACM Driver Bypass Module
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_BYPASS_H_
#define ACM_BYPASS_H_

#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/device.h>

/**
 * @addtogroup hwaccbypass
 * @{
 */

/**
 * @name Bypass Submodules Offsets
 * @brief Register offsets of the respective ACM Bypass Submodules
 */
/**@{*/
#define ACM_BYPASS_STREAM_IDENT		0x00000000
#define ACM_BYPASS_DMA_CMD_LOOKUP	0x00002000
#define ACM_BYPASS_SCATTER_DMA		0x00003000
#define ACM_BYPASS_GATHER_DMA		0x00005000
#define ACM_BYPASS_CONSTANT_BUFFER	0x00007000
#define ACM_BYPASS_TESTING		0x0000B000
#define ACM_BYPASS_CONTROL_AREA		0x0000C000
#define ACM_BYPASS_STATUS_AREA		0x0000E000
/**@}*/

/**
 * @name Bypass Stream Identification Register Offsets
 * @brief Offsets of the respective ACM Stream Identification register
 */
/**@{*/
#define ACM_BYPASS_STREAM_IDENT_LOOKUP_PATTERN	ACM_BYPASS_STREAM_IDENT
#define ACM_BYPASS_STREAM_IDENT_LAYER7_PATTERN	(ACM_BYPASS_STREAM_IDENT + 0x10)
#define ACM_BYPASS_STREAM_IDENT_LOOKUP_MASK	(ACM_BYPASS_STREAM_IDENT + 0x80)
#define ACM_BYPASS_STREAM_IDENT_LAYER7_MASK	(ACM_BYPASS_STREAM_IDENT + 0x90)
/**@}*/

/**
 * @name Bypass Gather DMA Register Offsets
 * @brief Offsets of the respective ACM gather DMA register
 */
/**@{*/
#define ACM_BYPASS_GATHER_DMA_EXEC	ACM_BYPASS_GATHER_DMA
#define ACM_BYPASS_GATHER_DMA_PREFETCH	(ACM_BYPASS_GATHER_DMA + 0x800)
/**@}*/

/**
 * @name Bypass Control Offsets
 * @brief Offsets of the respective ACM control register
 */
/**@{*/
#define ACM_BYPASS_CONTROL_AREA_NGN_ENABLE		0x00
#define ACM_BYPASS_CONTROL_AREA_LOOKUP_ENABLE		0x04
#define ACM_BYPASS_CONTROL_AREA_LAYER7_ENABLE		0x08
#define ACM_BYPASS_CONTROL_AREA_INGRESS_POLICING_ENABLE	0x0C
#define ACM_BYPASS_CONTROL_AREA_CONNECTION_MODE		0x10
#define ACM_BYPASS_CONTROL_AREA_OUTPUT_DISABLE		0x14
#define ACM_BYPASS_CONTROL_AREA_LAYER7_CHECK_LEN	0x18
#define ACM_BYPASS_CONTROL_AREA_SPEED			0x1C
#define ACM_BYPASS_CONTROL_AREA_STATIC_GATHER_DELAY	0x20
#define ACM_BYPASS_CONTROL_AREA_INGRESS_POLICING_CTRL	0x24
#define ACM_BYPASS_CONTROL_AREA_INDIV_RECOV_ENABLE	0x28
#define ACM_BYPASS_CONTROL_AREA_INDIV_RECOV_TAKE_ANY	0x2C
/**@}*/

/**
 * @name Bypass Status Offsets
 * @brief Offsets of the respective ACM status register
 */
/**@{*/
#define ACM_BYPASS_STATUS_AREA_IFC_VERSION		0x0000
#define ACM_BYPASS_STATUS_AREA_CONFIG_VERSION		0x0004
#define ACM_BYPASS_STATUS_AREA_IP_ERROR			0x0080
#define ACM_BYPASS_STATUS_AREA_HALT_ERROR		0x0084
#define HALT_ON_ERROR					BIT(0)
#define HALT_ON_OTHER					BIT(16)
#define ACM_BYPASS_STATUS_AREA_POLICING_ERROR		0x0088
#define ACM_BYPASS_STATUS_AREA_IP_ERR_FLAGS_LOW		0x0090
#define ACM_BYPASS_STATUS_AREA_IP_ERR_FLAGS_HIGH	0x0094
#define ACM_BYPASS_STATUS_AREA_POLICING_ERR_FLAGS	0x00A0
#define ACM_BYPASS_STATUS_AREA_RX_BYTES			0x0100
#define ACM_BYPASS_STATUS_AREA_RX_FRAMES		0x0104
#define ACM_BYPASS_STATUS_AREA_FCS_ERRORS		0x0108
#define ACM_BYPASS_STATUS_AREA_SIZE_ERRORS		0x010C
#define ACM_BYPASS_STATUS_AREA_FRAME_ERRORS		0x0110
#define FRAME_MII_ERRORS				GENMASK(7, 0)
#define FRAME_SOF_ERRORS				GENMASK(23, 16)
#define FRAME_RUNT_FRAMES				GENMASK(31, 24)
#define ACM_BYPASS_STATUS_AREA_LOOKUP_MATCH_VEC		0x0200
#define ACM_BYPASS_STATUS_AREA_L7_MATCH_VEC		0x0204
#define ACM_BYPASS_STATUS_AREA_LAST_STREAM_TRIGGER	0x0300
#define ACM_BYPASS_STATUS_AREA_INDIV_RECOV_DROPPED_FRAMES 0x0304
#define INDIV_RECOV_DROPPED_FRAMES			GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_L7_MISMATCH_CNT		0x0308
#define ACM_BYPASS_STATUS_AREA_INDIV_RECOV_SEQ_NUM(x)	\
	(0x340 + (x) * sizeof(u32))
#define INDIV_RECOV_SEQ_NUM				GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_RX_FRAMES_CRITICAL	0x0400
#define RX_FRAMES_CRITICAL_PREV				GENMASK(15, 0)
#define RX_FRAMES_CRITICAL_CURR				GENMASK(31, 16)
#define ACM_BYPASS_STATUS_AREA_RX_FRAMES_CRIT_CYC_CHNG	0x0404
#define ACM_BYPASS_STATUS_AREA_INGRESS_WINDOW_STATUS	0x0444
#define ACM_BYPASS_STATUS_AREA_DROPPED_FRAMES		0x0448
#define DROPPED_FRAMES_PREV_CYC				GENMASK(7, 0)
#define DROPPED_FRAMES_CURR_CYC				GENMASK(23, 16)
#define ACM_BYPASS_STATUS_AREA_1_STG_RECOV_EXECSTAT	0x044c
#define ACM_BYPASS_STATUS_AREA_SCHED_TRIG_CNT_PREV	0x0460
#define ACM_BYPASS_STATUS_AREA_SCHED_1STCOND_TRIG_CNT	0x0464
#define ACM_BYPASS_STATUS_AREA_SCHED_2NDCOND_TRIG_CNT	0x0468
#define ACM_BYPASS_STATUS_AREA_PENDING_REQ_MAXNUM	0x0480
#define ACM_BYPASS_STATUS_AREA_SCATTER_DMA_FRAMES	0x0500
#define SCATTER_DMA_FRAMES_PREV				GENMASK(15, 0)
#define SCATTER_DMA_FRAMES_CURR				GENMASK(31, 16)
#define ACM_BYPASS_STATUS_AREA_SCATTER_DMA_BYTES	0x0504
#define SCATTER_DMA_BYTES_PREV				GENMASK(23, 0)
#define SCATTER_DMA_BYTES_CURR				GENMASK(31, 24)

/* for ACM IP up till 1.0.0, i.e. ttt,acm-3.0 */
#define ACM_BYPASS_STATUS_AREA_GATHER_DMA_FRAMES	0x0600
#define GATHER_DMA_FRAMES_PREV				GENMASK(15, 0)
#define GATHER_DMA_FRAMES_CURR				GENMASK(31, 16)

/* for ACM IP since 1.1.0, i.e. ttt,acm-4.0 (diagnostic section) */
#define ACM_BYPASS_STATUS_AREA_RX_FRAMES_COUNTER	0x0600
#define RX_FRAMES_COUNTER				GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_TX_FRAMES_COUNTER	0x0604
#define TX_FRAMES_COUNTER				GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_INGRESS_WIN_CLOSED_FLAGS	0x0610
#define INGRESS_WINDOW_CLOSED_FLAGS_PREV_CYCLES		GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_INGRESS_WIN_CLOSED_COUNTER(i)	\
	(0x614 + sizeof(uint32_t) * (i))
#define INGRESS_WINDOW_CLOSED_COUNTER_PREV_CYCLES	GENMASK(7, 0)
#define ACM_BYPASS_STATUS_AREA_NO_FRAME_RECEIVED_FLAGS	0x0660
#define NO_FRAME_RECEIVED_FLAGS_PREV_CYCLES		GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_NO_FRAME_RECEIVED_COUNTER(i)	\
	(0x0664 + sizeof(uint32_t) * (i))
#define NO_FRAME_RECEIVED_COUNTER_PREV_CYCLES		GENMASK(7, 0)
#define ACM_BYPASS_STATUS_AREA_RECOVERY_FLAGS		0x06B0
#define RECOVERY_FLAGS_PREV_CYCLES			GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_RECOVERY_COUNTER(i)	\
	(0x06B4 + sizeof(uint32_t) * (i))
#define RECOVERY_COUNTER_PREV_CYCLES			GENMASK(7, 0)
#define ACM_BYPASS_STATUS_AREA_ADDITIONAL_FILTER_MISMATCH_FLAGS	0x0700
#define ADDITIONAL_FILTER_MISMATCH_FLAGS_PREV_CYCLES	GENMASK(15, 0)
#define ACM_BYPASS_STATUS_AREA_ADDITIONAL_FILTER_MISMATCH_COUNTER(i)	\
	(0x0704 + sizeof(uint32_t) * (i))
#define ADDITIONAL_FILTER_MISMATCH_COUNTER_PREV_CYCLES	GENMASK(7, 0)
#define ACM_BYPASS_STATUS_AREA_UPDATE_TIMESTAMP_NS	0x0744
#define UPDATE_TIMESTAMP_NS				GENMASK(29, 0)
#define ACM_BYPASS_STATUS_AREA_UPDATE_TIMESTAMP_S	0x0748
#define UPDATE_TIMESTAMP_S				GENMASK(7, 0)
#define ACM_BYPASS_STATUS_AREA_SCHEDULE_CYCLE_COUNTER	0x074C
#define SCHEDULE_CYCLE_COUNTER				GENMASK(7, 0)

#define ACM_BYPASS_STATUS_AREA_TX_BYTES			0x0800
#define TX_BYTES_PREV					GENMASK(23, 0)
#define TX_BYTES_CURR					GENMASK(31, 24)
#define ACM_BYPASS_STATUS_AREA_TX_FRAMES		0x0804
#define TX_FRAMES_PREV					GENMASK(15, 0)
#define TX_FRAMES_CURR					GENMASK(31, 16)
#define ACM_BYPASS_STATUS_AREA_GMII_ERROR		0x0808
#define GMII_ERROR_PREV					GENMASK(7, 0)
#define GMII_ERROR_CURR					GENMASK(23, 16)
#define ACM_BYPASS_STATUS_AREA_DISABLE_OVERRUN		0x0810
#define DISABLE_OVERRUN_PREV				GENMASK(7, 0)
#define DISABLE_OVERRUN_CURR				GENMASK(23, 16)
#define ACM_BYPASS_STATUS_AREA_TX_FRAMES_PREVPREV	0x0820
#define ACM_BYPASS_STATUS_AREA_TX_FRAMES_CYC_CHANGE	0x0824
#define ACM_BYPASS_STATUS_AREA_TX_FRM_CYC_1STCHANGE_LO	0x0840
#define ACM_BYPASS_STATUS_AREA_TX_FRM_CYC_1STCHANGE_HI	0x0844
#define ACM_BYPASS_STATUS_AREA_TX_FRM_CYC_LASTCHANGE_LO	0x0848
#define ACM_BYPASS_STATUS_AREA_TX_FRM_CYC_LASTCHANGE_HI	0x084C
/**@}*/

struct acm;

/**
 * @brief initialize bypass module handling
 */
int __must_check bypass_init(struct acm *acm);
/**
 * @brief exit bypass module handling
 */
void bypass_exit(struct acm *acm);

struct bypass;

void bypass_enable(struct bypass *bypass, bool enable);
bool bypass_is_enabled(struct bypass *bypass);

u32 bypass_area_read(struct bypass *bypass, off_t area, off_t offset);
void bypass_area_write(struct bypass *bypass, off_t area, off_t offset,
		       u32 value);
void bypass_block_read(struct bypass *bypass, void *dest, off_t offset,
		       size_t size);
void bypass_block_write(struct bypass *bypass, const void *src, off_t offset,
			size_t size);
void bypass_set_active(struct bypass *bypass, bool active);
bool bypass_get_active(struct bypass *bypass);

void bypass_cleanup(struct bypass *bypass);

int bypass_diag_read(struct bypass *bypass, struct acmdrv_diagnostics *diag);
int bypass_diag_init(struct bypass *bypass);

unsigned int bypass_get_diag_poll_time(const struct bypass *bypass);
void bypass_set_diag_poll_time(unsigned int poll, struct bypass *bypass);

void bypass_recovery_take_any(struct bypass *bypass, int idx);
bool bypass_recovery_no_frame_received(struct bypass *bypass, int idx);

/**
 * @brief delegate to bypass_area_read() for status area
 */
static inline u32 bypass_status_area_read(struct bypass *bypass, off_t offset)
{
	return bypass_area_read(bypass, ACM_BYPASS_STATUS_AREA, offset);
}

/**
 * @brief delegate to bypass_area_write() for status area
 */
static inline void bypass_status_area_write(struct bypass *bypass, off_t offset,
					    u32 value)
{
	bypass_area_write(bypass, ACM_BYPASS_STATUS_AREA, offset, value);
}

/**
 * @brief delegate to bypass_area_read() for control area
 */
static inline u32 bypass_control_area_read(struct bypass *bypass, off_t offset)
{
	return bypass_area_read(bypass, ACM_BYPASS_CONTROL_AREA, offset);
}

/**
 * @brief delegate to bypass_area_write() for control area
 */
static inline void bypass_control_area_write(struct bypass *bypass,
					     off_t offset,
					     u32 value)
{
	bypass_area_write(bypass, ACM_BYPASS_CONTROL_AREA, offset, value);
}

/**@} hwaccbypass*/

#endif /* ACM_BYPASS_H_ */
