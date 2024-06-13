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
 * @file scheduler.c
 * @brief ACM Driver Scheduler Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup hwaccsched ACM IP Scheduler
 * @brief Provides access to the scheduler module IP
 *
 * @{
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "edge.h"

#include "acm-module.h"
#include "scheduler.h"
#include "acmbitops.h"
#include "bypass.h"

#include <linux/delay.h>

/**
 * @brief max start time offset for delaying start time write request
 */
#define ACM_SCHEDULER_MAX_FUTURE_MSECS	64000

struct scheduler;
/**
 * @struct sched_table
 * @brief data per scheduler table
 *
 * @struct sched_table::sched_table_work
 * @brief extension of delayed_work
 */
struct sched_table {
	void __iomem *base;		/**< base address */

	struct mutex table_row_lock;	/**< table row access lock */
	struct mutex cycle_time_lock;	/**< cycle time access lock */

	struct timespec64 time;		/**< the requested start time */
	bool trigger;			/**< trigger schedule start */
	struct sched_table_work {
		struct scheduler *sched;	/**< backward reference */
		struct delayed_work dwork;	/**< kernel delayed work */
	} work;				/**< delayed work */
	struct completion complete;	/**< completion of write start time */
	struct mutex lock;		/**< prohibit simultaneous writes */
};

/**
 * @struct sched_data
 * @brief data per scheduler
 *
 * @var sched_data::base
 * @brief scheduler base address
 *
 * @var sched_data::active
 * @brief scheduler active flag
 *
 * @var sched_data::table
 * @brief scheduler tables
 */
struct sched_data {
	void __iomem *base;

	bool active;	/**< active state of scheduler */
	struct sched_table table[ACMDRV_SCHED_TBL_COUNT];
};

/**
 * @struct scheduler
 * @brief scheduler module handler instance
 *
 * @var scheduler::base
 * @brief scheduler handler base address
 *
 * @var scheduler::size
 * @brief scheduler handler module size
 *
 * @var scheduler::acm
 * @brief associated ACM instance
 *
 * @var scheduler::frtc
 * @brief FRTC for access to worker PTP time
 *
 * @var scheduler::data
 * @brief single scheduler instance data
 */
struct scheduler {
	void __iomem *base;
	resource_size_t	size;

	struct acm *acm;

	struct platform_device *frtc;
	struct sched_data data[ACMDRV_SCHEDULER_COUNT];
};

/**
 * @brief read generic scheduler clock frequency in MHz
 */
static u16 read_clk_freq(struct scheduler *scheduler)
{
	return readw(SCHED_GENERICS(scheduler, CLK_FREQ)) & GENMASK(7, 0);
}

/**
 * @brief read scheduler table cycle time from ACM-IP FSC scheduler table
 */
static void read_cycle_time(struct sched_table *table,
			    struct acmdrv_sched_cycle_time *time)
{
	mutex_lock(&table->cycle_time_lock);

	time->subns = readw(SCHED_TABLE(table, CYCLETIME_SUBNS_HIGH));
	time->subns <<= 8;
	time->subns += (readw(SCHED_TABLE(table, CYCLETIME_SUBNS_LOW))
		& GENMASK(15, 8)) >> 8;
	time->ns = readw(SCHED_TABLE(table, CYCLETIME_NS_HIGH))
		& GENMASK(13, 0);
	time->ns <<= 16;
	time->ns += readw(SCHED_TABLE(table, CYCLETIME_NS_LOW));

	mutex_unlock(&table->cycle_time_lock);
}

/**
 * @brief write scheduler table start time to ACM-IP
 */
static void write_start_time(struct sched_table *table)
{
	pr_debug("%s(%p, %lld.%09ld, %s)\n", __func__, table,
		table->time.tv_sec, table->time.tv_nsec,
		table->trigger ? "true" : "false");

	writew(table->time.tv_nsec & GENMASK(15, 0),
		SCHED_TABLE(table, STARTTIME_NS_LOW));
	writew((table->time.tv_nsec >> 16) & GENMASK(13, 0),
		SCHED_TABLE(table, STARTTIME_NS_HIGH));
	writew(table->time.tv_sec & GENMASK(7, 0),
		SCHED_TABLE(table, STARTTIME_SEC));

	/* now start the table */
	if (table->trigger) {
		u16 tbl_gen;

		/* we schedule forever */
		writew(0, SCHED_TABLE(table, LAST_CYCLE));

		/* trigger start */
		tbl_gen = readw(SCHED_TABLE(table, TBL_GEN));
		tbl_gen |= TBL_GEN_CAN_BE_TAKEN;
		tbl_gen &= ~TBL_GEN_LAST_CYC_NR_EN;
		writew(tbl_gen, SCHED_TABLE(table, TBL_GEN));
		pr_debug("%s(%p): Schedule started\n", __func__, table);
	}
}

/**
 * @brief Signal write start time completed
 */
static void write_start_time_done(struct sched_table *table)
{
	pr_debug("%s(%p)\n", __func__, table);

	complete_all(&table->complete);
}

/**
 * @brief Prepare eventually async write f start time
 */
static int write_start_time_prepare(struct sched_table *table,
	const struct timespec64 *time, bool trig, unsigned long timeout)
{
	pr_debug("%s(%p)\n", __func__, table);

	if (cancel_delayed_work_sync(&table->work.dwork))
		pr_debug("%s(%p): Work triggering at %lld.%09lu has been cancelled\n",
			__func__, table, table->time.tv_sec,
			table->time.tv_nsec);

	reinit_completion(&table->complete);
	table->time = *time;
	table->trigger = trig;
	return 0;
}

/**
 * @brief Get PTP worker time used for scheduler
 */
ktime_t scheduler_ktime_get_ptp(struct scheduler *scheduler)
{
	if (!scheduler)
		return ktime_set(0, 0);

	return edgx_ktime_get_worker_ptp(scheduler->frtc);
}


/**
 * @brief Trigger delayed write of schedule start time if required
 */
static void write_start_time_execute(struct sched_table *table)
{
	ktime_t start, now;
	u64 future_msecs;

	now = scheduler_ktime_get_ptp(table->work.sched);

	pr_debug("%s(%p): PTP time: %lld.%09lu\n", __func__, table,
		ktime_to_timespec64(now).tv_sec,
		ktime_to_timespec64(now).tv_nsec);

	/*
	 * do not write start time directly, but support the scheduler's
	 * start time window.
	 */
	start = timespec64_to_ktime(table->time);

	if (ktime_before(start, now)) {
		/* start is in the past */
		struct acmdrv_sched_cycle_time cycle_time;
		ktime_t delta = ktime_sub(now, start);
		s64 count;

		read_cycle_time(table, &cycle_time);

		/* FIXME: support subns */
		if (cycle_time.subns != 0)
			pr_warn("%s(%p): Cycle time sub nanoseconds ignored",
				__func__, table);

		if (cycle_time.ns == 0) {
			pr_err("%s(%p): Cycle time on not set", __func__,
				table);
			return;
		}
		count = ktime_divns(delta, cycle_time.ns) + 1;
		start = ktime_add_ns(start, count * cycle_time.ns);

		pr_debug("%s(%p): Start adjusted: %lld.%09lu\n",
			__func__, table, ktime_to_timespec64(start).tv_sec,
			ktime_to_timespec64(start).tv_nsec);
	}

	future_msecs = ktime_ms_delta(start, now);

	if (future_msecs > ACM_SCHEDULER_MAX_FUTURE_MSECS) {
		/* Too far away in the future to be handled now. */
		future_msecs = do_div(future_msecs,
			ACM_SCHEDULER_MAX_FUTURE_MSECS);

		pr_info("%s(%p): Delaying write start time for %lld milliseconds\n",
			__func__, table, future_msecs);
		mod_delayed_work(system_wq, &table->work.dwork,
			msecs_to_jiffies(future_msecs));

		return;
	}

	table->time = ktime_to_timespec64(start);

	write_start_time(table);
	write_start_time_done(table);
}

/**
 * @brief Worker function for delayed schedule start time write
 */
static void write_start_time_worker_func(struct work_struct *work)
{
	struct sched_table *table =
		container_of(work, struct sched_table, work.dwork.work);

	pr_debug("%s(%p)\n", __func__, table);

	write_start_time_execute(table);
}

/**
 * @brief synchronous write of scheduler start time (from user space))
 */
static int scheduler_write_start_time_sync(struct scheduler *sched, int sidx,
	int tidx, const struct timespec64 *time, bool trigger,
	unsigned long timeout)
{
	long timeleft;
	struct sched_table *table = &sched->data[sidx].table[tidx];

	dev_dbg(acm_dev(sched->acm), "%s(%d, %d, %lld.%09ld, %s)\n",
		__func__, sidx, tidx, time->tv_sec, time->tv_nsec,
		trigger ? "true" : "false");

	scheduler_write_start_time(sched, sidx, tidx, time, trigger);
	timeleft = wait_for_completion_interruptible_timeout(&table->complete,
		msecs_to_jiffies(timeout));

	if (timeleft == 0)
		return -ETIMEDOUT;
	if (timeleft < 0)
		return timeleft;

	return 0;
}

/**
 * @brief read sched_down_counter
 */
u16 scheduler_read_sched_down_counter(struct scheduler *sched, int sidx)
{
	struct sched_data *data;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n",
			__func__, sidx);
		return 0;
	}

	data = &sched->data[sidx];
	return readw(data->base + ACM_SCHEDULER_SCHED_CTRL_SCH_GEN);
}

