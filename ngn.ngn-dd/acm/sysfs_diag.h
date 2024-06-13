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
 * @file sysfs_diag.h
 * @brief ACM Driver SYSFS - Diagnostics Section
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_SYSFS_DIAG_H_
#define ACM_SYSFS_DIAG_H_

struct attribute_group;
struct acm;

const struct attribute_group *sysfs_diag_init(struct acm *acm);
void sysfs_diag_exit(struct acm *acm);

#endif /* ACM_SYSFS_DIAG_H_ */
