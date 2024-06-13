// SPDX-License-Identifier: GPL-2.0
/*
 * TTTech ACM Linux driver
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
/**
 * @file redundancy.c
 * @brief ACM Driver Redundancy Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macr0
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <asm/div64.h>

#include "acm-module.h"
#include "redundancy.h"
#include "acmio.h"
#include "bypass.h"

/**
 * @addtogroup acmmodparam
 * @{
 */
/**
 * @brief default value for IEEE802.1CB TicksPerSecond
 */
#define ACM_RECOVERY_TICKSPERSEC_DEFAULT	100

/**
 * @brief IEEE802.1CB: number of times per second that the variable
 *        RemainingTicks is decremented
 */
static unsigned int recovery_ticks_per_sec = ACM_RECOVERY_TICKSPERSEC_DEFAULT;
/**@} acmmodparam */

/**
 * @defgroup hwaccredund ACM IP Redundancy
 * @brief Provides access to the redundancy module IP
 *
 * @{
 */
/**
 * @name Redundancy Section Common Register Offsets
 * @brief Register offsets of Redundancy Common Section
 */
/**@{*/
#define REDUND_FRAMES_PRODUCED(x)	(0x0000 + (x) * sizeof(u32))
#define BASE_RECOVERY_TAKE_ANY		0x0008
#define BASE_RECOVERY_ENABLE		0x000C
/**@}*/

/**
 * @struct recovery_data
 * @brief recovery timeout handling data
 *
 * @var recovery_data::frer_seq_rcvy_reset_msec
 * @brief IEEE802.1CB: timeout period in milliseconds for the RECOVERY_TIMEOUT
 *        event
 *
 * @var recovery_data::remaining_ticks
 * @brief IEEE802.1CB: holds the number of clock ticks remaining until a
 *        RECOVERY_TIMEOUT event occurs
 */
struct recovery_data {
	u32 frer_seq_rcvy_reset_msec;
	u32 remaining_ticks;
};

/**
 * @struct base_recovery_data
 * @brief recovery timeout handling data for base recovery
 *
 * @var base_recovery_data::data
 * @brief recovery timeout handling data
 *
 * @var base_recovery_data::intseqnum
 * @brief vale for respective IntSeqNum stored at last poll
 */
struct base_recovery_data {
	struct recovery_data data;
	u16 intseqnum;
};

/**
 * @struct recovery
 * @brief redundancy module recovery data
 *
 * @var recovery::interval
 * @brief nanoseconds interval derived from TicksPerSecond
 *
 * @var recovery::timer
 * @brief TicksPerSecond timer
 *
 * @var recovery::individual
 * @brief individual recovery data
 *
 * @var recovery::base
 * @brief base recovery data
 *
 * @var recovery::lock
 * @brief recovery data lock
 */
struct recovery {
	u64 interval;
	struct hrtimer timer;

	struct recovery_data individual[ACMDRV_BYPASS_NR_RULES]
					[ACMDRV_BYPASS_MODULES_COUNT];
	struct base_recovery_data base[ACMDRV_REDUN_TABLE_ENTRY_COUNT];
	struct mutex lock;
};

/**
 * @brief redundancy module handler instance
 */
struct redundancy {
	void __iomem *base;	/**< base address */
	resource_size_t	size;	/**< module size */

	struct acm *acm;	/**< associated ACM instance */

	struct recovery recovery; /**<  redundancy module recovery data */
};

/**
 * @brief helper to read register in redundancy section
 */
u32 redundancy_area_read(struct redundancy *redundancy, off_t area,
			 off_t offset)
{
	if (area + offset >= redundancy->size) {
		dev_err(acm_dev(redundancy->acm), "%s: access out of range: 0x%08lx\n",
			__func__, area + offset);
		return 0;
	}
	return readl(redundancy->base + area + offset);
}

/**
 * @brief helper to write register in redundancy section
 */