/**
 * @brief write sched_down_counter
 */
void scheduler_write_sched_down_counter(struct scheduler *sched, int sidx,
					u16 cnt)
{
	struct sched_data *data;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		return;
	}

	data = &sched->data[sidx];
	writew(cnt, data->base + ACM_SCHEDULER_SCHED_CTRL_SCH_GEN);
}

/**
 * @brief number of retries when waiting for table row transfer to finish
 */
#define WAIT_TABLE_ROW_RETRY	8
/**
 * @brief delay in us between retries when waiting for table row transfer to
 *        finish
 */
#define WAIT_TABLE_ROW_DELAY	4
/**
 * @brief wait for a table row transfer to finish
 */
static int _wait_table_row_transfer(struct scheduler *scheduler)
{
	int i;
	u16 cmd0;

	/* check if transfer is done */
	for (i = 0; i < WAIT_TABLE_ROW_RETRY; ++i) {
		bool in_xfer;

		cmd0 = readw(SCHED_COMMON(scheduler, ROW_ACCESS_CMD0));
		in_xfer = read_bitmask16(&cmd0, CMD0_TRANSFER);

		if (in_xfer)
			udelay(WAIT_TABLE_ROW_DELAY);
		else
			break;
	}

	if (i < WAIT_TABLE_ROW_RETRY)
		return 0;

	return -ETIMEDOUT;
}

