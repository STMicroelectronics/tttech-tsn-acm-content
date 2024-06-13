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
#ifndef STATUS_H_
#define STATUS_H_

#include <linux/acm/acmdrv.h>

#include "libacmconfig_def.h"
#include "hwconfig_def.h"

/**
 * @brief directory substructure of system fs
 *
 * All the files of system fs are structed in groups. The groups are define in the driver. The
 * structure is used for reading the status items in an efficient way.
 */
struct sysfs_status_subpath {
    char group[10]; /**< directory in sysfs according group level for status item */
    char subpath[40]; /**< filename in sysfs for status item */
};

/**
 * @brief constant which holds all the group and filenames for values packed to status
 */
static const struct sysfs_status_subpath status_subpath[] = {
        [STATUS_HALT_ERROR_OCCURED] = {
                .group = __stringify(ACMDRV_SYSFS_ERROR_GROUP),
                .subpath = "halt_on_error"
        },
        [STATUS_IP_ERROR_FLAGS] = {
                .group = __stringify(ACMDRV_SYSFS_ERROR_GROUP),
                .subpath = "error_flags"
        },
        [STATUS_POLICING_ERROR_FLAGS] = {
                .group = __stringify(ACMDRV_SYSFS_ERROR_GROUP),
                .subpath = "policing_flags"
        },
        [STATUS_RUNT_FRAMES_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "runt_frames"
        },
        [STATUS_MII_ERRORS_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "mii_errors"
        },
        [STATUS_GMII_ERROR_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "gmii_error_prev_cycle"
        },
        [STATUS_SOF_ERRORS_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "sof_errors"
        },
        [STATUS_LAYER7_MISSMATCH_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "layer7_missmatch_cnt"
        },
        [STATUS_DROPPED_FRAMES_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "drop_frames_cnt_prev"
        },
        [STATUS_FRAMES_SCATTERED_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "scatter_DMA_frames_cnt_prev"
        },
        [STATUS_DISABLE_OVERRUN_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "disable_overrun_prev"
        },
        [STATUS_TX_FRAMES_CYCLE_CHANGE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "tx_frame_cycle_change"
        },
        [STATUS_TX_FRAMES_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "tx_frames_prev"
        },
        [STATUS_RX_FRAMES_CYCLE_CHANGE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "rx_frames_cycle_change"
        },
        [STATUS_RX_FRAMES_PREV_CYCLE] = {
                .group = __stringify(ACMDRV_SYSFS_STATUS_GROUP),
                .subpath = "rx_frames_prev"
        }, };

/**
 * @brief Diagnostic data of the ACM
 */
/**
 * @brief filename for accessing time frequency
 */
#define ACM_SYSFS_TIME_FREQ_FILE    time_freq
/**
 * @brief filename for accessing capability value configuration read back enabled/disabled
 */
#define ACM_SYSFS_READ_BACK         cfg_read_back
/**
 * @brief filename for accessing capability value debug enabled/disabled
 */
#define ACM_SYSFS_DEBUG             debug_enable
/**
 * @brief filename for accessing capability value for overall message buffer size in bytes
 */
#define ACM_SYSFS_MSGBUF_SIZE       msgbuf_memsize
/**
 * @brief filename for accessing capability value for number of available message buffers
 */
#define ACM_SYSFS_MSGBUF_COUNT      msgbuf_count
/**
 * @brief filename for accessing capability value for message buffer block size
 */
#define ACM_SYSFS_MSGBUF_DATAWIDTH  msgbuf_datawidth
/**
 * @brief filename for accessing capability value rx redundancy enabled/disabled
 */
#define ACM_SYSFS_RX_REDUNDANCY     rx_redundancy
/**
 * @brief filename for accessing capability value individual recovery enabled/disabled
 */
#define ACM_SYSFS_INDIV_RECOV       individual_recovery
/**
 * @brief filename for accessing ACM device id
 */
#define ACM_SYSFS_DEVICE            device_id
/**
 * @brief filename for accessing ACM version id
 */
#define ACM_SYSFS_VERSION           version_id
/**
 * @brief filename for accessing ACM revision id
 */
#define ACM_SYSFS_REVISION          revision_id

/**
 * @brief Diagnostic data of the ACM
 */
/**
 * @brief filename for accessing diagnostic data
 */
#define ACM_SYSFS_DIAGNOSTICS_FILE diagnostics
/**
 * @brief filename for accessing diagnostic poll time
 */
#define ACM_SYSFS_DIAG_POLL_TIME diag_poll_time