void redundancy_area_write(struct redundancy *redundancy, off_t area,
			   off_t offset, u32 value)
{
	if (area + offset >= redundancy->size) {
		dev_err(acm_dev(redundancy->acm), "%s: access out of range: 0x%08lx\n",
			__func__, area + offset);
		return;
	}
	writel(value, redundancy->base + area + offset);
}

/**
 * @brief helper to read data block from redundancy
 */
void redundancy_block_read(struct redundancy *redundancy, void *dest,
			   off_t offset, size_t size)
{
	u8 bounce[size] __aligned(sizeof(u32));

	if (size + offset >= redundancy->size) {
		dev_err(acm_dev(redundancy->acm), "%s: access out of range: 0x%08lx\n",
			__func__, size + offset);
		return;
	}

	acm_ioread32_copy(bounce, redundancy->base + offset, size);
	memcpy(dest, bounce, size);
}

/**
 * @brief helper to write data block to redundancy
 */
void redundancy_block_write(struct redundancy *redundancy, const void *src,
			    off_t offset, const size_t size)
{
	u8 bounce[size] __aligned(sizeof(u32));

	if (size + offset >= redundancy->size) {
		dev_err(acm_dev(redundancy->acm), "%s: access out of range: 0x%08lx\n",
			__func__, size + offset);
		return;
	}
	memcpy(bounce, src, size);
	acm_iowrite32_copy(redundancy->base + offset, bounce, size);
}

/**
 * @brief redundancy module hardware cleanup/initialization
 */
void redundancy_cleanup(struct redundancy *redundancy)
{
	int i, j;
	const u32 intseqnum[ACMDRV_REDUN_TABLE_ENTRY_COUNT] = { 0 };

	/* reset all redundancy control table entries */
	for (i = 0; i < ACMDRV_REDUN_CTRLTAB_COUNT; ++i)
		for (j = 0; j < ACMDRV_REDUN_TABLE_ENTRY_COUNT; ++j)
			redundancy_area_write(
				redundancy, ACM_REDUN_CTRLTAB(i),
				j * sizeof(struct acmdrv_redun_ctrl_entry),
				0);
	/* reset IntSeqNum table */
	redundancy_block_write(redundancy, intseqnum,
			       ACM_REDUN_INTSEQNNUMTAB, sizeof(intseqnum));
}

/**
 * @brief helper to reset remaining recovery ticks to start value
 *
 * @param data recovery data pointer
 */
static void reset_remaining_ticks(struct recovery_data *data)
{
	data->remaining_ticks = ((data->frer_seq_rcvy_reset_msec
		* recovery_ticks_per_sec) + 999) / 1000;
}

/**
 * @brief Read IntSeqNum from IntSeqNum table
 *
 * @param redund redundancy instance
 * @param idx index within IntSeqNum table
 * @return IntSeqNum value
 */
static u16 read_intseqnum(struct redundancy *redund, int idx)
{
	return redundancy_area_read(redund, ACM_REDUN_INTSEQNNUMTAB,
		idx * sizeof(u32)) & GENMASK(15, 0);
}

/**
 * @brief ReceiveTimeout for base recovery item
 *
 * Set base recovery take any bit of respective index
 *
 * @param redund redundancy instance
 * @param idx index
 */
static void base_recovery_receive_timeout(struct redundancy *redund, unsigned int idx)
{
	u32 take_any;

	dev_dbg(acm_dev(redund->acm),
		"Base recovery: TakeAny for index %d\n", idx);

	take_any = redundancy_area_read(redund, ACM_REDUN_COMMON,
		BASE_RECOVERY_TAKE_ANY);
	take_any |= (1 << idx);
	redundancy_area_write(redund, ACM_REDUN_COMMON, BASE_RECOVERY_TAKE_ANY,
		take_any);
}

/**
 * @brief ReceiveTimeout for individual recovery item
 *
 * Set individual recovery take any bit of respective index
 *
 * @param redund redundancy instance
 * @param module module number
 * @param rule Rule ID
 */
