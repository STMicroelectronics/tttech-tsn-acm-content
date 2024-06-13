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
 * @file sysfs_error.c
 * @brief ACM Driver SYSFS - Error Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup acmsysfserrorimpl ACM SYSFS Error Implementation
 * @brief Implementation of SYSFS Error Section
 *
 * @{
 */

#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include "sysfs_error.h"
#include "acm-module.h"
#include "bypass.h"
#include "acmbitops.h"

/**
 * @brief extended device attribute for error attributes
 */
struct sysfs_err_attribute {
	off_t offset;		/**< status area address offset of the item */
	int index;		/**< bypass module index */
	u64 bitmask;		/**< access bit mask */

	struct device_attribute dev_attr; /**< Linux device attribute base */
};

/**
 * @def ACM_BYPASS_ERR_ATTR_RO
 * @brief static initializer helper macro for struct sysfs_err_attribute
 *
 * @param _name base name of the attribute
 * @param _offs register offset to get the value from
 * @param _bitmask bitmask of the value
 */
#define ACM_BYPASS_ERR_ATTR_RO(_name, _offs, _bitmask)		\
struct sysfs_err_attribute sysfs_err_attr_##_name##_M0 =	\
{								\
	.offset		= (_offs),				\
	.index		= 0,					\
	.bitmask	= (_bitmask),				\
	.dev_attr	= __ATTR(_name##_M0, 0444,		\
				 sysfs_error_attr_show,		\
				 NULL),				\
};								\
struct sysfs_err_attribute sysfs_err_attr_##_name##_M1 =	\
{								\
	.offset		= (_offs),				\
	.index		= 1,					\
	.bitmask	= (_bitmask),				\
	.dev_attr	= __ATTR(_name##_M1, 0444,		\
				 sysfs_error_attr_show,		\
				 NULL),				\
}

/**
 * @brief Common show function for Error attributes
 */
static ssize_t sysfs_error_attr_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	u32 reg_lo;
	u32 val;
	unsigned long hibit;
	struct acm *acm;
	struct sysfs_err_attribute *err_attr;

	acm = dev_to_acm(dev);
	err_attr = container_of(attr, struct sysfs_err_attribute, dev_attr);
	hibit = fls64(err_attr->bitmask);

	reg_lo = bypass_status_area_read(acm->bypass[err_attr->index],
					 err_attr->offset);

	if (hibit >= 32) {
		u64 val64;
		u32 reg_hi;

		reg_hi = bypass_status_area_read(acm->bypass[err_attr->index],
						 err_attr->offset + 4);

		val64 = reg_hi;
		val64 <<= 32;
		val64 += reg_lo;

		val64 = read_bitmask64(&val64, err_attr->bitmask);
		return scnprintf(buf, PAGE_SIZE, "0X%llX\n", val64);
	}

	val = read_bitmask(&reg_lo, err_attr->bitmask);

	if (__sw_hweight64(err_attr->bitmask) == 1)
		return scnprintf(buf, PAGE_SIZE, "%X\n", val);

	return scnprintf(buf, PAGE_SIZE, "0X%X\n", val);
}

/**
 * @brief Error attribute halt_on_error
 */
static ACM_BYPASS_ERR_ATTR_RO(halt_on_error,
			      ACM_BYPASS_STATUS_AREA_HALT_ERROR, HALT_ON_ERROR);
/**
 * @brief Error attribute halt_on_other_bypass
 */
static ACM_BYPASS_ERR_ATTR_RO(halt_on_other_bypass,
			      ACM_BYPASS_STATUS_AREA_HALT_ERROR, HALT_ON_OTHER);
/**
 * @brief Error attribute error_flags
 */
/* mask set at init depending on IP version */
static ACM_BYPASS_ERR_ATTR_RO(error_flags,
			      ACM_BYPASS_STATUS_AREA_IP_ERR_FLAGS_LOW, 0);
/**
 * @brief Error attribute policing_flags
 */
static ACM_BYPASS_ERR_ATTR_RO(policing_flags,
			      ACM_BYPASS_STATUS_AREA_POLICING_ERR_FLAGS,
			      GENMASK(2, 0));

/**
 * @brief sysfs attributes of error section
 */
static struct attribute *error_attributes[] = {
	&sysfs_err_attr_halt_on_error_M0.dev_attr.attr,
	&sysfs_err_attr_halt_on_error_M1.dev_attr.attr,
	&sysfs_err_attr_halt_on_other_bypass_M0.dev_attr.attr,
	&sysfs_err_attr_halt_on_other_bypass_M1.dev_attr.attr,
	&sysfs_err_attr_error_flags_M0.dev_attr.attr,
	&sysfs_err_attr_error_flags_M1.dev_attr.attr,
	&sysfs_err_attr_policing_flags_M0.dev_attr.attr,
	&sysfs_err_attr_policing_flags_M1.dev_attr.attr,
	NULL
};

/**
 * @brief Linux attribute group for error section
 */
static const struct attribute_group error_group = {
	.name = __stringify(ACMDRV_SYSFS_ERROR_GROUP),
	.attrs = error_attributes,
};

/**
 * @brief initialize sysfs error section
 *
 * @param acm ACM instance
 */
const struct attribute_group *sysfs_error_init(struct acm *acm)
{
	/* ACM_IF_1_0 only provides 39bit error flags */
	if (acm->if_id == ACM_IF_1_0) {
		/* ttt,acm-1.0 */
		sysfs_err_attr_error_flags_M0.bitmask = GENMASK_ULL(39, 0);
		sysfs_err_attr_error_flags_M1.bitmask = GENMASK_ULL(39, 0);
	} else {
		/* ttt,acm-2.0 and above */
		sysfs_err_attr_error_flags_M0.bitmask = GENMASK_ULL(40, 0);
		sysfs_err_attr_error_flags_M1.bitmask = GENMASK_ULL(40, 0);
	}
	return &error_group;
}

/**
 * exit sysfs error section
 *
 * @param acm ACM instance
 */
void sysfs_error_exit(struct acm *acm)
{
	/* nothing to do */
}

/**@} acmsysfserrorimpl*/

