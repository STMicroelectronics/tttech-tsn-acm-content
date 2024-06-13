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
 * @file sysfs_config.h
 * @brief ACM Driver SYSFS - Config Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_SYSFS_CONFIG_H_
#define ACM_SYSFS_CONFIG_H_

#include <linux/sysfs.h>

/**
 * @brief convert struct timespec64 to proprietary
 *        struct acmdrv_timespec64
 */
static inline
struct acmdrv_timespec64 timespec64_to_start_time(struct timespec64 ts)
{
	return (struct acmdrv_timespec64){
		.tv_sec = ts.tv_sec,
		.tv_nsec = ts.tv_nsec
	};
}

/**
 * @brief convert proprietary struct acmdrv_timespec64 to struct
 *        timespec64
 */
static inline
struct timespec64 start_time_to_timespec64(struct acmdrv_timespec64 st)
{
	return (struct timespec64){
		.tv_sec = st.tv_sec,
		.tv_nsec = st.tv_nsec
	};
}

struct acm;

const struct attribute_group *sysfs_config_init(struct acm *acm);
void sysfs_config_exit(struct acm *acm);

#endif /* ACM_SYSFS_CONFIG_H_ */
