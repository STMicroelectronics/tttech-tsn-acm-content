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
 * @file reset.c
 * @brief ACM Driver Reset Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup hwaccreset ACM IP Reset Control
 * @brief Controls resetting the ACM IP
 *
 * @{
 */

#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/err.h>

#include "acm-module.h"
#include "commreg.h"

/**
 * @brief reset wait time for register based reset
 */
#define RESET_WAIT_US	1

/**
 * @brief maximum delay retries waiting for reset-in (gpio only)
 */
#define ACM_RST_MAX_RETRY	16

/**
 * @brief structure to maintain reset control instance
 */
struct reset {
	struct acm *acm;			/**< associated ACM instance */
	struct gpio_desc *gpios[2];		/**< reset GPIO descriptors */

	int (*init)(struct reset *rst);		/**< init func*/
	int (*trigger)(struct reset *trig);	/**< trigger reset func */
};

/**
 * @brief trigger GPIO based reset
 */
static int reset_gpio(struct reset *reset)
{
	int ret = -ETIMEDOUT;

	struct gpio_desc *rstout = reset->gpios[0];
	struct gpio_desc *rstin = reset->gpios[1];

	if (IS_ERR(rstout)) {
		dev_warn(&reset->acm->pdev->dev, "No reset-out GPIO configured, skip reset\n");
		return 0;
	}

	gpiod_set_value(rstout, 1);

	if (!IS_ERR(rstin)) {
		int retry;

		for (retry = 0; !gpiod_get_value(rstin); ++retry) {
			if (retry >= ACM_RST_MAX_RETRY)
				goto reset_deact;
			udelay(1);
		}

		gpiod_set_value(rstout, 0);

		for (retry = 0; gpiod_get_value(rstin); ++retry) {
			if (retry >= ACM_RST_MAX_RETRY)
				goto out;
			udelay(1);
		}
		ret = 0;
	} else {
		dev_warn(&reset->acm->pdev->dev, "No reset-in GPIO configured, using reset-out pulse only\n");
		/* no reset in configured, use simple delay */
		udelay(5);
		ret = 0;
	}

reset_deact:
	gpiod_set_value(rstout, 0);
out:
	return ret;
}

/**
 * @brief initialize GPIO based reset control
 */
static int reset_gpio_init(struct reset *reset)
{
	int i;
	struct device *dev = &reset->acm->pdev->dev;

	for (i = 0; i < 2; ++i) {

		enum gpiod_flags flags = GPIOD_OUT_LOW;
		if (i == 1) {
			flags = GPIOD_IN;
		}
		reset->gpios[i] = gpiod_get_index(dev, "reset", i, flags);
		if (IS_ERR(reset->gpios[i])) {
			dev_warn(dev, "No ACM GPIO %i configured: %ld\n",
				i, PTR_ERR(reset->gpios[i]));
		}
	}

	/* de-assert reset-out */
	gpiod_set_value(reset->gpios[0], 0);

	return 0;
}

/**
 * @brief trigger register based reset
 */
static int reset_register(struct reset *reset)
{
	commreg_reset(reset->acm->commreg);
	udelay(RESET_WAIT_US);

	return 0;
}

/**
 * @brief initialize register based reset control
 */
static int reset_register_init(struct reset *reset)
{
	return reset_register(reset);
}

/**
 * @brief initialize reset control
 */
int __must_check reset_init(struct acm *acm)
{
	struct reset *reset;
	struct device *dev = &acm->pdev->dev;

	reset = devm_kzalloc(dev, sizeof(*reset), GFP_KERNEL);
	if (!reset)
		return -ENOMEM;

	reset->acm = acm;
	acm->reset = reset;

	/* only if variant 0 has GPIO support */
	if (acm->if_id == ACM_IF_1_0) {
		reset->init = reset_gpio_init;
		reset->trigger = reset_gpio;
	} else {
		reset->init = reset_register_init;
		reset->trigger = reset_register;
	}
	return reset->init(reset);
}

/**
 * @brief exit reset control
 */
void reset_exit(struct acm *acm)
{
	/* nothing to do */
}

/**
 * @brief reset entire ACM IP
 */
int reset_trigger(struct reset *reset)
{
	return reset->trigger(reset);
}


/**@} hwaccreset*/
