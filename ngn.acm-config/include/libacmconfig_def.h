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
 * @file libacmconfig_def.h
 *
 * ACM defines of configuration library
 *
 */

#ifndef LIBACMCONFIG_DEF_H_
#define LIBACMCONFIG_DEF_H_

#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include <net/ethernet.h> /* for ETHER_ADDR_LEN */

#include <errno.h>

/**
 * @brief ACM local error - frame size of egress frames too short
 */
#define EACMEGRESSFRAMESIZE	140
/**
 * @brief ACM local error - config. contains stream without operation
 */
#define EACMOPMISSING 141
/**
 * @brief ACM local error - sum of constant message buffer sizes > 4096
 * for a module
 */
#define EACMCONSTBUFFER 142
/**
 * @brief ACM local error - number of redundand streams of a module > 31
 */
#define EACMREDUNDANDSTREAMS 143
/**
 * @brief ACM local error - number of schedule events of a module > 1024
 */
#define EACMSCHEDULEEVENTS	144
/**
 * @brief ACM local error - number of lookup entries of a module > 16
 */
#define EACMLOOKUPENTRIES 145
/**
 * @brief ACM local error - number of ingress operations of a module > 256
 */
#define EACMINGRESSOPERATIONS 146
/**
 * @brief ACM local error - number of egress operations of a module > 256
 */
#define EACMEGRESSOPERATIONS 147
/**
 * @brief ACM local error - stream and module period aren't compatible
 */
#define EACMINCOMPATIBLEPERIOD 148
/**
 * @brief ACM local error - no data could be read from sysfs file
 */
#define EACMSYSFSNODATA 149
/**
 * @brief ACM local error - invalid value of module cycle
 */
#define EACMMODCYCLE 150
/**
 * @brief ACM local error - problem with time of a fsc scheduling item
 */
#define EACMSCHEDTIME 151
/**
 * @brief ACM local error - problem at creation of schedule item
 */
#define EACMADDSCHED 152
/**
 * @brief ACM local error - internal software problem
 */
#define EACMINTERNAL 153
/**
 * @brief ACM local error - two referenced streams are added to different configurations
 */
#define EACMDIFFCONFIG	154
/**
 * @brief ACM local error - stream not added to config at final validation
 */
#define EACMSTREAMCONFIG	155
/**
 * @brief ACM local error - number of message buffers of config > 32
 */
#define EACMNUMMESSBUFF	156
/**
 * @brief ACM local error - there is no free scheduler table on HW to apply schedule
 */
#define EACMNOFREESCHEDTAB 157
/**
 * @brief ACM local error - forward operation offset > ingress position plus
 * max truncatable bytes
 */
#define EACMFWDOFFSET	158
/**
 * @brief ACM local error - frame payloadsize higher than maximum
 */
#define EACMPAYLOAD	159
/**
 * @brief ACM local error - name not found in aliases table
 */
#define EACMBUFFNAME	160
/**
 * @brief ACM local error - configuration not found
 */
#define EACMCONFIG      161
/**
 * @brief ACM local error - configuration value exceeds resources - too long
 */
#define EACMCONFIGVAL  162
/**
 * @brief ACM local error - two referenced streams are added to same module
 */
#define EACMREDSAMEMOD  163
/**
 * @brief ACM local error - too many insert operations in stream
 */
#define EACMNUMINSERT   164

/**
 * @brief MUST_CHECK
 *
 * Macro to enforce checking the return values of each a function.
 * If return value is not checked, a error will be shown during compilation
 */
#define __must_check __attribute__((warn_unused_result))

/**
 * @brief maximum buffer name size
 */
#define ACM_MAX_NAME_SIZE	55U

/**
 * @brief Number of ACM Bypass Modules
 */
#define ACM_MODULES_COUNT	2U

/**
 * @brief maximum filter data size in bytes
 */
#define ACM_MAX_FILTER_SIZE	112U

/**
 * @brief maximum lookup size at ingress
 */
#define ACM_MAX_LOOKUP_SIZE	16U

/**
 * @brief maximum frame size with VLAN-Tag and R-Tag
 */
#define ACM_MAX_FRAME_SIZE	1528U

/**
 * @brief minimal frame size
 */
#define ACM_MIN_FRAME_SIZE	64U

