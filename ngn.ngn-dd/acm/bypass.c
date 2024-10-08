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
 * @file bypass.c
 * @brief ACM Driver Bypass Module
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup hwaccbypass ACM IP Bypass Module
 * @brief Provides access to the bypass module IP
 *
 * @{
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_mdio.h>
#include <linux/netdevice.h>
#include <asm/unaligned.h>
#include <linux/mutex.h>

#include "acm-module.h"
#include "bypass.h"
#include "acmio.h"
#include "acmbitops.h"
#include "edge.h"
#include "scheduler.h"

/**@} hwaccbypass */

/**
 * @addtogroup acmmodparam
 * @{
 */
/**
 * @brief maximum retry for getting diagnostic data
 */
static unsigned int max_diag_retry = 3;

/**
 * @brief control, if diagnostics are cleared on read
 */
static bool clear_on_read = true;

/**
 * @brief diagnostics poll cycle in ms
 */
static unsigned int diag_poll = 50;

/**@} acmmodparam */

/**
 * @addtogroup hwaccbypass
 * @{
 */

/**
 * @brief special cache for no_frames_received data
 */
struct nfr_cache {
	u32 flags; /**< accumulated register value */
	struct mutex lock; /**< cache access lock */
};
/**
 * @brief Bypass Module Handler
 */
struct bypass {
	void __iomem		*base;		/**< base address */
	resource_size_t		size;		/**< module size */
	struct acm		*acm;		/**< associated ACM instance */
	int			index;		/**< module index */
	struct device_node	*phy_node;	/**< associated phy node */
	struct phy_device	*phy_dev;	/**< associated phy device */
	struct acmdrv_diagnostics diag;		/**< module diagnostics data */
	struct mutex		diag_lock;	/**< diag data access lock */
	struct delayed_work	diag_work;	/**< diag data polling work */
	unsigned int		diag_poll_time;	/**< diag data poll time (ms) */
	bool			active;	/**< denotes bypass module as active */

	struct nfr_cache	no_frames_received; /**< recovery cache data */
	struct mutex		take_any_lock; /**< take_any access lock */
};

/**
 * @brief read bypass module register of specified area and offset
 */
u32 bypass_area_read(struct bypass *bypass, off_t area, off_t offset)
{
	u32 val;

	if (area + offset >= bypass->size) {
		dev_err(acm_dev(bypass->acm), "%s: access out of range: 0x%08lx, bybass size: 0x%pa\n",
			__func__, area + offset, &bypass->size);
		return 0;
	}

	val = readl(bypass->base + area + offset);

//	dev_dbg(acm_dev(bypass->acm), "[BP%d]: %s(0x%08lx, 0x%08lx) -> 0x%08x\n",
//		bypass->index, __func__, area, offset, val);

	return val;
}

/**
 * @brief write bypass module register of specified area and offset
 */
void bypass_area_write(struct bypass *bypass, off_t area, off_t offset,
			      u32 value)
{
	if (area + offset >= bypass->size) {
		dev_err(acm_dev(bypass->acm), "%s: access out of range: 0x%08lx, bybass size: 0x%pa\n",
			__func__, area + offset, &bypass->size);
		return;
	}

//	dev_dbg(acm_dev(bypass->acm), "[BP%d]: 0x%08x -> %s(0x%08lx, 0x%08lx)\n",
//		bypass->index, value, __func__, area, offset);

	writel(value, bypass->base + area + offset);
}

/**
 * @brief read bypass module data block without bounce buffer
 *
 * Remark: dest must be word aligned!
 */
static int bypass_block_read_direct(struct bypass *bypass, void *dest,
				     off_t offset, size_t size)
{
	if (offset + size >= bypass->size) {
		dev_err(acm_dev(bypass->acm), "%s: access out of range: 0x%08lx\n",
			__func__, offset + size);
		return -EPERM;
	}

	acm_ioread32_copy(dest, bypass->base + offset, size);
	return 0;
}


/**
 * @brief read bypass module data block
 */
void bypass_block_read(struct bypass *bypass, void *dest, off_t offset,
		       size_t size)
{
	int ret;
	u8 bounce[size] __aligned(sizeof(u32));

//	dev_dbg(acm_dev(bypass->acm), "[BP%d]: %s(0x%p, 0x%08lx, 0x%08zx)\n",
//		bypass->index, __func__, dest, offset, size);

	ret = bypass_block_read_direct(bypass, bounce, offset, size);
	if (ret == 0)
		memcpy(dest, bounce, size);
}

/**
 * @brief write bypass module data block
 */
void bypass_block_write(struct bypass *bypass, const void *src, off_t offset,
			const size_t size)
{
	u8 bounce[size] __aligned(sizeof(u32));

//	dev_dbg(acm_dev(bypass->acm), "[BP%d]: %s(0x%p, 0x%08lx, 0x%08zx)\n",
//		bypass->index, __func__, src, offset, size);

	if (offset + size >= bypass->size) {
		dev_err(acm_dev(bypass->acm), "%s: access out of range: 0x%08lx\n",
			__func__, offset + size);
		return;
	}

	memcpy(bounce, src, size);
	acm_iowrite32_copy(bypass->base + offset, bounce, size);
}

/**
 * @brief enable bypass module
 */
void bypass_enable(struct bypass *bypass, bool enable)
{
	bypass_control_area_write(bypass, ACM_BYPASS_CONTROL_AREA_NGN_ENABLE,
				  acmdrv_bypass_ctrl_enable_create(enable));
}

/**
 * @brief check if bypass module is enabled
 */
bool bypass_is_enabled(struct bypass *bypass)
{
	return bypass_control_area_read(bypass,
		ACM_BYPASS_CONTROL_AREA_NGN_ENABLE) != 0;
}

/**
 * @brief mark bypass module as active/inactive
 */
void bypass_set_active(struct bypass *bypass, bool active)
{
	bypass->active = active;
}

/**
 * @brief check if bypass module is active
 */
bool bypass_get_active(struct bypass *bypass)
{
	return bypass->active;
}

/**
 * @struct bypass_diag_access_helper
 * @brief helper struct to access diagnostic data uniformly
 *
 * @var bypass_diag_access_helper::regoffs
 * @brief status register offset
 *
 * @var bypass_diag_access_helper::mask
 * @brief data value mask
 *
 * @var bypass_diag_access_helper::dataoffs
 * @brief offset in diagnostic acmdrv_diagnostics
 *
 * @var bypass_diag_access_helper::read
 * @brief HW data access function
 *
 * @var bypass_diag_access_helper::count
 * @brief number of elements to read
 */
struct bypass_diag_access_helper {
	off_t regoffs;
	uint32_t mask;
	off_t dataoffs;
	void (*read)(struct bypass *bypass,
		     const struct bypass_diag_access_helper *acc);
	uint32_t count;
};

/**
 * @brief Acculmulate diagnostic counter data
 *
 * Maximum counter values indicate a (possible) overflow. Propagate any
 * overflow to the maximum accumulated value (i.e. 0xFFFFFFFF), so 0xFFFFFFFF
 * indicates an overflow.
 */
static void bypass_diag_acculmulate(struct bypass *bypass,
	const struct bypass_diag_access_helper *acc)
{
	int i;
	uint8_t *diagdata = (void *)(&bypass->diag);
	uint32_t *data = (void *)(&diagdata[acc->dataoffs]);
	uint32_t dest[acc->count];

	bypass_block_read_direct(bypass, dest,
				 ACM_BYPASS_STATUS_AREA + acc->regoffs,
				 acc->count * sizeof(uint32_t));

	for (i = 0; i < acc->count; ++i, data++) {
		uint32_t val;

		/* if data already overflowed, do nothing */
		if (*data == 0xFFFFFFFF)
			continue;

		val = (dest[i] & acc->mask) >> (ffs(acc->mask) - 1);

		/* counter overflow, i.e. max value reached? */
		if (val == (acc->mask >> (ffs(acc->mask) - 1))) {
			*data = 0xFFFFFFFF;
			continue;
		}

		/* check for overflow */
		if (*data + val < *data) {
			*data = 0xFFFFFFFF;
			continue;
		}

		*data += val;
	}
}

/**
 * @brief Logically or diagnostic flag data
 */
static void bypass_diag_or(struct bypass *bypass,
			   const struct bypass_diag_access_helper *acc)
{
	uint32_t raw;
	uint8_t *diagdata = (uint8_t *)(&bypass->diag);
	uint32_t *data = (uint32_t *)(&diagdata[acc->dataoffs]);

	raw = bypass_status_area_read(bypass, acc->regoffs);
	*data |= raw & acc->mask;
}

/**
 * @brief no_frames_received flags for diagnostic and recovery
 */
static void bypass_diag_or_no_frames_received(struct bypass *bypass,
	const struct bypass_diag_access_helper *acc)
{
	uint32_t raw;
	uint8_t *diagdata = (uint8_t *)(&bypass->diag);
	uint32_t *data = (uint32_t *)(&diagdata[acc->dataoffs]);

	mutex_lock(&bypass->no_frames_received.lock);

	raw = bypass_status_area_read(bypass, acc->regoffs);
	*data |= raw & acc->mask;
	bypass->no_frames_received.flags |= raw & acc->mask;

	mutex_unlock(&bypass->no_frames_received.lock);
}

/**
 * @brief Enhance timestamp to PTP time
 */
static void bypass_fixup_diag_timestamp(struct bypass *bypass)
{
	struct timespec64 now;
	int64_t sec = bypass->diag.timestamp.tv_sec;

	now = ktime_to_timespec64(
		scheduler_ktime_get_ptp(bypass->acm->scheduler));

	bypass->diag.timestamp.tv_sec += now.tv_sec & ~UPDATE_TIMESTAMP_S;
	if ((bypass->diag.timestamp.tv_sec & UPDATE_TIMESTAMP_S) < sec)
		bypass->diag.timestamp.tv_sec -= UPDATE_TIMESTAMP_S + 1;
}

/**
 * @brief Read diagnostic data timestamp
 */
static void bypass_diag_timestamp(struct bypass *bypass,
	const struct bypass_diag_access_helper *acc)
{
	uint8_t *diagdata = (void *)(&bypass->diag);
	struct acmdrv_timespec64 *time = (void *)(&diagdata[acc->dataoffs]);
	uint32_t oldsec;

	do {
		oldsec = bypass_status_area_read(bypass,
			ACM_BYPASS_STATUS_AREA_UPDATE_TIMESTAMP_S);

		time->tv_nsec = bypass_status_area_read(bypass,
			ACM_BYPASS_STATUS_AREA_UPDATE_TIMESTAMP_NS);
		time->tv_sec = bypass_status_area_read(bypass,
			ACM_BYPASS_STATUS_AREA_UPDATE_TIMESTAMP_S);
	} while (oldsec != time->tv_sec);

	bypass_fixup_diag_timestamp(bypass);
}

/**
 * @brief index enumeration for diagnostic elements
 */
enum diag_index {
	SCHEDULE_CYCLE_COUNTER_DIDX = 0,
	UPDATE_TIMESTAMP_NS_DIDX,
	RX_FRAMES_COUNTER_DIDX,
	TX_FRAMES_COUNTER_DIDX,
	INGRESS_WIN_CLOSED_FLAGS_DIDX,
	INGRESS_WIN_CLOSED_COUNTER_DIDX,
	NO_FRAME_RECEIVED_FLAGS_DIDX,
	NO_FRAME_RECEIVED_COUNTER_DIDX,
	RECOVERY_FLAGS_DIDX,
	RECOVERY_COUNTER_DIDX,
	ADDITIONAL_FILTER_MISMATCH_FLAGS_DIDX,
	ADDITIONAL_FILTER_MISMATCH_COUNTER_DIDX,

	MAX_DIDX
};

/**
 * @brief Table for uniform diagnostic data access
 */
static const struct bypass_diag_access_helper bypass_diag_access[MAX_DIDX] = {
	[SCHEDULE_CYCLE_COUNTER_DIDX] = {
		/* read cycle counter first! */
		.regoffs = ACM_BYPASS_STATUS_AREA_SCHEDULE_CYCLE_COUNTER,
		.mask = SCHEDULE_CYCLE_COUNTER,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			scheduleCycleCounter),
		.read = bypass_diag_acculmulate,
		.count = 1
	},
	[UPDATE_TIMESTAMP_NS_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_UPDATE_TIMESTAMP_NS,
		.mask = 0xFFFFFFFF,
		.dataoffs = offsetof(struct acmdrv_diagnostics, timestamp),
		.read = bypass_diag_timestamp,
		.count = 1
	},
	[RX_FRAMES_COUNTER_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_RX_FRAMES_COUNTER,
		.mask = RX_FRAMES_COUNTER,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			rxFramesCounter),
		.read = bypass_diag_acculmulate,
		.count = 1
	},
	[TX_FRAMES_COUNTER_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_TX_FRAMES_COUNTER,
		.mask = TX_FRAMES_COUNTER,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			txFramesCounter),
		.read = bypass_diag_acculmulate,
		.count = 1
	},
	[INGRESS_WIN_CLOSED_FLAGS_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_INGRESS_WIN_CLOSED_FLAGS,
		.mask = INGRESS_WINDOW_CLOSED_FLAGS_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			ingressWindowClosedFlags),
		.read = bypass_diag_or,
		.count = 1
	},
	[INGRESS_WIN_CLOSED_COUNTER_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_INGRESS_WIN_CLOSED_COUNTER(0),
		.mask = INGRESS_WINDOW_CLOSED_COUNTER_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			ingressWindowClosedCounter[0]),
		.read = bypass_diag_acculmulate,
		.count = ACMDRV_BYPASS_NR_RULES
	},
	[NO_FRAME_RECEIVED_FLAGS_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_NO_FRAME_RECEIVED_FLAGS,
		.mask = NO_FRAME_RECEIVED_FLAGS_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			noFrameReceivedFlags),
		.read = bypass_diag_or_no_frames_received,
		.count = 1
	},
	[NO_FRAME_RECEIVED_COUNTER_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_NO_FRAME_RECEIVED_COUNTER(0),
		.mask = NO_FRAME_RECEIVED_COUNTER_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			noFrameReceivedCounter[0]),
		.read = bypass_diag_acculmulate,
		.count = ACMDRV_BYPASS_NR_RULES
	},
	[RECOVERY_FLAGS_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_RECOVERY_FLAGS,
		.mask = RECOVERY_FLAGS_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			recoveryFlags),
		.read = bypass_diag_or,
		.count = 1
	},
	[RECOVERY_COUNTER_DIDX] = {
		.regoffs = ACM_BYPASS_STATUS_AREA_RECOVERY_COUNTER(0),
		.mask = RECOVERY_COUNTER_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			recoveryCounter[0]),
		.read = bypass_diag_acculmulate,
		.count = ACMDRV_BYPASS_NR_RULES
	},
	[ADDITIONAL_FILTER_MISMATCH_FLAGS_DIDX] = {
		.regoffs =
			ACM_BYPASS_STATUS_AREA_ADDITIONAL_FILTER_MISMATCH_FLAGS,
		.mask = ADDITIONAL_FILTER_MISMATCH_FLAGS_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			additionalFilterMismatchFlags),
		.read = bypass_diag_or,
		.count = 1
	},
	[ADDITIONAL_FILTER_MISMATCH_COUNTER_DIDX] = {
		.regoffs =
		ACM_BYPASS_STATUS_AREA_ADDITIONAL_FILTER_MISMATCH_COUNTER(0),
		.mask = ADDITIONAL_FILTER_MISMATCH_COUNTER_PREV_CYCLES,
		.dataoffs = offsetof(struct acmdrv_diagnostics,
			additionalFilterMismatchCounter[0]),
		.read = bypass_diag_acculmulate,
		.count = ACMDRV_BYPASS_NR_RULES
	}
};

