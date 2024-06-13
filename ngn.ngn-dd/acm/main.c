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
 * @file main.c
 * @brief ACM Driver Main Module
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 *
 * @mainpage Acceleration Control Module Device Driver
 * @section intro_sec Introduction
 * The ACM Device Driver provides kernel-space components responsible to
 * establish controlled access to the ACM IP hardware. The communication between
 * kernel and the IP takes place via  memory mapping. Thus IP memory and
 * registers are accessible via a memory read/write calls.
 *
 * The following limitations are taken into account by the driver:
 *   - ensure correctly aligned access to ACM IP (either 32bit or 16bit)
 *   - do not write to readonly registers
 *
 * The driver's API related stuff can be found at
 * @ref acmapi "the ACM kernel module's userspace API".
 *
 * The device driver consists of the following parts:
 *
 *   - @ref acmsysfs "Status, Control & Configuration": This part is
 *     responsible for configuring the requested streams and subsequent
 *     associated message buffers. Regarding user interface, a dedicated device
 *     node is created dynamically for each configured message buffer.
 *     Furthermore error- and status information is provided.
 *     This interfaces are provided via sysfs in respective sections.
 *     Since the ACM device derives its name from the devictree node name, the
 *     base directory for the sysfs interface results in <b><tt>
 *     /sys/devices/\<devicetree nodename\></tt></b>, so it is
 *     <b><tt>/sys/devices/ngn</tt></b> in the example below. This
 *     provides at least some configurability of the parent directory name.
 *
 *   - @ref communication "Communication": This part of the device driver is
 *     instantiated for each configured message buffer, each of which them uses
 *     its own, dedicated device node. It reads and/or writes data to the
 *     corresponding IP message buffer.
 *
 *   - Hardware Access: encapsulates the access to the respective ACM IP
 *     registers and provides access to the following parts:
 *     - @ref hwaccbypass "Bypass Modules": there are two of them, Bypass0
 *       controlling the bypassed
 *       traffic from port 1 to 0, and vice versa Bypass1.
 *     - @ref hwaccredund "Redundancy Module": Controlling the redundant paths
 *       of frames
 *     - @ref hwaccsched "Scheduler Module": there a two schedulers with two
 *       scheduling tables each to control the traffic scheduling
 *     - @ref hwaccmsgbuf "Messagebuffer Module": provides 32 entry/exit points
 *       for the cut-through streams
 *
 *     <br>Since ACM IP 0.9.28 there are the following additional parts:
 *     - @ref hwacccommreg "Common Register Block": contains various info
 *       registers about ACM IP and controls resetting it via @ref hwaccreset
 *       "Reset Control"
 *
 *   - @ref logicctrl "Logic Control": Handles state transitions and IP
 *     configuration.
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of_platform.h>
#include <linux/fs.h>
#include <linux/stringify.h>

#include "acm-module.h"
#include "sysfs.h"
#include "chardev.h"
#include "state.h"
#include "bypass.h"
#include "scheduler.h"
#include "redundancy.h"
#include "msgbuf.h"
#include "reset.h"
#include "commreg.h"

#include <linux/delay.h>

struct class *acm_class;

/**
 * @brief Linux device driver release function
 */
static void acm_release(struct device *dev)
{
	/* nothing to do */
}

/**
 * @brief Linux kernel device tree ID table
 */
static const struct of_device_id acm_dt_ids[] = {
	{ .compatible = "ttt,acm-1.0", .data = (void *)ACM_IF_1_0 },
	{ .compatible = "ttt,acm-2.0", .data = (void *)ACM_IF_2_0 },
	{ .compatible = "ttt,acm-3.0", .data = (void *)ACM_IF_3_0 },
	{ .compatible = "ttt,acm-4.0", .data = (void *)ACM_IF_4_0 },
	{ /* sentinel */ }
};

/**
 * @brief Linux kernel device table macro
 */
MODULE_DEVICE_TABLE(of, acm_dt_ids);

/**
 * @brief Get ACM IP version
 */
static const char *acm_get_version(struct acm *acm)
{
	if (!acm->commreg)
		return NULL;

	return commreg_read_version(acm->commreg);
}

/**
 * @brief Get ACM IP revision
 */
static const char *acm_get_revision(struct acm *acm)
{
	if (!acm->commreg)
		return NULL;

	return commreg_read_revision(acm->commreg);
}

/**
 * @brief Linux Platform Driver Probe Function
 */
