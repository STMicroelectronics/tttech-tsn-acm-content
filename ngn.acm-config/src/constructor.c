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

#include "logging.h"
#include "tracing.h"
#include "hwconfig_def.h"
#include "sysfs.h"
#include "constructor.h"

CTOR void con(void) {
    char config_str_value[12];
    int ret;
    int level = -1;

    /* read/set log level */
    ret = sysfs_get_configfile_item(KEY_LOGLEVEL, config_str_value, sizeof (config_str_value));
    if (ret == 0) {
        level = strtoul(config_str_value, NULL, 10);
        ret = set_loglevel(level);
    }
    if (ret == 0) {
        printf("loglevel set from configuration file to %d \n", level);
    } else {
        printf("loglevel not set with value from configuration file\n");
    }
    /* read/set trace level */
    ret = sysfs_get_configfile_item(KEY_TRACELEVEL, config_str_value, sizeof (config_str_value));
    if (ret == 0) {
        level = strtoul(config_str_value, NULL, 10);
        set_tracelayer(level);
        printf("tracelayer set from configuration file to %d \n", level);
    } else {
        printf("tracelayer not set with value from configuration file\n");
    }

}