/**
 * @brief read a row of scheduler table
 */
int __must_check scheduler_read_table_row(struct scheduler *scheduler,
					  int sched_id, int tab_id, int row_id,
					  struct acmdrv_sched_tbl_row *row)
{
	int ret;
	u16 cmd1 = 0;
	u16 cmd0 = 0;

	if (sched_id >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(scheduler->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sched_id);
		return -EINVAL;
	}

	if (tab_id >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(scheduler->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tab_id);
		return -EINVAL;
	}

	if (row_id >= ACMDRV_SCHED_TBL_ROW_COUNT) {
		dev_err(acm_dev(scheduler->acm),
			"%s: scheduler table row out of range: %d\n",
			__func__, row_id);
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(
		&scheduler->data[sched_id].table[tab_id].table_row_lock);
	if (ret)
		return ret;

	/* trigger read */
	write_bitmask16(row_id, &cmd1, CMD1_ROW_NUMBER);
	writew(cmd1, SCHED_COMMON(scheduler, ROW_ACCESS_CMD1));
	write_bitmask16(sched_id, &cmd0, CMD0_SCHEDULER);
	write_bitmask16(tab_id, &cmd0, CMD0_TABLE);
	write_bitmask16(0, &cmd0, CMD0_WRITE);
	write_bitmask16(1, &cmd0, CMD0_TRANSFER);
	writew(cmd0, SCHED_COMMON(scheduler, ROW_ACCESS_CMD0));

	ret = _wait_table_row_transfer(scheduler);
	if (ret)
		goto out;

	row->cmd = (readw(SCHED_COMMON(scheduler, ROW_ACCESS_DATA1)) << 16)
			+ readw(SCHED_COMMON(scheduler, ROW_ACCESS_DATA0));
	row->delta_cycle = readw(SCHED_COMMON(scheduler, ROW_ACCESS_DATA4));

out:
	mutex_unlock(&scheduler->data[sched_id].table[tab_id].table_row_lock);
	return ret;
}

/**
 * @brief write a row of scheduler table
 */
int __must_check scheduler_write_table_row(struct scheduler *scheduler,
		int sched_id, int tab_id, int row_id,
		const struct acmdrv_sched_tbl_row *row)
{
	int ret;
	u16 cmd1 = 0;
	u16 cmd0 = 0;

	if (sched_id >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(scheduler->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sched_id);
		return -EINVAL;
	}

	if (tab_id >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(scheduler->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tab_id);
		return -EINVAL;
	}

	if (row_id >= ACMDRV_SCHED_TBL_ROW_COUNT) {
		dev_err(acm_dev(scheduler->acm),
			"%s: scheduler table row out of range: %d\n",
			__func__, row_id);
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(
		&scheduler->data[sched_id].table[tab_id].table_row_lock);
	if (ret)
		return ret;

	writew(low_16bits(row->cmd), SCHED_COMMON(scheduler, ROW_ACCESS_DATA0));
	writew(high_16bits(row->cmd),
	       SCHED_COMMON(scheduler, ROW_ACCESS_DATA1));
	writew(row->delta_cycle, SCHED_COMMON(scheduler, ROW_ACCESS_DATA4));

	write_bitmask16(row_id, &cmd1, CMD1_ROW_NUMBER);
	writew(cmd1, SCHED_COMMON(scheduler, ROW_ACCESS_CMD1));

	write_bitmask16(sched_id, &cmd0, CMD0_SCHEDULER);
	write_bitmask16(tab_id, &cmd0, CMD0_TABLE);
	write_bitmask16(1, &cmd0, CMD0_WRITE);
	write_bitmask16(1, &cmd0, CMD0_TRANSFER);
	writew(cmd0, SCHED_COMMON(scheduler, ROW_ACCESS_CMD0));

	ret = _wait_table_row_transfer(scheduler);

	mutex_unlock(&scheduler->data[sched_id].table[tab_id].table_row_lock);
	return ret;
}

/**
 * @brief read scheduler table control & status
 */
u16 scheduler_read_table_status(struct scheduler *sched, int sidx, int tidx)
{
	u16 status;
	struct sched_table *table;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		return 0;
	}

	if (tidx >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tidx);
		return 0;
	}