static void individual_recovery_receive_timeout(struct redundancy *redund,
	unsigned int module, unsigned int rule)
{
	struct bypass *bypass = redund->acm->bypass[module];

	bypass_recovery_take_any(bypass, rule);
}

/**
 * @brief Delegate NoFrameReceived flag check to respective bypass
 *
 * @param redund redundancy instance
 * @param module module number
 * @param rule Rule ID
 *
 * @return flag if respective no_frames_received flag has been set
 */
static bool individual_recovery_no_frame_received(struct redundancy *redund,
	unsigned int module, unsigned int rule)
{
	struct bypass *bypass = redund->acm->bypass[module];

	return bypass_recovery_no_frame_received(bypass, rule);
}
/**
 * @brief Read individual recovery timeout of respective rule id and module
 *
 * Recovery lock must be held when calling this function.
 *
 * @param redund redundancy instance
 * @param rule Rule ID
 * @param module module number
 * @return timeout value of the respective individual recovery setting
 */
static u32 get_individual_recovery_timeout(struct redundancy *redund,
			unsigned int module, unsigned int rule)
{
	struct recovery_data *individual =
		&redund->recovery.individual[rule][module];

	return individual->frer_seq_rcvy_reset_msec;
}

/**
 * @brief Set individual recovery timeout of respective rule id and module
 *        and enable individual recovery
 *
 * Recovery lock must be held when calling this function.
 *
 * @param redund redundancy instance
 * @param rule Rule ID
 * @param module module number
 * @param timeout value of the respective individual recovery setting
 */
static void set_individual_recovery_timeout(struct redundancy *redund,
			unsigned int module, unsigned int rule,
			u32 timeout)
{
	u32 rcvy_enable;
	struct recovery_data *individual =
		&redund->recovery.individual[rule][module];
	struct bypass *bypass = redund->acm->bypass[module];

	dev_dbg(acm_dev(redund->acm),
		"Setting individual recovery timeout for module %u, rule  %u to %u ms\n",
		module, rule, timeout);

	/* enable/disable individual recovery automatically */
	rcvy_enable = bypass_area_read(bypass, ACM_BYPASS_CONTROL_AREA,
		ACM_BYPASS_CONTROL_AREA_INDIV_RECOV_ENABLE);
	if (timeout > 0)
		rcvy_enable |= (1 << rule);
	else
		rcvy_enable &= ~(1 << rule);
	bypass_area_write(bypass, ACM_BYPASS_CONTROL_AREA,
		ACM_BYPASS_CONTROL_AREA_INDIV_RECOV_ENABLE, rcvy_enable);

	/* initialize no_frame_received cache (it is clear on read!) */
	(void)bypass_recovery_no_frame_received(bypass, rule);

	individual->frer_seq_rcvy_reset_msec = timeout;
	reset_remaining_ticks(individual);
}

/**
 * @brief Read base recovery timeout of respective index
 *
 * Recovery lock must be held when calling this function.
 *
 * @param redund redundancy instance
 * @param idx index
 * @return timeout value of the respective base recovery setting
 */
static u32 get_base_recovery_timeout(struct redundancy *redund,
	unsigned int idx)
{
	return redund->recovery.base[idx].data.frer_seq_rcvy_reset_msec;
}

/**
 * @brief Set base recovery timeout of respective index and enable base recovery
 *
 * Recovery lock must be held when calling this function.
 *
 * @param redund redundancy instance
 * @param idx index
 * @param timeout value of the respective base recovery setting
 */
