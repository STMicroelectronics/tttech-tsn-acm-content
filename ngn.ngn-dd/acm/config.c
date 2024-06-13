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
 * @file config.c
 * @brief ACM Driver Configuration Handling
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup logicctrl ACM Driver Logic Control
 * @brief ACM state and configuration control handling
 *
 * @{
 */

#include <linux/delay.h>
#include <linux/io.h>

#include "acm-module.h"
#include "config.h"
#include "scheduler.h"
#include "bypass.h"
#include "msgbuf.h"
#include "chardev.h"
#include "redundancy.h"
#include "reset.h"
#include "commreg.h"

/**
 * @brief Cleanup ACM
 */
static void acm_cleanup(struct acm *acm)
{
	int i;

	msgbuf_cleanup(acm->msgbuf);

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i)
		bypass_cleanup(acm->bypass[i]);

	scheduler_cleanup(acm->scheduler);
	redundancy_cleanup(acm->redundancy);
}

/**
 * @brief Clean up the entire ACM-configuration.
 *
 * @param acm ACM instance
 * @param new_status Returns new ACM state
 * @return Returns 0 on success, negative error code otherwise.
 */
int acm_config_remove(struct acm *acm, enum acmdrv_status *new_status)
{
	int i;
	int ret = 0;

	*new_status = ACMDRV_INIT_STATE;

	/* 1. Disable emergency_disable */
	for (i = 0; i < ACMDRV_SCHEDULER_COUNT; ++i) {
		scheduler_write_emergency_disable(acm->scheduler, i, 1);
		scheduler_set_active(acm->scheduler, i, false);
	}

	/* 2. disable ACM-Modules and remove recovery settings */
	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i) {
		int j;

		bypass_enable(acm->bypass[i], false);
		bypass_set_active(acm->bypass[i], false);

		for (j = 0; j < ACMDRV_BYPASS_NR_RULES; ++j)
			/* ignore returned value */
			ret = redundancy_set_individual_recovery_timeout(
				acm->redundancy, i, j, 0);
	}
	for (i = 0; i < ACMDRV_REDUN_TABLE_ENTRY_COUNT; ++i)
		/* ignore returned value */
		ret = redundancy_set_base_recovery_timeout(acm->redundancy, i,
			0);

	/* 3. reset the IP */
	ret = reset_trigger(acm->reset);
	if (ret)
		return ret;

	/* 4. Clean up devices for the message buffers */
	for (i = 0; i < commreg_read_msgbuf_count(acm->commreg); ++i) {
		struct acm_dev *adev = acm_dev_get_device(acm->devices, i);

		acm_dev_deactivate(adev);
		/*
		 * clear message buffer status registers and overwritten
		 * counters by reading the status and the counters
		 */
		msgbuf_read_status(acm->msgbuf, i);
		msgbuf_read_clear_overwritten(acm->msgbuf, i);
	}

	/* 5. clean-up entire ACM IP */
	acm_cleanup(acm);
	return 0;
}

/**
 * @brief start configuration phase of ACM
 *
 * @param acm ACM instance
 * @param new_status Returns new ACM state
 * @return Returns 0 on success, negative error code otherwise.
 */
int acm_config_start(struct acm *acm, enum acmdrv_status *new_status)
{
	return acm_config_remove(acm, new_status);
}

/**
 * @brief end configuration phase of ACM
 *
 * @param acm ACM instance
 * @param new_status Returns new ACM state
 * @return Returns 0 on success, negative error code otherwise.
 */
int acm_config_end(struct acm *acm, enum acmdrv_status *new_status)
{
	int i;

	/* Store which modules are active */
	for (i = 0; i < ACMDRV_SCHEDULER_COUNT; ++i)
		if (scheduler_read_emergency_disable(acm->scheduler, i))
			scheduler_set_active(acm->scheduler, i, false);
		else
			scheduler_set_active(acm->scheduler, i, true);

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i)
		bypass_set_active(acm->bypass[i],
				  bypass_is_enabled(acm->bypass[i]));

	*new_status = ACMDRV_RUN_STATE;
	return 0;
}

/**
 * @brief stop running ACM config
 *
 * @param acm ACM instance
 * @param new_status Returns new ACM state
 * @return Returns 0 on success, negative error code otherwise.
 */
int acm_config_stop_running(struct acm *acm, enum acmdrv_status *new_status)
{
	int i;

	for (i = 0; i < ACMDRV_SCHEDULER_COUNT; ++i)
		scheduler_write_emergency_disable(acm->scheduler, i, 1);

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i)
		bypass_enable(acm->bypass[i], false);

	*new_status = ACMDRV_DESYNC_STATE;
	return 0;
}

/**
 * @brief restart ACM config
 *
 * @param acm ACM instance
 * @param new_status Returns new ACM state
 * @return Returns 0 on success, negative error code otherwise.
 */
int acm_config_restart_running(struct acm *acm, enum acmdrv_status *new_status)
{
	int i;

	/* Set the enable NGN modules and Schedulers accordingly */
	for (i = 0; i < ACMDRV_SCHEDULER_COUNT; ++i) {
		if (scheduler_get_active(acm->scheduler, i))
			scheduler_write_emergency_disable(acm->scheduler, i, 0);
		else
			scheduler_write_emergency_disable(acm->scheduler, i, 1);
	}

	for (i = 0; i < ACMDRV_BYPASS_MODULES_COUNT; ++i)
		bypass_enable(acm->bypass[i],
			      bypass_get_active(acm->bypass[i]));


	*new_status = ACMDRV_RUN_STATE;
	return 0;
}
/**@} logicctrl */