	table = &sched->data[sidx].table[tidx];
	status = readw(SCHED_TABLE(table, TBL_GEN));

	return status;
}

/**
 * @brief write scheduler table cycle time
 */
void scheduler_read_cycle_time(struct scheduler *sched, int sidx, int tidx,
			       struct acmdrv_sched_cycle_time *time)
{
	struct sched_table *table;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		return;
	}

	if (tidx >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tidx);
		return;
	}

	table = &sched->data[sidx].table[tidx];
	read_cycle_time(table, time);
}
/**
 * @brief write scheduler table cycle time
 */
void scheduler_write_cycle_time(struct scheduler *sched, int sidx, int tidx,
				const struct acmdrv_sched_cycle_time *time)
{
	struct sched_table *table;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		return;
	}

	if (tidx >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tidx);
		return;
	}

	table = &sched->data[sidx].table[tidx];
	mutex_lock(&table->cycle_time_lock);

	writew((time->subns << 8) & GENMASK(15, 8),
		SCHED_TABLE(table, CYCLETIME_SUBNS_LOW));
	writew((time->subns >> 8) & GENMASK(15, 0),
		SCHED_TABLE(table, CYCLETIME_SUBNS_HIGH));
	writew(time->ns & GENMASK(15, 0),
		SCHED_TABLE(table, CYCLETIME_NS_LOW));
	writew((time->ns >> 16) & GENMASK(13, 0),
		SCHED_TABLE(table, CYCLETIME_NS_HIGH));

	mutex_unlock(&table->cycle_time_lock);
}