static int acm_probe(struct platform_device *pdev)
{
	struct acm *acm;
	int ret = 0;
	size_t buffers;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id = NULL;
	const char *version, *revision;

	dev_info(dev, "Version %s", __stringify(ACMDRV_VERSION));
	/* check for existing DT node */
	if (!np) {
		dev_err(dev, "device tree configuration required");
		return -EINVAL;
	}

	acm = devm_kzalloc(dev, sizeof(*acm), GFP_KERNEL);
	if (!acm) {
		ret = -ENOMEM;
		goto out;
	}

	acm->pdev = pdev;
	platform_set_drvdata(pdev, acm);

	of_id = of_match_device(acm_dt_ids, &pdev->dev);
	if (of_id)
		acm->if_id = (enum acm_ip_if_variant)of_id->data;
	else {
		dev_err(dev, "Unmatched ACM IP variant");
		ret = -ENODEV;
		goto out;
	}

	/* Common Register Block must be initialize first, so we can use it */
	dev_info(dev, "commreg_init");
        udelay(500);
	ret = commreg_init(acm);
	if (ret)
		goto out;
	dev_info(dev, "reset_init");
        udelay(500);
	ret = reset_init(acm, np);
	if (ret)
		goto out_commreg;
	dev_info(dev, "bypass_init");
        udelay(500);

	ret = bypass_init(acm);
	if (ret)
		goto out_reset;
	dev_info(dev, "scheduler_init");
        udelay(500);

	ret = scheduler_init(acm);
	if (ret)
		goto out_bypass;
	dev_info(dev, "redundancy_init");
        udelay(500);

	ret = redundancy_init(acm);
	if (ret)
		goto out_scheduler;
dev_info(dev, "msgbuf_init");
        udelay(500);

	ret = msgbuf_init(acm);
	if (ret)
		goto out_redundancy;

	/* create ACM device */
	acm->dev.release = acm_release;
dev_info(dev, "dev_set_name");
        udelay(500);

	ret = dev_set_name(&acm->dev, np->name);
	if (ret)
		goto out_msgbuf;
dev_info(dev, "device_register");
        udelay(500);

	ret = device_register(&acm->dev);
	if (ret)
		goto out_msgbuf;

	/*
	 * reserve the respective character devices, one for each message
	 * buffer
	 */
dev_info(dev, "commreg_read_msgbuf_count");
        udelay(500);

	buffers = commreg_read_msgbuf_count(acm->commreg);
dev_info(dev, "alloc_chrdev_region");
        udelay(500);

	ret = alloc_chrdev_region(&acm->devt, 0, buffers, ACMDRV_NAME);
	if (ret) {
		dev_err(dev, "Cannot allocate char devices: %d", ret);
		goto out_unreg_device;
	}

	/* create associated ACM devices */
dev_info(dev, "acm_dev_create");
        udelay(500);

	acm->devices = acm_dev_create(acm, buffers);
	if (IS_ERR(acm->devices)) {
dev_info(dev, "unregister_chrdev_region");
        udelay(500);
		unregister_chrdev_region(acm->devt, buffers);
		ret = PTR_ERR(acm->devices);
		goto unregister_chrdev;
	}
dev_info(dev, "acm_sysfs_init");
        udelay(500);

	ret = acm_sysfs_init(acm);
	if (ret)
		goto dev_destroy;

dev_info(dev, "acm_sysfs_init");
        udelay(500);
	ret = acm_state_init(acm);
	if (ret)
		goto sysfs_exit;
dev_info(dev, "acm_get_version");
        udelay(500);

	version = acm_get_version(acm);
dev_info(dev, "acm_get_revision");
        udelay(500);
	revision = acm_get_revision(acm);

	if (version && revision)
		dev_info(dev, "ACM IP %s (%s) probed successfully", version,
			 revision);
	else
		dev_info(dev, "ACM IP probed successfully");

	goto out;

sysfs_exit:
	acm_sysfs_exit(acm);
dev_destroy:
	acm_dev_destroy(acm);
unregister_chrdev:
	unregister_chrdev_region(acm->devt, buffers);
out_unreg_device:
	device_unregister(&acm->dev);
out_msgbuf:
	msgbuf_exit(acm);
out_redundancy:
	redundancy_exit(acm);
out_scheduler:
	scheduler_exit(acm);
out_bypass:
	bypass_exit(acm);
out_reset:
	reset_exit(acm);
out_commreg:
	commreg_exit(acm);
out:
	return ret;
}

/**
 * @brief Linux Platform Driver Remove Function
 */
static int acm_remove(struct platform_device *pdev)
{
	struct acm *acm = platform_get_drvdata(pdev);

	acm_state_exit(acm);
	acm_dev_destroy(acm);
	unregister_chrdev_region(acm->devt,
				 commreg_read_msgbuf_count(acm->commreg));
	device_unregister(&acm->dev);
	msgbuf_exit(acm);
	redundancy_exit(acm);
	scheduler_exit(acm);
	bypass_exit(acm);
	reset_exit(acm);
	commreg_exit(acm);

	return 0;
}

/**
 * @brief Linux kernel platform driver instance
 */
static struct platform_driver acm_driver = {
	.probe		= acm_probe,
	.remove		= acm_remove,
	.driver		= {
		.name		= ACMDRV_NAME,
		.of_match_table	= of_match_ptr(acm_dt_ids),
	},
};

/**
 * @brief Linux kernel module initialization function
 */
static int __init acm_module_init(void)
{
	int ret;

	acm_class = class_create(THIS_MODULE, ACMDRV_NAME);
	if (IS_ERR_OR_NULL(acm_class)) {
		ret = PTR_ERR(acm_class);
		pr_err("class_create() failed: %d\n", ret);
		goto out;
	}

	ret = platform_driver_register(&acm_driver);
	if (ret) {
		pr_err("platform_driver_register() failed: %d\n", ret);
		goto out_class_destroy;
	}
	return ret;

out_class_destroy:
	class_destroy(acm_class);
out:
	return ret;
}

/**
 * @brief Linux kernel module exit function
 */
static void __exit acm_module_exit(void)
{
	platform_driver_unregister(&acm_driver);
	class_destroy(acm_class);
}

module_init(acm_module_init);
module_exit(acm_module_exit);

/**
 * @brief Linux kernel Module License
 */
MODULE_LICENSE("GPL v2");
/**
 * @brief Linux kernel Module Description
 */
MODULE_DESCRIPTION("TTTech Acceleration Control Module");
/**
 * @brief Linux kernel Module Author
 */
MODULE_AUTHOR("Helmut Buchsbaum");
/**
 * @brief Linux kernel Module Alias
 */
MODULE_ALIAS("platform:acm");

/**
 * @brief Linux kernel Module Version
 */
MODULE_VERSION(__stringify(ACMDRV_VERSION));
