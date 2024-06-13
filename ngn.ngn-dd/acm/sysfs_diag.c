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
 * @file sysfs_diag.c
 * @brief ACM Driver SYSFS - Diagnostics Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @brief kernel pr_* format macro
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

/**
 * @defgroup acmsysfsdiagimpl ACM SYSFS Diagnostics Implementation
 * @brief Implementation of SYSFS Diagnostics Section
 *
 * @{
 */
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/div64.h>

#include "sysfs_diag.h"
#include "acm-module.h"
#include "bypass.h"
#include "sysfs.h"

/**
 * @brief extended binary device attribute for binary diagnostic attributes
 */
struct diag_bin_attribute {
	int index; /**< bypass module index */
	struct bin_attribute bin_attr; /**< Linux binary attribute base */
};

/**
 * @brief extended device attribute for diagnostic device attributes
 */
struct diag_device_attribute {
	int index; /**< bypass module index */
	struct device_attribute dev_attr; /**< Linux device attribute base */
};

/**
 * @def ACM_DIAG_BINATTR_RW
 * @brief static initializer helper macro for struct diag_bin_attribute
 *
 * @param _name base name of the attribute
 * @param _size size of binary data
 */
#define ACM_DIAG_BINATTR_RW(_name, _size)			\
static struct diag_bin_attribute diag_binattr_##_name##_M0 =	\
{								\
	.index		= 0,					\
	.bin_attr	= __BIN_ATTR(_name##_M0, 0644,		\
				_name##_read, _name##_write,	\
				_size),				\
};								\
static struct diag_bin_attribute diag_binattr_##_name##_M1 =	\
{								\
	.index		= 1,					\
	.bin_attr	= __BIN_ATTR(_name##_M1, 0644,		\
				_name##_read, _name##_write,	\
				_size),				\
}

/**
 * @def ACM_DIAG_ATTR_RW
 * @brief static initializer helper macro for struct diag_device_attribute
 *
 * @param _name base name of the attribute
 */
#define ACM_DIAG_ATTR_RW(_name)						\
static struct diag_device_attribute diag_devattr_##_name##_M0 =		\
{									\
	.index		= 0,						\
	.dev_attr	= __ATTR(_name##_M0, 0644,			\
					_name##_show, _name##_store),	\
};									\
static struct diag_device_attribute diag_devattr_##_name##_M1 =		\
{									\
	.index		= 1,						\
	.dev_attr	= __ATTR(_name##_M1, 0644,			\
					_name##_show, _name##_store),	\
}

/**
 * @brief Read function for for diagnostic data
 *
 * Reads the data of the array of struct acmdrv_diagnostics data field,
 * All reads must access on struct acmdrv_diagnostics data size boundaries
 * (representing the respective module's diagnostic data) with a multiple of
 * struct acmdrv_diagnostics data size length, thus providng access to
 * diagnostics of all modules at once.
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr binary attribute given
 * @param buf Data buffer to read into
 * @param off Offset where to read the data from within the binary attribute
 * @param size Amount of data to read
 * @return Return number of bytes read into the buffer or negative error id
 */
static ssize_t diagnostics_read(struct file *file, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t size)
{
	int ret;
	struct acm *acm = kobj_to_acm(kobj);
	const size_t data_size = sizeof(struct acmdrv_diagnostics);
	struct diag_bin_attribute *diagnostics;

	diagnostics = container_of(bin_attr, struct diag_bin_attribute,
		bin_attr);

	if (size == 0)
		return 0;

	if (off != 0 || size < data_size)
		return -EINVAL;

	ret = bypass_diag_read(acm->bypass[diagnostics->index], (void *)buf);
	if (ret)
		return ret;

	return data_size;
}

/**
 * @brief write function for diagnostic data
 *
 * Any write access resets the respective diagnostic fields, Since the entire
 * diagnostic data consist of ACMDRV_BYPASS_MODULES_COUNT sets of data, any
 * access within the respective data set resets th respective module's
 * diagnostic data
 *
 * @param file Standard parameter not used in here
 * @param kobj Kernel object the attribute belongs to
 * @param bin_attr Binary attribute given
 * @param buf Data buffer to read from
 * @param off Offset where to write the data from within the binary attribute
 * @param size Amount of data to write
 * @return Returns number of bytes written or negative error id
 */
static ssize_t diagnostics_write(struct file *file, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t size)
{
	int ret;
	struct acm *acm = kobj_to_acm(kobj);
	struct diag_bin_attribute *diagnostics;

	diagnostics = container_of(bin_attr, struct diag_bin_attribute,
		bin_attr);

	ret = bypass_diag_init(acm->bypass[diagnostics->index]);
	if (ret)
		return ret;

	return size;
}

/**
 * @brief Read diagnostic poll time for a bypass module
 *
 * @param dev the device the attribute is associated with
 * @param attr respective device_attribute
 * @param buf Data buffer to write the values to.
 * @return return number of bytes written into the buffer.
 */
static ssize_t diag_poll_time_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct acm *acm = dev_to_acm(dev);
	struct diag_device_attribute *diagnostics;