/**
 * @brief update diagnostic data cache
 *
 * The diag_lock must be held when calling this function
 */
static int bypass_diag_update(struct bypass *bypass)
{
	int ret = -ETIMEDOUT;
	int retry;
	u64 start, end;

	start = ktime_get_mono_fast_ns();
	for (retry = 0; retry < max_diag_retry; ++retry) {
		int i;
		uint32_t counter;

		/* read each diagnostic item */
		for (i = 0; i < ARRAY_SIZE(bypass_diag_access); ++i) {
			const struct bypass_diag_access_helper *acc;

			acc = &bypass_diag_access[i];
			acc->read(bypass, acc);
		}

		/* check if cycle counter has changed */
		counter = bypass->diag.scheduleCycleCounter;
		bypass_diag_access[SCHEDULE_CYCLE_COUNTER_DIDX].read(bypass,
			&bypass_diag_access[SCHEDULE_CYCLE_COUNTER_DIDX]);
		if (counter == bypass->diag.scheduleCycleCounter) {
			ret = 0;
			break;
		}
	}
	end = ktime_get_mono_fast_ns();
	dev_dbg(acm_dev(bypass->acm), "%s[%d] took %llu ns with %d retries\n",
		__func__, bypass->index, end - start, retry);

	return ret;
}