/**
 * @brief read scheduler table start time
 */
int scheduler_read_start_time(struct scheduler *sched, int sidx,
			       int tidx, struct timespec64 *time)
{
	int ret = 0;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		ret = -EINVAL;
		goto out;
	}

	if (tidx >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tidx);
		ret = -EINVAL;
		goto out;
	}

	*time = sched->data[sidx].table[tidx].time;
out:
	return 0;
}

/**
 * @brief Write scheduler start time (for use from user space API/SYSFS)
 */
int scheduler_write_start_time(struct scheduler *sched, int sidx, int tidx,
		const struct timespec64 *time, bool trig)
{
	struct sched_table *table;
	int ret = 0;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		ret = -EINVAL;
		goto out;
	}

	if (tidx >= ACMDRV_SCHED_TBL_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler table index out of range: %d\n",
			__func__, tidx);
		ret = -EINVAL;
		goto out;
	}

	table = &sched->data[sidx].table[tidx];
	dev_dbg(acm_dev(sched->acm), "%s(%d, %d, %lld.%09ld, %s)\n",
		__func__, sidx, tidx, time->tv_sec, time->tv_nsec,
		trig ? "true" : "false");

	ret = mutex_lock_interruptible(&table->lock);
	if (ret)
		goto out;

	ret = write_start_time_prepare(table, time, trig, 1000);
	if (ret) {
		dev_err(acm_dev(sched->acm),
			"%s(%d, %d): write_start_time_prepare() failed: %d\n",
			__func__, sidx, tidx, ret);
		goto out_unlock;
	}
	write_start_time_execute(table);

out_unlock:
	mutex_unlock(&table->lock);
out:
	return ret;
}

/**
 * @brief read scheduler emergency disable
 */
u16 scheduler_read_emergency_disable(struct scheduler *sched, int sidx)
{
	struct sched_data *data;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		return 0;
	}

	data = &sched->data[sidx];
	return readw(data->base + ACM_SCHEDULER_SCHED_CTRL_EME_DIS_CTRL);
}

/**
 * @brief write scheduler emergency disable
 */
void scheduler_write_emergency_disable(struct scheduler *sched,
				       int sidx, u16 emerg_disable)
{
	struct sched_data *data;

	if (sidx >= ACMDRV_SCHEDULER_COUNT) {
		dev_err(acm_dev(sched->acm),
			"%s: scheduler index out of range: %d\n", __func__,
			sidx);
		return;
	}

	data = &sched->data[sidx];
	writew(emerg_disable, data->base +
		ACM_SCHEDULER_SCHED_CTRL_EME_DIS_CTRL);
}


/**
 * @brief entire scheduler module cleanup/initialization of hardware
 */
