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
 * @file config.h
 * @brief ACM Driver Configuration Handling
 *
 * @author Helmut Buchsbaum <helmut.buchsbaum@tttech-industrial.com>
 */

#ifndef ACM_CONFIG_H_

#define ACM_CONFIG_H_

#include "acm-module.h"
#include "state.h"

int acm_config_start(struct acm *acm, enum acmdrv_status *new_status);
int acm_config_end(struct acm *acm, enum acmdrv_status *new_status);
int acm_config_stop_running(struct acm *acm, enum acmdrv_status *new_status);
int acm_config_restart_running(struct acm *acm, enum acmdrv_status *new_status);
int acm_config_remove(struct acm *acm, enum acmdrv_status *new_status);

#endif /* ACM_CONFIG_H_ */
