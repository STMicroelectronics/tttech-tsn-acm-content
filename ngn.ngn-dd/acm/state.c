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
 * @file state.c
 * @brief ACM Driver State Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @addtogroup logicctrl
 *
 * @{
 */
#include <linux/mutex.h>

#include "acm-module.h"
#include "state.h"
#include "config.h"

/**
 * @brief ACM state handler instance
 */
struct acm_state {
	struct acm *acm;	/**< associated ACM instance */

	struct mutex lock;	/**< state access lock */
	enum acmdrv_status status;	/**< ACM state */
};

/**
 * Getter method for ACM state
 */
enum acmdrv_status acm_state_get(struct acm_state *state)
{
	return state->status;
}

/**
 * Setter method for ACM state
 */
int acm_state_set(struct acm_state *state, enum acmdrv_status status)
{
	int ret;
	enum acmdrv_status new_status;

	ret = mutex_lock_interruptible(&state->lock);
	if (ret)
		return ret;

	switch (status) {
	case ACMDRV_CONFIG_START_STATE:
		ret = acm_config_start(state->acm, &new_status);
		break;
	case ACMDRV_CONFIG_END_STATE:
		ret = acm_config_end(state->acm, &new_status);
		break;
	case ACMDRV_DESYNC_STATE:
		ret = acm_config_stop_running(state->acm, &new_status);
		break;
	case ACMDRV_RESTART_STATE:
		ret = acm_config_restart_running(state->acm, &new_status);
		break;
	case ACMDRV_INIT_STATE:
		new_status = status;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret == 0)
		state->status = new_status;

	mutex_unlock(&state->lock);

	return ret;
}

/**
 * check is ACM is in running state
 */
bool acm_state_is_running(struct acm_state *state)
{
	return state->status == ACMDRV_RUN_STATE;
}

/**
 * @brief initialize ACM state handler
 */
int __must_check acm_state_init(struct acm *acm)
{
	struct acm_state *state;
	struct platform_device *pdev = acm->pdev;
	struct device *dev = &pdev->dev;

	state = devm_kzalloc(dev, sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	state->acm = acm;

	mutex_init(&state->lock);
	state->status = ACMDRV_INIT_STATE;

	acm->status = state;

	return 0;
}

/**
 * @brief exit ACM state handler
 */
void acm_state_exit(struct acm *acm)
{
	/* nothing to do */
}
/**@} logicctrl */