void scheduler_cleanup(struct scheduler *scheduler)
{
	int ret;
	int sidx;

	const int min_delta_cycle = 8;
	const struct acmdrv_sched_tbl_row stop_row = {
		.cmd = 0,
		.delta_cycle = min_delta_cycle,
	};

	const struct acmdrv_sched_tbl_row nullrow = {
		.cmd = 0,
		.delta_cycle = 0,
	};

	const struct acmdrv_sched_cycle_time cycletime = {
		.subns = 0,
		.ns = 1000 / read_clk_freq(scheduler) * min_delta_cycle
	};


	for (sidx = 0; sidx < ARRAY_SIZE(scheduler->data); ++sidx) {
		int tidx;
		int i, j;
		u16 tbl_gen;
		ktime_t now;
		struct timespec64 starttime;
		struct sched_table *table;
		struct sched_data *data = &scheduler->data[sidx];

		/* find unused table */
		for (tidx = 0; tidx < ACMDRV_SCHED_TBL_COUNT; ++tidx) {
			u16 status;

			status = scheduler_read_table_status(scheduler, sidx,
							     tidx);
			if (!(status & TBL_GEN_IN_USE))
				break;
		}

		if (tidx >= ACMDRV_SCHED_TBL_COUNT) {
			dev_err(acm_dev(scheduler->acm),
				"%s: Cannot find unused scheduling table(%d)\n",
				__func__, sidx);
			continue;
		}

		dev_dbg(acm_dev(scheduler->acm),
			"%s: Using table %d for scheduler %d\n", __func__, tidx,
			sidx);


		/* write stop row entry to scheduling table */
		ret = scheduler_write_table_row(scheduler, sidx, tidx, 0,
						&stop_row);
		if (ret)
			dev_err(acm_dev(scheduler->acm),
				"scheduler_write_table_row(%d, %d, 0) failed: %d",
				sidx, tidx, ret);
		/* set the rest of the table to null */
		for (i = 1; i < ACMDRV_SCHED_TBL_ROW_COUNT; ++i) {
			ret = scheduler_write_table_row(scheduler, sidx,
							tidx, i, &nullrow);
			if (ret)
				dev_err(acm_dev(scheduler->acm),
					"scheduler_write_table_row(%d, %d, %d) failed: %d",
					sidx, tidx, i, ret);
		}

		/* set minimal cycle time */
		scheduler_write_cycle_time(scheduler, sidx, tidx,
			&cycletime);

		/* set start time to start immediately */
		now = edgx_ktime_get_worker_ptp(scheduler->frtc);
		starttime = ktime_to_timespec64(now);

		dev_dbg(acm_dev(scheduler->acm),
			"%s/ST%d:%d: PTP time: %lld.%09ld\n", __func__,
			sidx, tidx, starttime.tv_sec, starttime.tv_nsec);

		ret = scheduler_write_start_time_sync(scheduler, sidx, tidx,
			&starttime, false, 1000);
		if (ret != 0) {
			dev_err(acm_dev(scheduler->acm),
				"%s: scheduler_write_start_time_sync(%d, %d) failed: %d",
				__func__, sidx, tidx, ret);
			continue;
		}

		table = &data->table[tidx];

		/* we only schedule once */
		writew(1, SCHED_TABLE(table, LAST_CYCLE));

		/* enable last cycle and start schedule */
		tbl_gen = readw(SCHED_TABLE(table, TBL_GEN));
		tbl_gen |= TBL_GEN_CAN_BE_TAKEN;
		tbl_gen |= TBL_GEN_LAST_CYC_NR_EN;
		writew(tbl_gen, SCHED_TABLE(table, TBL_GEN));
		pr_debug("%s(%p): Schedule started\n", __func__, table);

		/* wait until table has been activated and last cycle reached */
		do {
			tbl_gen = readw(SCHED_TABLE(table, TBL_GEN));
			cond_resched();
		} while ((tbl_gen & (TBL_GEN_IN_USE | TBL_GEN_LAST_CYC_REACHED))
			!= (TBL_GEN_IN_USE | TBL_GEN_LAST_CYC_REACHED));
		pr_debug("%s(%p): Last cycle reached, tbl_gen: 0x%04x\n",
			__func__, table, tbl_gen);

		/* finally null the other scheduling tables */
		for (i = 0; i < ACMDRV_SCHED_TBL_COUNT; ++i) {
			if (i == tidx)
				continue;
			for (j = 0; j < ACMDRV_SCHED_TBL_ROW_COUNT; ++j) {
				ret = scheduler_write_table_row(scheduler, sidx,
					i, j, &nullrow);
				if (ret)
					dev_err(acm_dev(scheduler->acm),
						"scheduler_write_table_row(%d, %d, %d) failed: %d",
						sidx, i, j, ret);
			}
		}
	}
}