static void set_base_recovery_timeout(struct redundancy *redund,
			unsigned int idx, u32 timeout)
{
	u32 rcvy_enable;
	struct base_recovery_data *base = &redund->recovery.base[idx];

	dev_dbg(acm_dev(redund->acm),
		"Setting base recovery timeout for index %u to %u ms\n", idx,
		timeout);

	rcvy_enable = redundancy_area_read(redund, ACM_REDUN_COMMON,
		BASE_RECOVERY_ENABLE);
	if (timeout > 0) {
		rcvy_enable |= (1 << idx);
		/* store actual IntSeqNum for later comparison */
		base->intseqnum = read_intseqnum(redund, idx);
	} else {
		rcvy_enable &= ~(1 << idx);
		base->intseqnum = 0;
	}
	redundancy_area_write(redund, ACM_REDUN_COMMON,
			BASE_RECOVERY_ENABLE, rcvy_enable);

	base->data.frer_seq_rcvy_reset_msec = timeout;
	reset_remaining_ticks(&base->data);
}

/**
 * @brief Read individual recovery timeout of respective rule id and module
 *
 * @param redund redundancy instance
 * @param module module number
 * @param rule Rule ID
 * @param timeout pointer to store value of the respective individual recovery
 *                setting
 * @return 0 on success, negative error code otherwise
 */
int __must_check redundancy_get_individual_recovery_timeout(
	struct redundancy *redund, unsigned int module, unsigned int rule,
	u32 *timeout)
{
	int ret;

	if (!redund || !timeout)
		return -EINVAL;
	if (rule >= ACMDRV_BYPASS_NR_RULES)
		return -EINVAL;
	if (module >= ACMDRV_BYPASS_MODULES_COUNT)
		return -EINVAL;

	ret = mutex_lock_interruptible(&redund->recovery.lock);
	if (ret)
		return ret;
	*timeout = get_individual_recovery_timeout(redund, module, rule);
	mutex_unlock(&redund->recovery.lock);

	return 0;
}

/**
 * @brief Set individual recovery timeout of respective rule id and module
 *        and enable individual recovery
 *
 * @param redund redundancy instance
 * @param module module number
 * @param rule Rule ID
 * @param timeout value of the respective individual recovery setting
 * @return 0 on success, negative error code otherwise
 */
int __must_check redundancy_set_individual_recovery_timeout(
	struct redundancy *redund, unsigned int module, unsigned int rule,
	u32 timeout)
{
	int ret;

	if (!redund)
		return -EINVAL;
	if (rule >= ACMDRV_BYPASS_NR_RULES)
		return -EINVAL;
	if (module >= ACMDRV_BYPASS_MODULES_COUNT)
		return -EINVAL;

	ret = mutex_lock_interruptible(&redund->recovery.lock);
	if (ret)
		return ret;
	set_individual_recovery_timeout(redund, module, rule, timeout);
	mutex_unlock(&redund->recovery.lock);

	return 0;
}

/**
 * @brief Read base recovery timeout of respective index
 *
 * @param redund redundancy instance
 * @param idx index
 * @param timeout pointer to store value of the respective base recovery
 *                setting
 * @return 0 on success, negative error code otherwise
 */
int __must_check redundancy_get_base_recovery_timeout(struct redundancy *redund,
	unsigned int idx, u32 *timeout)
{
	int ret;

	if (!redund || !timeout)
		return -EINVAL;
	if (idx >= ACMDRV_REDUN_TABLE_ENTRY_COUNT)
		return -EINVAL;

	ret = mutex_lock_interruptible(&redund->recovery.lock);
	if (ret)
		return ret;
	*timeout = get_base_recovery_timeout(redund, idx);
	mutex_unlock(&redund->recovery.lock);

	return 0;
}

/**
 * @brief Set base recovery timeout of respective index and enable base recovery
 *
 * @param redund redundancy instance
 * @param idx index
 * @param timeout value of the respective base recovery setting
 * @return 0 on success, negative error code otherwise
 */
int __must_check redundancy_set_base_recovery_timeout(struct redundancy *redund,
			unsigned int idx, u32 timeout)
{
	int ret;

	if (!redund)
		return -EINVAL;
	if (idx >= ACMDRV_REDUN_TABLE_ENTRY_COUNT)
		return -EINVAL;

	ret = mutex_lock_interruptible(&redund->recovery.lock);
	if (ret)
		return ret;
	set_base_recovery_timeout(redund, idx, timeout);
	mutex_unlock(&redund->recovery.lock);

	return 0;
}