	diagnostics = container_of(attr, struct diag_device_attribute,
		dev_attr);

	return scnprintf(buf, PAGE_SIZE, "%u\n",
		bypass_get_diag_poll_time(acm->bypass[diagnostics->index]));
}


/**
 * @brief Write diagnostic poll time for a bypass module
 *
 * @param dev the device the attribute is associated with
 * @param attr respective device_attribute
 * @param buf Data buffer to read the values from.
 * @param count Data buffer size.
 * @return return number of bytes read (>=0) or error number (<0).
 */
static ssize_t diag_poll_time_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	ssize_t ret;
	struct acm *acm = dev_to_acm(dev);
	struct diag_device_attribute *diagnostics;

	diagnostics = container_of(attr, struct diag_device_attribute,
		dev_attr);

	if (count == 0)
		return 0;

	ret = kstrtoul(buf, 0, &value);
	if (ret)
		return ret;
	bypass_set_diag_poll_time(value, acm->bypass[diagnostics->index]);
	return count;
}

/**
 * @brief Diagnostic binary attribute diagnostics
 */
ACM_DIAG_BINATTR_RW(diagnostics, sizeof(struct acmdrv_diagnostics));

/**
 * @brief Diagnostic ASCII attribute diag_poll_time
 */
ACM_DIAG_ATTR_RW(diag_poll_time);

/**
 * @brief sysfs binary attributes of diagnostic section
 */
static struct bin_attribute *diag_bin_attrs[] = {
	&diag_binattr_diagnostics_M0.bin_attr,
	&diag_binattr_diagnostics_M1.bin_attr,
	NULL
};


/**
 * @brief sysfs ASCII attributes of diagnostic section
 */
static struct attribute *diag_ascii_attr[] = {
	&diag_devattr_diag_poll_time_M0.dev_attr.attr,
	&diag_devattr_diag_poll_time_M1.dev_attr.attr,
	NULL
};

/**
 * @brief Linux attribute group for diagnostics section
 */
static const struct attribute_group diag_group = {
	.name = __stringify(ACMDRV_SYSFS_DIAG_GROUP),
	.bin_attrs = diag_bin_attrs,
	.attrs = diag_ascii_attr,
};

/**
 * @brief initialize sysfs diagnostics section
 *
 * @param acm ACM instance
 */
const struct attribute_group *sysfs_diag_init(struct acm *acm)
{
	if (acm->if_id < ACM_IF_4_0)
		return NULL;

	return &diag_group;
}

/**
 * exit sysfs diag section
 *
 * @param acm ACM instance
 */
void sysfs_diag_exit(struct acm *acm)
{
	/* nothing to do */
}

/**@} acmsysfsdiagimpl */