/**
 * @brief activate/deactivate scheduler
 */
void scheduler_set_active(struct scheduler *scheduler, int i, bool active)
{
	scheduler->data[i].active = active;
}

/**
 * @brief check if scheduler is active
 */
bool scheduler_get_active(struct scheduler *scheduler, int i)
{
	return scheduler->data[i].active;
}

/**
 * @brief initialize data for scheduler table
 */
static void scheduler_data_table_init(struct scheduler *sched,
				      struct sched_data *data, int idx)
{
	struct sched_table *table = &data->table[idx];

	table->base = data->base + ACM_SCHEDULER_SCHED_TAB(idx);

	mutex_init(&table->table_row_lock);
	mutex_init(&table->cycle_time_lock);

	table->time = (struct timespec64){ 0, 0 };
	table->trigger = false;
	init_completion(&table->complete);
	table->work.sched = sched;
	INIT_DELAYED_WORK(&table->work.dwork,
			write_start_time_worker_func);
	mutex_init(&table->lock);
}

/**
 * @brief de-initialize scheduler table
 */
static void scheduler_data_table_exit(struct sched_data *data, int idx)
{
	struct sched_table *table = &data->table[idx];

	cancel_delayed_work_sync(&table->work.dwork);
	complete_all(&table->complete);
}

/**
 * @brief initialize data for single scheduler
 */
static void scheduler_data_init(struct scheduler *sched, int idx)
{
	int i;
	struct sched_data *data = &sched->data[idx];

	data->base = sched->base + ACM_SCHEDULER_SCHED(idx);
	data->active = false;

	for (i = 0; i < ARRAY_SIZE(data->table); ++i)
		scheduler_data_table_init(sched, data, i);
}

/**
 * @brief de-initialize data for single scheduler
 */
static void scheduler_data_exit(struct scheduler *sched, int idx)
{
	int i;
	struct sched_data *data = &sched->data[idx];

	for (i = 0; i < ARRAY_SIZE(data->table); ++i)
		scheduler_data_table_exit(data, i);
}

/**
 * @brief initialize scheduler module handler
 */
