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
 * @file reset.h
 * @brief ACM Driver Reset Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */


#ifndef ACM_RESET_H_
#define ACM_RESET_H_

#include "acm-module.h"
struct reset;

/**
 * @addtogroup hwaccreset
 * @{
 */

int __must_check reset_init(struct acm *acm);
void reset_exit(struct acm *acm);
int reset_trigger(struct reset *reset);

/**@} hwaccreset*/

#endif /* ACM_RESET_H_ */
