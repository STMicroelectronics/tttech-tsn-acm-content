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

#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>

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
 * @brief helper structure to maintain reset gpios
 */
struct reset_gpio {
	int		gpio;	/**< GPIO number */
	unsigned long	flags;	/**< GPIO flags */
	const char	*label;	/**< Label */
};

/**
 * @brief structure to maintain reset control instance
 */
struct reset {
	struct acm *acm;		/**< associated ACM instance */
	struct reset_gpio gpios[2];	/**< reset GPIO data */

	int (*init)(struct reset *rst, struct device_node *np); /**< init func*/
	int (*trigger)(struct reset *trig);	/**< trigger reset func */
};

/**
 * @brief trigger GPIO based reset
 */
static int reset_gpio(struct reset *reset)
{
	int ret = -ETIMEDOUT;

	int rstout = reset->gpios[0].gpio;
	int rstin = reset->gpios[1].gpio;
	struct gpio_desc *rstout_desc;


	if (!gpio_is_valid(rstout))
		return 0;

	rstout_desc = gpio_to_desc(rstout);
	gpiod_set_value(rstout_desc, 1);

	if (gpio_is_valid(rstin)) {
		struct gpio_desc *rstin_desc = gpio_to_desc(rstin);
		int retry;

		for (retry = 0; !gpiod_get_value(rstin_desc); ++retry) {
			if (retry >= ACM_RST_MAX_RETRY)
				goto reset_deact;
			udelay(1);
		}

		gpiod_set_value(rstout_desc, 0);

		for (retry = 0; gpiod_get_value(rstin_desc); ++retry) {
			if (retry >= ACM_RST_MAX_RETRY)
				goto out;
			udelay(1);
		}
		ret = 0;
	} else {
		dev_warn_once(&reset->acm->pdev->dev, "No reset-in configured, using reset-out pulse only\n");
		/* no reset in configured, use simple delay */
		udelay(5);
		ret = 0;
	}

reset_deact:
	gpiod_set_value(rstout_desc, 0);
out:
	return ret;
}

/**
 * @brief initialize GPIO based reset control
 */
static int reset_gpio_init(struct reset *reset, struct device_node *np)
{
	int i, ret;
	struct device *dev = &reset->acm->pdev->dev;

	reset->gpios[0].flags = GPIOF_OUT_INIT_LOW;
	reset->gpios[0].label = "acm-rst-out";
	reset->gpios[1].flags = GPIOF_DIR_IN;
	reset->gpios[1].label = "acm-rst-in";

	for (i = 0; i < 2; ++i) {
		enum of_gpio_flags flags;

		reset->gpios[i].gpio = -1;
		reset->gpios[i].gpio = of_get_named_gpio_flags(np,
			"reset-gpios", i, &flags);

		if (gpio_is_valid(reset->gpios[i].gpio)) {

			if (flags == OF_GPIO_ACTIVE_LOW)
				reset->gpios[i].flags |= GPIOF_ACTIVE_LOW;
			else
				reset->gpios[i].flags &= ~GPIOF_ACTIVE_LOW;

			ret = devm_gpio_request_one(dev, reset->gpios[i].gpio,
				reset->gpios[i].flags, reset->gpios[i].label);

			if (ret) {
				dev_err(dev, "failed to get %s: %d\n",
						reset->gpios[i].label, ret);
				return ret;
			}
		} else {
			dev_warn(dev, "no %s configured\n",
				 reset->gpios[i].label);
		}
	}


	/* de-assert reset-out */
	if (gpio_is_valid(reset->gpios[0].gpio))
		gpiod_set_value(gpio_to_desc(reset->gpios[0].gpio), 0);

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
static int reset_register_init(struct reset *reset, struct device_node *np)
{
	return reset_register(reset);
}

/**
 * @brief initialize reset control
 */
int __must_check reset_init(struct acm *acm, struct device_node *np)
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
	return reset->init(reset, np);
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