/**
 * @ingroup acmstatusarea
 * @brief Read actual configuration identifier
 *
 * The function reads the configuration identifier from the hardware, of the configuration
 * currently applied to HW.
 *
 * @return value of configuration identifier. Negative values represent an error.
 */
int64_t __must_check status_read_config_identifier();

/**
 * @ingroup acmstatusarea
 * @brief Read buffer index for buffer name
 *
 * The function iterates through all configured message buffers and compares if one has the same
 * name as in parameter buffer. In this case the index for this name is returned.
 * If the name is not found, the function returns an error.
 *
 * @param buffer pointer to buffer name
 *
 * @return value of buffer_alias index for name in parameter buffer. Negative values represent an
 * error.
 */
int __must_check status_get_buffer_id_from_name(const char* buffer);

/**
 * @ingroup acmstatusarea
 * @brief Read buffer locking vector directly from HW
 *
 * The function reads the locking vector of message buffers directly from HW. If there is any
 * problem reading this value, the function returns an error.
 *
 * @return lock vector. Negative values represent an error.
 */
int64_t __must_check status_read_buffer_locking_vector();

/**
 * @ingroup acmstatusarea
 * @brief Read a specific status item of the defined module
 *
 * The function creates the path for access to acm filesystem and reads the
 * status data from there.
 *
 * @param module_id identifier of the module to read the status from.
 * @param id status value to be read
 *
 * @return value of status item. Negative values represent an error.
 */
int64_t __must_check status_read_item(enum acm_module_id module_id, enum acm_status_item id);

/**
 * @ingroup acmdiagarea
 * @brief Set diagnostic poll cycle
 *
 * The function creates the path to access acm filesystem and sets the
 * diagnostic data there.
 *
 * @param module_id identifier of the module for which the diagnostic poll time is set.
 * @param interval_ms polling time in milli seconds.
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check status_set_diagnostics_poll_time(enum acm_module_id module_id,
        uint16_t interval_ms);

/**
 * @ingroup acmdiagarea
 * @brief Read diagnostic data of a specific module
 *
 * The function creates the path for access to acm filesystem and reads the
 * diagnostic data from there. The function converts the read data from HW
 * format to external format.
 *
 * @param module_id identifier of the module to read the diagnostic data from.
 *
 * @return diagnostic counter values. The values are provided as acm_diagnostics struct
 * containing the complete diagnostic dataset of the module. NULL will be returned in
 * case of an error.
 */
struct acm_diagnostic* __must_check status_read_diagnostics(enum acm_module_id module_id);

/**
 * @ingroup acmstatusarea
 * @brief Read a specific capability item of the system
 *
 * The function creates the path for access to acm filesystem and reads the
 * capability data from there.
 *
 * @param id capability value to be read
 *
 * @return value of capability item. Negative values represent an error.
 */
int32_t __must_check status_read_capability_item(enum acm_capability_item id);

/**
 * @ingroup acmstatusarea
 * @brief Read status item time_freq
 *
 * The function creates the path for access to acm filesystem and reads the
 * value of time frequency.
 *
 * @return frequency at which ACM HW is running. Negative values represent an error.
 */
int64_t __must_check status_read_time_freq(void);

/**
 * @ingroup acmstatusarea
 * @brief Read status item of type int32
 *
 * The function creates the path for access to acm filesystem and reads the
 * value contained in the specified file.
 *
 * @param filename pointer to the filename, from which the value is read.
 *
 * @return int32 value read from specified file. Negative values represent an error.
 */
int32_t __must_check get_int32_status_value(const char* filename);

/**
 * @ingroup acmdiagarea
 * @brief convert diagnostic data from packed to unpacked
 *
 * The function converts the packed diagnostic data read from hardware to
 * an unpacked structure for the external interface.
 *
 * @param source diagnostic data in packed format.
 * @param target diagnostic data in unpacked format for external interface.
 */
void convert_diag2unpacked(struct acmdrv_diagnostics *source, struct acm_diagnostic *target);

/**
 * @ingroup acmstatusarea
 * @brief calculate tick duration from time frequency
 *
 * The function reads the time frequency from hardware and calculates
 * the tick duration.
 *
 * @return tick duration in nanoseconds. Negative values and zero indicate an
 * error.
 */
int32_t __must_check calc_tick_duration(void);

/**
 * @ingroup acmstatusarea
 * @brief read ip version
 *
 * The function reads the values for device id, version id and revision id, packs them into a
 * string and returns it to the caller.
 *
 * @return Pointer to a string which contains the version of ip system. A NULL pointer indicates an
 * error.
 */
char* __must_check status_get_ip_version(void);

#endif /* STATUS_H_ */