/**
 * @brief Read REDUND_FRAMES_PRODUCED registers
 */
unsigned int redundancy_get_redund_frames_produced(struct redundancy *redund,
	unsigned int modidx)
{
	return redundancy_area_read(redund, ACM_REDUN_COMMON,
		REDUND_FRAMES_PRODUCED(modidx));
}

/**
 * @brief Start/stop recovery timer
 *
 * 0 ticks per second stop the timer, all other values start it accordingly.
 *
 * @param recovery instance of recovery object
 * @param ticks_per_second number of ticks per second for recovery tick
 */
static void set_recovery_tick(struct recovery *recovery,
	unsigned int ticks_per_second)
{
	struct acm *acm;
	acm = container_of(recovery, struct redundancy, recovery)->acm;

	/* stop timer */
	if (ticks_per_second == 0) {
		recovery->interval = 0;
		hrtimer_cancel(&recovery->timer);
		return;
	}

	recovery->interval = NSEC_PER_SEC;
	do_div(recovery->interval, ticks_per_second);

	dev_dbg(acm_dev(acm), "Setting recovery timer interval to %lld ns\n",
		recovery->interval);

	hrtimer_start_range_ns(&recovery->timer,
		ns_to_ktime(recovery->interval), 50000,
		HRTIMER_MODE_REL);

}

/**
 * @brief check for receive timeout for single individual recovery
 *
 * @param redund redundancy instance
 * @param module bypass module index
 * @param rule rule index within the bypass module
 */
static void individual_recovery_process(struct redundancy *redund,
	unsigned int module, unsigned int rule)
{
	struct recovery_data *individual;

	individual = &redund->recovery.individual[rule][module];

	if (individual->frer_seq_rcvy_reset_msec == 0)
		return;

	/* check, if no frame has been received */
	if (individual_recovery_no_frame_received(redund, module, rule)) {
		individual->remaining_ticks--;
		if (individual->remaining_ticks > 0)
			return;
		individual_recovery_receive_timeout(redund, module, rule);
	}

	reset_remaining_ticks(individual);
}


/**
 * @brief check for timeout of single base recovery
 *
 * @param redund redundancy instance
 * @param idx IntSeqNum table index
 */
static void base_recovery_process(struct redundancy *redund,
	unsigned int idx)
{
	struct base_recovery_data *base = &redund->recovery.base[idx];
	u16 intseqnum;

	if (base->data.frer_seq_rcvy_reset_msec == 0)
		return;

	intseqnum = read_intseqnum(redund, idx);
	if (intseqnum == base->intseqnum) {
		base->data.remaining_ticks--;
		if (base->data.remaining_ticks > 0)
			return;
		base_recovery_receive_timeout(redund, idx);
	}
	base->intseqnum = intseqnum;
	reset_remaining_ticks(&base->data);
}

/**
 * @brief recovery tick data processing
 *
 * Recovery lock must be held when calling this function.
 *
 * @param recovery recovery data of redundancy module
 */
static void recovery_process(struct recovery *recovery)
{
	unsigned int module;
	unsigned int idx;

	struct redundancy *redund = container_of(recovery, struct redundancy,
		recovery);

	for (module = 0; module < ACMDRV_BYPASS_MODULES_COUNT; ++module)
		for (idx = 0; idx < ACMDRV_BYPASS_NR_RULES; ++idx)
			individual_recovery_process(redund, module, idx);

	for (idx = 0; idx < ACMDRV_REDUN_TABLE_ENTRY_COUNT; ++idx)
		base_recovery_process(redund, idx);
}

/**
 * @brief recovery timer function
 *
 * @param t hrtimer associated with recovery
 * @return HRTIMER_RESTART if recovery tick <> 0, else HRTIMER_NORESTART
 */
