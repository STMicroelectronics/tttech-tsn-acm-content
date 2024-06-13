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
 * @file chardev.h
 * @brief ACM Driver Message Buffer Character Device
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_CHARDEV_H_
#define ACM_CHARDEV_H_

#include <linux/types.h>

struct acm_dev;
struct acm;

/**
 * @ingroup communication
 * @brief helper macro to access ACM device per index
 */
#define ACM_DEVICE(acm, i) acm_dev_get_device((acm)->devices, i)

struct acm_dev *acm_dev_create(struct acm *acm, size_t nr);
void acm_dev_destroy(struct acm *acm);

struct acm_dev *acm_dev_get_device(struct acm_dev *first, int i);
int __must_check acm_dev_activate(struct acm_dev *adev, const char *alias,
				  u64 id);
void acm_dev_deactivate(struct acm_dev *adev);
bool acm_dev_get_timestamping(const struct acm_dev *adev);
void acm_dev_set_timestamping(struct acm_dev *adev, bool enable);
const char *acm_dev_get_name(const struct acm_dev *adev);
bool acm_dev_is_active(const struct acm_dev *adev);
u64 acm_dev_get_id(const struct acm_dev *adev);

#endif /* ACM_CHARDEV_H_ */
