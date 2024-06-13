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
 * @file sysfs.c
 * @brief ACM Driver SYSFS Interface
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup acmsysfsimpl ACM SYSFS Implementation
 * @brief Implementation of SYSFS Interface
 *
 * @{
 */

#include <linux/sysfs.h>
#include "acm-module.h"
#include "sysfs.h"
#include "sysfs_control.h"
#include "sysfs_config.h"
#include "sysfs_status.h"
#include "sysfs_error.h"
#include "sysfs_diag.h"

/**
 * Linux kernel attribute group array for ACM's sysfs interface
 */
static const struct attribute_group *attribute_groups[6];

/**
 * @brief Check parameters when accessing binary sysfs attribute
 *
 * @param bin_attr pointer to binary attribute
 * @param loff data offset for access
 * @param totalsize amount of bytes to be accessed
 * @param elsize size of a single item that can be accessed
 * @return negative error code on failure, else 0
 */
int sysfs_bin_attr_check(struct bin_attribute *bin_attr, loff_t loff,
			 size_t totalsize, size_t elsize)
{
	off_t off;

	/* no support for large offsets, only positive offsets are used */
	if ((loff < 0) || (loff > LONG_MAX)) {
		pr_err("%s: Offset out of range: %lld\n", bin_attr->attr.name,
		       loff);
		return -EOVERFLOW;
	}

	/* safely use off_t to avoid 64bit operation */
	off = (off_t)loff;

	/* access exceeds maximum */
	if (totalsize + off > bin_attr->size) {
		pr_err("%s: Trying to access %zu bytes with offset %lu: only %zu bytes available\n",
		       bin_attr->attr.name, totalsize, off, bin_attr->size);
		return -EOVERFLOW;
	}

	/* we only can access multiple of elsize */
	if (totalsize % elsize != 0) {
		pr_err("%s: Required length (%zu) not aligned to %zu\n",
		       bin_attr->attr.name, totalsize, elsize);
		return -EINVAL;
	}

	/* we only can access at elsize boundaries */
	if (off % elsize != 0) {
		pr_err("%s: Offset (%lu) not aligned to %zu\n",
		       bin_attr->attr.name, off, elsize);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief initialize ACM sysfs handling
 */
int __must_check acm_sysfs_init(struct acm *acm)
{
	const struct attribute_group *agroup;
	int ret;
	int idx = 0;

	agroup = sysfs_control_init(acm);
	if (IS_ERR(agroup)) {
		ret = PTR_ERR(agroup);
		goto sysfs_control_init_err;
	}
	if (agroup)
		attribute_groups[idx++] = agroup;

	agroup = sysfs_config_init(acm);
	if (IS_ERR(agroup)) {
		ret = PTR_ERR(agroup);
		goto sysfs_config_init_err;
	}
	if (agroup)
		attribute_groups[idx++] = agroup;

	agroup = sysfs_error_init(acm);
	if (IS_ERR(agroup)) {
		ret = PTR_ERR(agroup);
		goto sysfs_error_init_err;
	}
	if (agroup)
		attribute_groups[idx++] = agroup;

	agroup = sysfs_status_init(acm);
	if (IS_ERR(agroup)) {
		ret = PTR_ERR(agroup);
		goto sysfs_status_init_err;
	}
	if (agroup)
		attribute_groups[idx++] = agroup;

	agroup = sysfs_diag_init(acm);
	if (IS_ERR(agroup)) {
		ret = PTR_ERR(agroup);
		goto sysfs_diag_init_err;
	}
	if (agroup)
		attribute_groups[idx++] = agroup;

	attribute_groups[idx] = NULL;

	ret = sysfs_create_groups(&acm->dev.kobj, attribute_groups);
	if (ret)
		goto sysfs_create_groups_err;

	return 0;

sysfs_create_groups_err:
	sysfs_diag_exit(acm);
sysfs_diag_init_err:
	sysfs_status_exit(acm);
sysfs_status_init_err:
	sysfs_error_exit(acm);
sysfs_error_init_err:
	sysfs_config_exit(acm);
sysfs_config_init_err:
	sysfs_control_exit(acm);
sysfs_control_init_err:
	return ret;
}

/**
 * @brief exit ACM sysfs handling
 */
void acm_sysfs_exit(struct acm *acm)
{
	struct platform_device *pdev = acm->pdev;

	sysfs_remove_groups(&pdev->dev.kobj, attribute_groups);
	sysfs_status_exit(acm);
	sysfs_error_exit(acm);
	sysfs_config_exit(acm);
	sysfs_control_exit(acm);
}
/**@} acmsysfsimpl */