/**
 * @brief reset diagnostic data cache
 *
 * Remark: Caller must hold diag_lock!
 */
static void bypass_diag_reset(struct bypass *bypass)
{
	if (!bypass)
		return;

	memset(&bypass->diag, 0, sizeof(bypass->diag));
}

/**
 * @brief reset and initialize diagnostic data cache
 */
int bypass_diag_init(struct bypass *bypass)
{
	int ret;

	if (!bypass)
		return -EINVAL;

	ret = mutex_lock_interruptible(&bypass->diag_lock);
	if (ret)
		return ret;
	bypass_diag_reset(bypass);
	ret = bypass_diag_update(bypass);
	mutex_unlock(&bypass->diag_lock);

	return ret;
}

/**
 * @brief update diagnostic data cache and provide the respective data
 *
 * If the module parameter clear on read is set, the diagnostic cache will be
 * reset.
 */
int bypass_diag_read(struct bypass *bypass, struct acmdrv_diagnostics *diag)
{
	int ret;

	ret = mutex_lock_interruptible(&bypass->diag_lock);
	if (ret)
		return ret;

	ret = bypass_diag_update(bypass);
	if (ret)
		goto unlock;
	memcpy(diag, &bypass->diag, sizeof(*diag));
	if (clear_on_read)
		bypass_diag_reset(bypass);

unlock:
	mutex_unlock(&bypass->diag_lock);

	if (bypass->diag_poll_time > 0)
		/* (re)start timer */
		mod_delayed_work(bypass->acm->wq, &bypass->diag_work,
				 msecs_to_jiffies(bypass->diag_poll_time));

	return ret;
}

