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
 * @file sysfs.h
 * @brief ACM Driver SYSFS Interface
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

/**
 * @addtogroup acmsysfsimpl
 * @{
 */

#ifndef ACM_SYSFS_H_
#define ACM_SYSFS_H_

#include <linux/kernel.h>
#include <acm-module.h>

struct acm;

/**
 * @brief helper to iterate through all item of an array on sysfs interface
 */
#define foreach_item(_item, _offset, _size, _elsize)		\
	for (_item = ((off_t)(_offset)) / (_elsize);		\
	     _item < ((off_t)(_offset) + (_size)) / (_elsize);	\
	     ++_item)


/* as long as the kernel does not provide a write-only binary attribute .. */
#ifndef BIN_ATTR_WO

/**
 * @def __BIN_ATTR_WO
 * @brief Additional macro initializer for write only binary attributes
 *
 * @param _name name of the attribute
 * @param _size size of the attribute
 */
#define __BIN_ATTR_WO(_name, _size) {				\
	.attr	= { .name = __stringify(_name), .mode = 0664 },	\
	.write	= _name##_write,				\
	.size	= _size,					\
}


/**
 * @def BIN_ATTR_WO
 * @brief Additional instantiation macro for write only binary attributes
 *
 * @param _name name of the attribute
 * @param _size size of the attribute
 */
#define BIN_ATTR_WO(_name, _size)	\
struct bin_attribute bin_attr_##_name = __BIN_ATTR_WO(_name, _size)

#endif /* BIN_ATTR_WO */

/**
 * @brief add visibility control for device attributes
 */
struct visible_device_attribute {
	enum acm_ip_if_variant min; /**< minimum interface variant */
	enum acm_ip_if_variant max; /**< maximum interface variant */
	bool debug_only; /**< only for debug bitstreams */
	struct device_attribute dattr; /**< Linux device attribute */
};

/**
 * @brief initialize macro for struct visible_device_attribute
 */
#define __VATTR(_name, _mode, _show, _store, _min_if, _max_if, _dbg) {	\
	.min = _min_if,							\
	.max = _max_if,							\
	.debug_only = _dbg,						\
	.dattr = __ATTR(_name, _mode, _show, _store),			\
}

/**
 * @brief initialize macro for read-only struct visible_device_attribute
 */
#define __VATTR_RO(_name, _min_if, _max_if, _dbg) {	\
	.min = _min_if,					\
	.max = _max_if,					\
	.debug_only = _dbg,				\
	.dattr = __ATTR_RO(_name),			\
}

/**
 * @def DEVICE_VATTR_RO
 * @brief Macro for instantiation of a read-only visibility controlled
 *        device attribute
 *
 * This macro instantiates the constants and structures required to read
 * a partition register, which is cleared on read.
 *
 * @param _name attribute name
 * @param _min_if minimum interface version to be visible
 * @param _max_if maximum interface version to be visible
 * @param _dbg: visible only for extended status enable ACM IPs
 */
#define DEVICE_VATTR_RO(_name, _min_if, _max_if, _dbg)		\
	struct visible_device_attribute vdev_attr_##_name =	\
		__VATTR_RO(_name, _min_if, _max_if, _dbg)

int __must_check acm_sysfs_init(struct acm *acm);
void acm_sysfs_exit(struct acm *acm);

int sysfs_bin_attr_check(struct bin_attribute *bin_attr, loff_t off,
			 size_t totalsize, size_t elsize);

#endif /* ACM_SYSFS_H_ */
/**@} acmsysfsimpl */

