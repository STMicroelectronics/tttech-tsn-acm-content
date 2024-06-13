/*
 * TTTech ACM Configuration Library (libacmconfig)
 * Copyright(c) 2019 TTTech Industrial Automation AG.
 *
 * ALL RIGHTS RESERVED.
 * Usage of this software, including source code, netlists, documentation,
 * is subject to restrictions and conditions of the applicable license
 * agreement with TTTech Industrial Automation AG or its affiliates.
 *
 * All trademarks used are the property of their respective owners.
 *
 * TTTech Industrial Automation AG and its affiliates do not assume any liability
 * arising out of the application or use of any product described or shown
 * herein. TTTech Industrial Automation AG and its affiliates reserve the right to
 * make changes, at any time, in order to improve reliability, function or
 * design.
 *
 * Contact: https://tttech.com * support@tttech.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "libacmconfig_def.h"
#include "config.h"

/**
 * @ingroup acmconfig
 * @brief Apply a complete configuration.
 *
 * The old configuration of the device will be replaced by the new configuration
 *
 * @param config ACM configuration to be applied to the device
 * @param identifier configuration id for verification in case of schedule change
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check apply_configuration(struct acm_config *config, uint32_t identifier);
/**
 * @ingroup acmconfig
 * @brief Apply a new schedule to a configuration.
 *
 * The function writes the schedules of all modules of the configuration to hardware
 *
 * @param config ACM configuration to be used to write the schedules
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check apply_schedule(struct acm_config *config);
/**
 * @ingroup acmconfig
 * @brief Delete actually applied configuration from hardware.
 *
 * The function removes the complete configuration from hardware and then writes the delays for
 * parallel mode for both modules. (Because acm hardware falls to parallel mode in case of
 * configuration deletion.)
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check remove_configuration(void);

#endif /* APPLICATION_H_ */