/**
 * @brief Check and clear on read NoFrameReceived flag for respective rule index
 */
bool bypass_recovery_no_frame_received(struct bypass *bypass, int idx)
{
	bool ret;
	const struct bypass_diag_access_helper *acc;

	/* update no_fames_received cache data */
	acc = &bypass_diag_access[NO_FRAME_RECEIVED_FLAGS_DIDX];
	acc->read(bypass, acc);

	/* get respective bit with clear on read */
	mutex_lock(&bypass->no_frames_received.lock);

	ret = !!(bypass->no_frames_received.flags & (1 << idx));
	bypass->no_frames_received.flags &= ~(1 << idx);

	mutex_unlock(&bypass->no_frames_received.lock);

	return ret;
}

/**
 * @brief Set respective TakeAny bit for individual recovery
 */
void bypass_recovery_take_any(struct bypass *bypass, int idx)
{
	u32 take_any;

	dev_dbg(acm_dev(bypass->acm),
		"Individual recovery: TakeAny for rule %d\n", idx);

	mutex_lock(&bypass->take_any_lock);

	take_any = bypass_control_area_read(bypass,
		ACM_BYPASS_CONTROL_AREA_INDIV_RECOV_TAKE_ANY);
	take_any |= (1 << idx);
	bypass_control_area_write(bypass,
		ACM_BYPASS_CONTROL_AREA_INDIV_RECOV_TAKE_ANY, take_any);

	mutex_unlock(&bypass->take_any_lock);
}
/**
 * @brief cleanup/initialize bypass module IP
 */