static enum hrtimer_restart recovery_timer(struct hrtimer *t)
{
	enum hrtimer_restart ret = HRTIMER_NORESTART;
	struct recovery *recovery = container_of(t, struct recovery, timer);

	mutex_lock(&recovery->lock);

	if (recovery->interval > 0) {
		recovery_process(recovery);
		hrtimer_forward_now(t, ns_to_ktime(recovery->interval));

		ret = HRTIMER_RESTART;
	}

	mutex_unlock(&recovery->lock);
	return ret;
}

/**
 * @brief recovery sub-module initialization
 */
static void recovery_init(struct redundancy *redund)
{
	int module, idx;
	struct recovery *recovery = &redund->recovery;

	mutex_init(&recovery->lock);

	mutex_lock(&recovery->lock);

	for (module = 0; module < ACMDRV_BYPASS_MODULES_COUNT; ++module) {
		for (idx = 0; idx < ACMDRV_BYPASS_NR_RULES; ++idx)
			set_individual_recovery_timeout(redund, module, idx, 0);
	}
	for (idx = 0; idx < ACMDRV_REDUN_TABLE_ENTRY_COUNT; ++idx)
		set_base_recovery_timeout(redund, idx, 0);

	mutex_unlock(&recovery->lock);

	/* start recovery tick */
	hrtimer_init(&recovery->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	recovery->timer.function = recovery_timer;
	set_recovery_tick(recovery, recovery_ticks_per_sec);
}

/**
 * @brief recovery sub-module exit
 */
static void recovery_exit(struct redundancy *redund)
{
	int module, idx;
	struct recovery *recovery = &redund->recovery;

	/* stop recovery tick */
	set_recovery_tick(recovery, 0);

	mutex_lock(&recovery->lock);

	for (module = 0; module < ACMDRV_BYPASS_MODULES_COUNT; ++module) {
		for (idx = 0; idx < ACMDRV_BYPASS_NR_RULES; ++idx)
			set_individual_recovery_timeout(redund, module, idx, 0);
	}
	for (idx = 0; idx < ACMDRV_REDUN_TABLE_ENTRY_COUNT; ++idx)
		set_base_recovery_timeout(redund, idx, 0);

	mutex_unlock(&recovery->lock);
}

/**
 * @brief redundancy module handler initialization
 */
int __must_check redundancy_init(struct acm *acm)
{
	struct resource *res;
	struct redundancy *redundancy;
	struct platform_device *pdev = acm->pdev;
	struct device *dev = &pdev->dev;

	redundancy = devm_kzalloc(dev, sizeof(*redundancy), GFP_KERNEL);
	if (!redundancy)
		return -ENOMEM;
	redundancy->acm = acm;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "Redundancy");
	redundancy->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(redundancy->base))
		return PTR_ERR(redundancy->base);
	redundancy->size = resource_size(res);
	recovery_init(redundancy);
	dev_dbg(dev, "Probed Redundancy %pr -> 0x%p\n", res, redundancy->base);

	acm->redundancy = redundancy;
	pr_err("%s: redundancy object at 0x%p\n", __func__, acm->redundancy);
	return 0;
}

/**
 * @brief redundancy module handler exit
 */
void redundancy_exit(struct acm *acm)
{
	pr_err("%s: redundancy object at 0x%p\n", __func__, acm->redundancy);
	recovery_exit(acm->redundancy);
}
/**@} hwaccredund */

/**
 * @addtogroup acmmodparam
 * @{
 */
/**
 * @brief Linux module parameter definition for IEEE802.1CB TicksPerSecond
 */
module_param(recovery_ticks_per_sec, uint, 0644);

/**
 * @brief Linux module parameter description
 */
MODULE_PARM_DESC(recovery_ticks_per_sec, "IEEE802.1CB TicksPerSecond");

/**@} acmmodparam */

