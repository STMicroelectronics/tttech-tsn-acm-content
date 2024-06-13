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
 * @file acm-module.h
 * @brief ACM Driver Internal Data Representation
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_MODULE_H_
#define ACM_MODULE_H_

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/bitsperlong.h>

#include "api/acmdrv.h"

/**
 * @brief interface variant enumeration
 */
enum acm_ip_if_variant {
	ACM_IF_1_0,	/**< interface id for ttt,acm-1.0 */
	ACM_IF_2_0,	/**< interface id for ttt,acm-2.0 */
	ACM_IF_3_0,	/**< interface id for ttt,acm-3.0: new status regs */
	ACM_IF_4_0,	/**< interface id for ttt,acm-4.0 */
	ACM_IF_DONT_CARE
};

/**
 * @struct acm
 * @brief ACM instance data
 *
 * @var acm::dev
 * @brief the underlying device that provides the ACM sysfs interface
 *
 * @var acm::pdev
 * @brief the associated platform device
 *
 * @var acm::bypass
 * @brief array of bypass module handlers
 *
 * @var acm::scheduler
 * @brief scheduler module handler
 *
 * @var acm::redundancy
 * @brief redundancy module handler
 *
 * @var acm::msgbuf
 * @brief redundancy module handler
 *
 * @var acm::commreg
 * @brief common register block handling
 *
 * @var acm::reset
 * @brief reset control
 *
 * @var acm::netdev_nb
 * @brief notifier block for netdev events
 *
 * @var acm::status
 * @brief ACM status handling
 *
 * @var acm::devt
 * @brief base device number of message buffer character devices
 *
 * @var acm::devices
 * @brief list of message buffer character devices for data transfer
 *
 * @var acm::if_id
 * @brief interface identifier
 *
 */
struct acm {
	struct device		dev;
	struct platform_device	*pdev;

	struct bypass		*bypass[ACMDRV_BYPASS_MODULES_COUNT];
	struct scheduler	*scheduler;
	struct redundancy	*redundancy;
	struct msgbuf		*msgbuf;
	struct commreg		*commreg;
	struct reset		*reset;

	struct notifier_block	netdev_nb;

	struct acm_state	*status;

	dev_t			devt;
	struct acm_dev		*devices;

	enum acm_ip_if_variant	if_id;
};

/**
 * @brief device driver class object for ACM
 */
extern struct class *acm_class;

/**
 * @brief Provide struct acm pointer from kernel object pointer
 */
static inline struct acm *kobj_to_acm(struct kobject *kobj)
{
	return container_of(kobj, struct acm, dev.kobj);
}

/**
 * @brief Provide struct acm pointer from device pointer
 */
static inline struct acm *dev_to_acm(struct device *dev)
{
	return container_of(dev, struct acm, dev);
}

/**
 * @brief accessor to underlying device
 */
static inline struct device *acm_dev(struct acm *acm)
{
	return &acm->pdev->dev;
}
#endif /* ACM_MODULE_H_ */
