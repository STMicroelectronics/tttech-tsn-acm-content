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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "application.h"
#include "logging.h"
#include "tracing.h"
#include "hwconfig_def.h"
#include "sysfs.h"

int __must_check apply_configuration(struct acm_config *config, uint32_t identifier) {
    int ret, i;
    TRACE2_ENTER();
    /* If there is a running configuration in ACM HW, then disable it.
     * Anyhow clean all tables. */
    ret = write_clear_all_fpga();
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // start configuration
    ret = sysfs_write_config_status_to_HW(ACMDRV_CONFIG_START_STATE);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    /* flash the msg_buffers - it is important to first write the file
     * descriptors and then the message buffer aliases */
    ret = sysfs_write_msg_buff_to_HW(&config->msg_buffs, BUFF_DESC);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    ret = sysfs_write_msg_buff_to_HW(&config->msg_buffs, BUFF_ALIAS);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // flash the modules
    for (i = 0; i < ACM_MODULES_COUNT; i++) {
        if (config->bypass[i] != NULL) {
            ret = write_module_data_to_HW(config->bypass[i]);
            if (ret < 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
        }
    }
    // write base recovery timer table - exists only once in system
    ret = sysfs_write_base_recovery(config);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write configuration ID to a register in ACM HW for future validation.
    ret = sysfs_write_configuration_id(identifier);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // finish configuration
    ret = sysfs_write_config_status_to_HW(ACMDRV_CONFIG_END_STATE);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* write schedules to HW */
    ret = apply_schedule(config);
    if (ret != 0) {
        LOGERR("Config: applying schedule to HW failed");
        TRACE2_MSG("Fail");
        return ret;
    }

    TRACE2_EXIT();
    return 0;
}

int __must_check apply_schedule(struct acm_config *config) {
    int ret, i;

    TRACE2_ENTER();
    for (i = 0; i < ACM_MODULES_COUNT; i++) {
        if (config->bypass[i] != NULL) {
            ret = write_module_schedule_to_HW(config->bypass[i]);
            if (ret < 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
        }
    }

    TRACE2_EXIT();
    return 0;
}

int __must_check remove_configuration(void) {
    int ret;

    TRACE2_ENTER();
    ret = write_clear_all_fpga();
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    TRACE2_EXIT();
    return ret;
}

