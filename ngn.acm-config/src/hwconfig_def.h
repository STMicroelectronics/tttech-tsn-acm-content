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

/**
 * @file hwconfig_def.h
 *
 * HW configurations for ACM library
 *
 */

#ifndef HWCONFIG_DEF_H_
#define HWCONFIG_DEF_H_

#include <stdint.h>
#include <stdbool.h>

#include "libacmconfig_def.h"

#ifndef ACMDEV_BASE
/**
 * @brief default for base directories
 *
 * These might be overridden by setting them as compiler flags for testing purposes on
 * non-target systems.
 */
/**
 * @brief default for base SYSFS directory for ACM driver access
 */
#define ACMDEV_BASE "/sys/devices/acm/"
/**
 * @brief default SYSFS directory for writing delays to hardware
 */
#define DELAY_BASE "/sys/class/net/"
/**
 * @brief default SYSFS directory for message buffer files on hardware
 */
#define MSGBUF_BASE "/dev"
/**
 * @brief default SYSFS file for configuration file
 */
#define CONFIG_FILE "/etc/default/config_acm"
#endif

/**
 * @brief max stringlength reserved for path of sysfs device
 */
#define SYSFS_PATH_LENGTH 90U

/**
 * @brief HW delays for scheduling
 *
 * schedule_delays for different ACM speeds to be considered for ingress and
 * egress streams for PHY and chip
 *
 */
struct hw_dependent_delay {
    uint32_t chip_in; /**< ingress delay of chip */
    uint32_t chip_eg; /**< egress delay of chip */
    uint32_t phy_in; /**< ingress delay of physical layer*/
    uint32_t phy_eg; /**< egress delay of physical layer*/
    uint32_t ser_bypass; /**< bypass delay of serial connection mode */
    uint32_t ser_switch; /**< switch delay of serial connection mode */
};

/**
 * @brief default values for HW delays for scheduling
*/
static const struct hw_dependent_delay schedule_delays[] = {
    [SPEED_100MBps] = { .chip_in = 50, .chip_eg = 120, .phy_in = 404, .phy_eg = 444, .ser_bypass = 2844, .ser_switch = 3900},
    [SPEED_1GBps] = { .chip_in = 50, .chip_eg = 120, .phy_in = 298, .phy_eg = 199, .ser_bypass = 439, .ser_switch = 940}, };

/**
 * @brief size of timestamp in message buffer
 *
 * for read operations the message buffer has to be extended by the size of the
 * timestamp which is added automatically
*/
#define SIZE_TIMESTAMP 4U

/**
 * @brief minimum number of ticks between two HW scheduling commands
*/
#define ANZ_MIN_TICKS 8U

/**
 * @brief number of characters the praefix for message buffer names can have
*/
#define PRAEFIX_LENGTH 10U

/**
 * @brief default praefix for message buffer names
 *
 * used, when no praefix is found in configuration file
*/
#define DEFAULT_PRAEFIX "acm_"

/**
 * @brief key word for configuration of message buffer praefix in configuration file CONFIG_FILE
*/
#define KEY_PRAEFIX "PRAEFIX_MSG_BUFFER_FILENAME"

/**
 * @brief key word for configuration of ingress delay of chip for speed 100MBps in file CONFIG_FILE
*/
#define KEY_CHIP_IN_100MBps 	"HW_DELAY_100MBps_CHIP_IN_NS"
/**
 * @brief key word for configuration of egress delay of chip for speed 100MBps in file CONFIG_FILE
*/
#define KEY_CHIP_EG_100MBps 	"HW_DELAY_100MBps_CHIP_EG_NS"
/**
 * @brief key word for configuration of ingress delay of physical layer for speed 100MBps in file CONFIG_FILE
*/
#define KEY_PHY_IN_100MBps 		"HW_DELAY_100MBps_PHY_IN_NS"
/**
 * @brief key word for configuration of egress delay of physical layer for speed 100MBps in file CONFIG_FILE
*/
#define KEY_PHY_EG_100MBps 		"HW_DELAY_100MBps_PHY_EG_NS"
/**
 * @brief key word for configuration of bypass delay for speed 100MBps in file CONFIG_FILE
*/
#define KEY_SER_BYPASS_100MBps 	"HW_DELAY_100MBps_SER_BYPASS_NS"
/**
 * @brief key word for configuration of switch delay for speed 100MBps in file CONFIG_FILE
*/
#define KEY_SER_SWITCH_100MBps 	"HW_DELAY_100MBps_SER_SWITCH_NS"
/**
 * @brief key word for configuration of ingress delay of chip for speed 1GBps in file CONFIG_FILE
*/
#define KEY_CHIP_IN_1GBps 		"HW_DELAY_1GBps_CHIP_IN_NS"
/**
 * @brief key word for configuration of egress delay of chip for speed 1GBps in file CONFIG_FILE
*/
#define KEY_CHIP_EG_1GBps 		"HW_DELAY_1GBps_CHIP_EG_NS"
/**
 * @brief key word for configuration of ingress delay of physical layer for speed 1GBps in file CONFIG_FILE
*/
#define KEY_PHY_IN_1GBps 		"HW_DELAY_1GBps_PHY_IN_NS"
/**
 * @brief key word for configuration of egress delay of physical layer for speed 1GBps in file CONFIG_FILE
*/
#define KEY_PHY_EG_1GBps 		"HW_DELAY_1GBps_PHY_EG_NS"
/**
 * @brief key word for configuration of bypass delay for speed 1GBps in file CONFIG_FILE
*/
#define KEY_SER_BYPASS_1GBps 	"HW_DELAY_1GBps_SER_BYPASS_NS"
/**
 * @brief key word for configuration of switch delay for speed 1GBps in file CONFIG_FILE
*/
#define KEY_SER_SWITCH_1GBps 	"HW_DELAY_1GBps_SER_SWITCH_NS"

/**
 * @brief key word for configuration of recovery timeout for redundant TX streams in file CONFIG_FILE
*/
#define KEY_RECOVERY_TIMEOUT_MS "RECOVERY_TIMEOUT_MS"
/**
 * @brief key word for configuration of log level in file CONFIG_FILE
*/
#define KEY_LOGLEVEL            "LOGLEVEL"
/**
 * @brief key word for configuration of trace level in file CONFIG_FILE
*/
#define KEY_TRACELEVEL          "TRACELEVEL"

/**
 * @brief default value for recovery timeout for redundant TX streams
 *
 * used if not configured in file CONFIG_FILE
*/
#define DEFAULT_REC_TIMEOUT_MS	1000

/**
 * @brief maximum value for type uint32_t
*/
#define UINT32_T_MAX 0xFFFFFFFF
/**
 * @brief maximum value for type uint16_t
*/
#define UINT16_T_MAX 0xFFFF

/**
 * @brief number of ticks used for artificial nop schedule commands, if distance to next is too far.
*/
#define NOP_DELTA_CYCLE 60000

/**
 * @brief interface name of port of module 0
*/
#define PORT_MODULE_0 "sw0p2"
/**
 * @brief interface name of port of module 1
*/
#define PORT_MODULE_1 "sw0p3"

/**
 * @brief stringification
 * @{
 */
#define QUOTE(str) #str
#define __stringify(str) QUOTE(str)
/** @} */

#endif /* HWCONFIG_DEF_H_ */