void bypass_cleanup(struct bypass *bypass)
{
	acm_iowrite32_set(bypass->base + ACM_BYPASS_STREAM_IDENT, 0,
			  ACMDRV_BYPASS_NR_RULES *
			  2 * sizeof(struct acmdrv_bypass_layer7_check));

	acm_iowrite32_set(bypass->base + ACM_BYPASS_DMA_CMD_LOOKUP, 0,
			  (ACMDRV_BYPASS_NR_RULES + 1) *
			  sizeof(struct acmdrv_bypass_stream_trigger));

	acm_iowrite32_set(bypass->base + ACM_BYPASS_SCATTER_DMA, 0,
			  ACMDRV_BYPASS_SCATTER_DMA_CMD_COUNT *
			  sizeof(struct acmdrv_bypass_dma_command));

	acm_iowrite32_set(bypass->base + ACM_BYPASS_GATHER_DMA_PREFETCH, 0,
			  ACMDRV_BYPASS_PREFETCH_DMA_CMD_COUNT *
			  sizeof(struct acmdrv_bypass_dma_command));

	acm_iowrite32_set(bypass->base + ACM_BYPASS_GATHER_DMA_EXEC, 0,
			  ACMDRV_BYPASS_GATHER_DMA_CMD_COUNT *
			  sizeof(struct acmdrv_bypass_dma_command));

	acm_iowrite32_set(bypass->base + ACM_BYPASS_CONSTANT_BUFFER, 0,
			  sizeof(struct acmdrv_bypass_const_buffer));
}

