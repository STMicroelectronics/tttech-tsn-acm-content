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
 * @file sysfs_status.h
 * @brief ACM Driver SYSFS - Status Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_SYSFS_STATUS_H_
#define ACM_SYSFS_STATUS_H_

#include <linux/sysfs.h>
struct acm;

const struct attribute_group *sysfs_status_init(struct acm *acm);
void sysfs_status_exit(struct acm *acm);

#endif /* ACM_SYSFS_STATUS_H_ */
