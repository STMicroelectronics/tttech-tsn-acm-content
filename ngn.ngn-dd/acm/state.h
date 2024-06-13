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
 * @file state.h
 * @brief ACM Driver State Control
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_STATE_H_
#define ACM_STATE_H_

#include "acm-module.h"

int __must_check acm_state_init(struct acm *acm);
void acm_state_exit(struct acm *acm);

struct acm_state;

enum acmdrv_status acm_state_get(struct acm_state *state);
int acm_state_set(struct acm_state *state, enum acmdrv_status status);

bool acm_state_is_running(struct acm_state *state);

#endif /* ACM_STATE_H_ */