/**
 * @brief Getter for diagnostic data poll time
 */
unsigned int bypass_get_diag_poll_time(const struct bypass *bypass)
{
	return bypass->diag_poll_time;
}

/**
 * @brief Setter for diagnostic data poll time
 */
void bypass_set_diag_poll_time(unsigned int poll, struct bypass *bypass)
{
	if (bypass->diag_poll_time == poll)
		return;

	bypass->diag_poll_time = poll;

	if (poll > 0)
		/* (re)start timer */
		mod_delayed_work(bypass->acm->wq, &bypass->diag_work,
				 msecs_to_jiffies(bypass->diag_poll_time));
	else
		cancel_delayed_work_sync(&bypass->diag_work);
}

/**
 * @brief helper to gate bypass module tx output
 */
static void bypass_output_enable(struct bypass *bypass, bool onoff)
{
	dev_dbg(acm_dev(bypass->acm), "[BP%d]: output %s\n",
			bypass->index, onoff ? "enabled" : "disabled");
	bypass_control_area_write(bypass,
				  ACM_BYPASS_CONTROL_AREA_OUTPUT_DISABLE,
				  !onoff);
}

/**
 * @brief callback for netdevice events
 *
 * Since ACM IP 0.9.27 the output of the bypass module must be disabled by
 * software whenever the link goes down and vice versa. Otherwise some phys
 * (like KSZ9031) might hang thus not being able to autonegotiate a new link.
 * Here network events of the respective bypass module's port (derived by the
 * associated phy given in devicetree) trigger the appropriate action.
 */
static int bypass_netdev_event(struct notifier_block *nb,
			       unsigned long event, void *ptr)
{
	int i;
	struct net_device *ndev = netdev_notifier_info_to_dev(ptr);
	struct acm *acm = container_of(nb, struct acm, netdev_nb);

	dev_dbg(&acm->dev, "%s: netdev event 0x%08lx\n", ndev->name, event);

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i) {
		struct bypass *bypass = acm->bypass[i];

		/* no phy node -> no action */
		if (!bypass->phy_node)
			continue;

		/* if phy device has not been found yet, retry it now */
		if (!bypass->phy_dev)
			bypass->phy_dev = of_phy_find_device(bypass->phy_node);

		/* no phy device -> no action */
		if (!bypass->phy_dev)
			continue;

		/* not the right netdevice -> no action */
		if (bypass->phy_dev->attached_dev != ndev)
			continue;

		/*
		 * finally, it's an event from the bypass module's
		 * associated netdevice, so turn on/off the bypass module's
		 * output accordingly
		 */
		switch (event) {
		case NETDEV_UP:
			if (netif_carrier_ok(ndev))
				bypass_output_enable(bypass, true);
			break;
		case NETDEV_DOWN:
			bypass_output_enable(bypass, false);
			break;
		case NETDEV_CHANGE:
			if (netif_running(ndev))
				bypass_output_enable(bypass,
						     netif_carrier_ok(ndev));
			break;
		default:
			break;
		}
	}
	return NOTIFY_DONE;
}

/**
 * @brief timer function for diagnostic data poll
 */