/**
 * @brief maximum payload size within frame
 */
#define MAX_PAYLOAD_HEADER_SIZE 1518U

/**
 * @brief maximum insert operations per stream
 */
#define ACM_MAX_INSERT_OPERATIONS	8U

/**
 * @brief maximum egress operations per module (gather operations)
 */
#define ACM_MAX_EGRESS_OPERATIONS	256U

/**
 * @brief maximum ingress operations per module(scatter operations)
 */
#define ACM_MAX_INGRESS_OPERATIONS	256U

/**
 * @brief maximum number of schedule events per module
 */
#define ACM_MAX_SCHEDULE_EVENTS	1024U

/**
 * @brief maximum length of constant buffers per module
 */
#define ACM_MAX_CONST_BUFFER_SIZE 4096U

/**
 * @brief maximum number of redundant streams per module
 */
#define ACM_MAX_REDUNDANT_STREAMS 32U

/**
 * @brief maximum number of lookup items per module
 */
#define ACM_MAX_LOOKUP_ITEMS 16U

/**
 * @brief minimal scheduling gap in ticks
 * 1 tick = 10ns at 100Mhz operation
 * 1 tick = 8ns at 125Mhz operation
 */
#define ACM_MIN_SCHEDULING_GAP	8U

/**
 * @brief maximum number of bytes which can be truncated with a forward operation
 */
#define MAX_TRUNC_BYTES 19U

/**
 * @brief minimum value for VLAN id of a stream
 */
#define ACM_VLAN_ID_MIN	3U

/**
 * @brief maximum value for VLAN id of a stream
 *
 * The maximum value for vlan id is 4094, but 4095 is used as value with
 * special meaning, therefore the value range ends at 4095
 */
#define ACM_VLAN_ID_MAX	4095U

/**
 * @brief minimum value for VLAN priority of a stream
 */
#define ACM_VLAN_PRIO_MIN	0U

/**
 * @brief maximum value for VLAN priority of a stream
 */
#define ACM_VLAN_PRIO_MAX	7U


/**
 * @ingroup acmmodule
 * @brief ACM linkspeed
 *
 * enum acm_linkspeed is used to define the the linkspeed of a module
 *
 */
enum acm_linkspeed {
    SPEED_100MBps = 0, /**< speed for 100Mbps operation */
    SPEED_1GBps = 1, /**< speed for 1Gbps operation */
};

/**
 * @ingroup acmmodule
 * @brief ACM connection mode
 *
 * enum acm_connection mode is used to define the operating mode of the
 * ACM module. It is possible to define if the module is in serial or parallel mode.
 * Serial mode is required to forward frames between ports having a different linkspeed.
 * Parallel mode is the default operating mode allowing fast frame forwarding.
 */
enum acm_connection_mode {
    CONN_MODE_SERIAL = 0, /**<option to set the module to serial-mode operation */
    CONN_MODE_PARALLEL = 1, /**<option to set the module to parallel-mode operation */
};

/**
 * @ingroup acmmodule
 * @brief Module ID
 *
 * The module ID defines the communication path of the streams.
 */
enum acm_module_id {
    MODULE_0 = 0, /**<receiving frames from sw0p3, transmitting frames on sw0p2 */
    MODULE_1 = 1, /**<receiving frames from sw0p2, transmitting frames on sw0p3 */
};

/**
 * @ingroup acmstatusarea
 * @brief Status items
 *
 * The status items allow to define which status value shall be read from
 * the module.
 */