int __must_check scheduler_init(struct acm *acm)
{
	int i;
	struct resource *res;
	struct scheduler *scheduler;
	struct platform_device *pdev = acm->pdev;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	u16 freq, dwncntr_speed;
	struct device_node *ptp_node;

	pr_warn("Scheduler probe devm_kzalloc\n");
	mdelay(5);

	scheduler = devm_kzalloc(dev, sizeof(*scheduler), GFP_KERNEL);
	if (!scheduler)
		return -ENOMEM;
	scheduler->acm = acm;
	pr_warn("Scheduler probe platform_get_resource_byname\n");
	mdelay(5);
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "Scheduler");
	pr_warn("Scheduler probe devm_ioremap_resource\n");
	mdelay(5);
	scheduler->base = devm_ioremap_resource(dev, res);
	pr_warn("Scheduler probe base addr:%p\n", scheduler->base);
	mdelay(5);
	if (IS_ERR(scheduler->base))
		return PTR_ERR(scheduler->base);
	scheduler->size = resource_size(res);

	for (i = 0; i < ARRAY_SIZE(scheduler->data); ++i) {
		pr_warn("Scheduler probe scheduler_data_init:%d\n", i);
		mdelay(5);
		scheduler_data_init(scheduler, i);
	}
	pr_warn("Scheduler probe of_parse_phandle\n");
	mdelay(5);

	ptp_node = of_parse_phandle(np, "ptp_worker", 0);
	if (!ptp_node) {
		pr_warn("Mandatory ptp_worker not defined");
		return -EINVAL;
	}
	dev_dbg(dev, "Scheduler probe of_find_device_by_node\n");
	mdelay(5);

	scheduler->frtc = of_find_device_by_node(ptp_node);
	/* defer probing until deipce got loaded */
	if (!scheduler->frtc)
		return -EPROBE_DEFER;

	/*
	 * Issue 680: The emergency Status register
	 */
	for (i = 0; i < ARRAY_SIZE(scheduler->data); ++i) {
		struct sched_data *data = &scheduler->data[i];
		pr_warn("Scheduler probe emergency Status registe:%d\n",i);
		pr_warn("Scheduler probe ADDr1:%p ADDr2:%p\n",data->base + ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(0), data->base + ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(1));
		pr_warn("Scheduler probe or base + ADDr1:%x base + ADDr2:%x\n",ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(0), ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(1));
		mdelay(1);

//		writew(0, data->base +
//			ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(0));
//		writew(0x8000, data->base +
//		       ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(1));
		pr_warn("Scheduler probe values ADDr1:%x\n",readw(data->base + ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(0)));
		mdelay(1);
		pr_warn("Scheduler probe values ADDr2:%x\n",readw(data->base + ACM_SCHEDULER_SCHED_CTRL_EME_DIS_STAT(1)));
		mdelay(1);
	}

	pr_warn("Scheduler probe read_clk_freq\n");
	mdelay(5);

	/* Configure clock frequency */
	freq = read_clk_freq(scheduler);
	pr_warn("Scheduler probe read_clk_freq:%d\n", freq);
	mdelay(5);
	switch (freq) {
	case 125:
		dwncntr_speed = DWNCNTR_SPD_125MHZ;
		break;
	case 100:
		dwncntr_speed = DWNCNTR_SPD_100MHZ;
		break;
	case 200:
		dwncntr_speed = DWNCNTR_SPD_200MHZ;
		pr_warn("Probed Scheduler %pr -> freq %d \n", res, freq);
		mdelay(5);
		break;
	default:
		dev_err(dev, "Unsupported scheduler clock frequency: %u", freq);
		return -EIO;
	}

	for (i = 0; i < ARRAY_SIZE(scheduler->data); ++i) {
		struct sched_data *data = &scheduler->data[i];
		pr_warn("Scheduler probe write downctr speed:%d\n", i);
		pr_warn("Scheduler probe ADDr:%p \n",data->base + ACM_SCHEDULER_SCHED_CTRL_DWNCNTR_SPD);
		pr_warn("Scheduler probe or base + ADDr1:%x\n", ACM_SCHEDULER_SCHED_CTRL_DWNCNTR_SPD);
		mdelay(5);
		//writew(dwncntr_speed,
		//	data->base + ACM_SCHEDULER_SCHED_CTRL_DWNCNTR_SPD);
		pr_warn("Scheduler probe values ADDr1:%x\n", readw(data->base + ACM_SCHEDULER_SCHED_CTRL_DWNCNTR_SPD));
		mdelay(1);

	}

	dev_dbg(dev, "Probed Scheduler %pr -> 0x%p\n", res, scheduler->base);

	acm->scheduler = scheduler;
	return 0;
}

/**
 * @brief exit scheduler module handler
 */
void scheduler_exit(struct acm *acm)
{
	int i;
	struct scheduler *sched = acm->scheduler;

	for (i = 0; i < ARRAY_SIZE(sched->data); ++i)
		scheduler_data_exit(sched, i);

}
/**@} hwaccsched */