static void bypass_diag_poll(struct work_struct *work)
{
	struct bypass *bypass = container_of(work, struct bypass,
					     diag_work.work);
	int ret;

	ret = mutex_lock_interruptible(&bypass->diag_lock);
	if (ret)
		return;
	bypass_diag_update(bypass);
	mutex_unlock(&bypass->diag_lock);
	if (bypass->diag_poll_time > 0)
		queue_delayed_work(bypass->acm->wq,
				   &bypass->diag_work,
				   msecs_to_jiffies(bypass->diag_poll_time));
}

int __must_check bypass_init(struct acm *acm)
{
	int ret;
	int i;
	struct bypass *bypass;
	struct platform_device *pdev = acm->pdev;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

	bypass = devm_kcalloc(dev, ACMDRV_BYPASS_MODULES_COUNT, sizeof(*bypass),
			      GFP_KERNEL);
	if (!bypass)
		return -ENOMEM;

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i) {
		struct resource *res;
		char *resname = kasprintf(GFP_KERNEL, "Bypass%i", i);

		if (!resname)
			return -ENOMEM;

		bypass[i].acm = acm;
		bypass[i].index = i;

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   resname);
		bypass[i].base = devm_ioremap_resource(dev, res);
		if (IS_ERR(bypass[i].base))
			return PTR_ERR(bypass[i].base);

		bypass[i].size = resource_size(res);

		bypass[i].phy_node = of_parse_phandle(np, "ports", i);
		bypass[i].phy_dev = of_phy_find_device(bypass[i].phy_node);

		bypass_enable(&bypass[i], false);

		bypass_control_area_write(bypass,
			ACM_BYPASS_CONTROL_AREA_CONNECTION_MODE,
			ACMDRV_BYPASS_CONN_MODE_PARALLEL);

		mutex_init(&bypass[i].diag_lock);

		/* Init those 2 here as bypass_diag_or_no_frames_received may
		   be called at the beginning */
		bypass[i].no_frames_received.flags = 0;
		mutex_init(&bypass[i].no_frames_received.lock);

		ret = bypass_diag_init(&bypass[i]);
		if (ret)
			return ret;

		INIT_DELAYED_WORK(&bypass[i].diag_work, bypass_diag_poll);

		/* load initial diagnostic poll timeout */
		bypass_set_diag_poll_time(diag_poll, &bypass[i]);

		mutex_init(&bypass[i].take_any_lock);

		dev_dbg(dev, "Probed Bypass[%d] %pr -> 0x%p\n", i, res,
			bypass[i].base);

		acm->bypass[i] = &bypass[i];
	}

	acm->netdev_nb.notifier_call = bypass_netdev_event;
	ret = register_netdevice_notifier(&acm->netdev_nb);
	return ret;
}

void bypass_exit(struct acm *acm)
{
	int i;

	unregister_netdevice_notifier(&acm->netdev_nb);
	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i) {
		struct bypass *bypass = acm->bypass[i];

		cancel_delayed_work_sync(&bypass->diag_work);
		if (bypass->phy_dev)
			put_device(&bypass->phy_dev->mdio.dev);
		bypass->phy_dev = NULL;
		of_node_put(bypass->phy_node);
		bypass->phy_node = NULL;
	}
}
/**@} hwaccbypass */

/**
 * @addtogroup acmmodparam
 * @{
 */
/**
 * @brief Linux module parameter definition for maximum diagnostic read retries
 */
module_param(max_diag_retry, uint, 0644);

/**
 * @brief Linux module parameter description
 */
MODULE_PARM_DESC(max_diag_retry, "Number of maximum diagnostic read retries");

/**
 * @brief Linux module parameter definition to control, if diagnostics are
 *        cleared on read.
 */
module_param(clear_on_read, bool, 0644);

/**
 * @brief Linux module parameter description
 */
MODULE_PARM_DESC(clear_on_read, "Enable Clear on Read");

/**
 * @brief Linux module parameter for initial diagnostic poll cycle in ms
 */
module_param(diag_poll, uint, 0444);

/**
 * @brief Linux module parameter description
 */
MODULE_PARM_DESC(diag_poll, "Initial diagnostic poll cycle in ms");

/**@} acmmodparam */