enum acm_status_item {
    STATUS_HALT_ERROR_OCCURED, /**<Flag if an HALT error occurred on the module */
    STATUS_IP_ERROR_FLAGS, /**< Bit-mask of all IP error flags */
    STATUS_POLICING_ERROR_FLAGS, /**< Bit-mask of all policing error flags */
    STATUS_RUNT_FRAMES_PREV_CYCLE, /**< Counter representing the number of RUNT frame errors */
    STATUS_MII_ERRORS_PREV_CYCLE, /**< Counter representing the number of MII frame errors */
    STATUS_GMII_ERROR_PREV_CYCLE, /**< Counter representing the number of GMII frame errors*/
    STATUS_SOF_ERRORS_PREV_CYCLE, /**< Counter representing the number of SOF frame errors */
    STATUS_LAYER7_MISSMATCH_PREV_CYCLE, /**< Counter representing the number of Layer7 mismatches (additional filter of ingress-frame) in the last cycle */
    STATUS_DROPPED_FRAMES_PREV_CYCLE, /**< Counter representing the number of dropped frames in the last cycle */
    STATUS_FRAMES_SCATTERED_PREV_CYCLE, /**< Counter representing the number of scattered frames in the last cycle */
    STATUS_DISABLE_OVERRUN_PREV_CYCLE, /**< Counter representing the number of overrun events in the last cycle */
    STATUS_TX_FRAMES_CYCLE_CHANGE, /**< Counter representing the number of transmitted frames difference between two cycles */
    STATUS_TX_FRAMES_PREV_CYCLE, /**< Counter representing the number of transmitted frames in the last cycle */
    STATUS_RX_FRAMES_CYCLE_CHANGE, /**< Counter representing the number of received frames difference between two cycles */
    STATUS_RX_FRAMES_PREV_CYCLE, /**< Counter representing the number of received frames in the last cycle */
    STATUS_ITEM_NUM, /**< not a status item, just for checking range */
};

/**
 * @ingroup acmdiagarea
 * @brief ACM diagnostic structure
 *
 * struct acm_diagnostic contains all diagnostic information of one module.
 */
struct acm_diagnostic {
    struct timespec timestamp; /**< Timestamp when the diagnostic data had been updated last time */
    uint32_t scheduleCycleCounter; /**< Number of cycles contained in the diagnostic dataset */

    uint32_t txFramesCounter; /**< Number of transmitted frames */
    uint32_t rxFramesCounter; /**< Number of received frames */

    uint32_t ingressWindowClosedFlags; /**< Flags representing the stream-id
     where a frame had been received outside of the defined ingress window(s)*/
    uint32_t ingressWindowClosedCounter[ACM_MAX_LOOKUP_SIZE]; /**< One counter for each stream-id.
     The counter will increase with each frame received outside of a ingress window*/

    uint32_t noFrameReceivedFlags; /**< Flags representing the stream-id
     where a window had been closed without receiving a frame*/
    uint32_t noFrameReceivedCounter[ACM_MAX_LOOKUP_SIZE]; /**< One counter for each stream-id.
     The counter will increase if the policing window had been closed without a single frame received. */

    uint32_t recoveryFlags; /**< Flags representing the stream-id
     where a recovery frame had been generated*/
    uint32_t recoveryCounter[ACM_MAX_LOOKUP_SIZE]; /**< One counter for each stream-id.
     The counter will increase with each recovery frame beeing generated.*/

    uint32_t additionalFilterMismatchFlags; /**< Flags representing the stream-id
     where the header was correct identified, but the additional filter information raised a mismatch.*/
    uint32_t additionalFilterMismatchCounter[ACM_MAX_LOOKUP_SIZE]; /**< One counter for each stream-id.
     The counter will increase with each frame producing the mismatch of the additional filter information. */
};

/**
 * @ingroup acmcapability
 * @brief ACM capability items
 *
 * The item allow to read different capability informations of the device
 */
enum acm_capability_item {
    CAP_MIN_SCHEDULE_TICK, /**< Minimal scheduling tick in ns */
    CAP_MAX_MESSAGE_BUFFER_SIZE, /**< overall message buffer size in bytes */
    CAP_CONFIG_READBACK, /**< Configuration readback enabled(1) or disabled(0) */
    CAP_DEBUG, /**< Debug-mode enabled(1) or disabled(0) */
    CAP_MAX_ANZ_MESSAGE_BUFFER, /**<number of available message buffers */
    CAP_MESSAGE_BUFFER_BLOCK_SIZE, /**< message buffer is allocated in number of
     blocks - not bytes */
    CAP_REDUNDANCY_RX, /**< number 0 - not enabled, number 1 - enabled*/
    CAP_INDIV_RECOVERY /**< number 0 - not enabled, number 1 - enabled*/
};

/**
 * @brief enabled value for CAP_REDUNDANCY_RX
 */
#define RX_REDUNDANCY_SET 1

#ifdef __cplusplus
} /* extern "C" */
#endif	/* __cplusplus */

#endif /* LIBACMCONFIG_DEF_H_ */
