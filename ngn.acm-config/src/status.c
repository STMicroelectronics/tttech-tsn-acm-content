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
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "status.h"

#include "sysfs.h"
#include "logging.h"
#include "hwconfig_def.h"
#include "libacmconfig_def.h"
#include "memory.h"

int64_t __must_check status_read_config_identifier() {
    int ret;

    ret = sysfs_read_configuration_id();
    return ret;
}

int __must_check status_get_buffer_id_from_name(const char *buffer) {
    int ret;
    int i, fd, anz_char_read, anz_msg_buffer;
    char path_name[SYSFS_PATH_LENGTH];
    struct acmdrv_buff_alias buffer_alias;

    if ( (!buffer) || (strlen(buffer) == 0)) {
        ret = -EINVAL;
        LOGERR("Status: no msg buffer name or name length zero");
        return ret;
    }
    /* construct pathname */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_MSGBUFF_ALIAS));
    if (ret != 0) {
        return ret;
    }
    anz_msg_buffer = get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_COUNT));

    /* read message buffer alias items from system filesysten and check if the
     * name in the alias item is equal to name in parameter buffer
     * special care for wrong strings in buffer ! */

    /* open file */
    fd = open(path_name, O_RDONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        return -errno;
    }
    /* read file */
    for (i = 0; i < anz_msg_buffer; i++) {
        anz_char_read = pread(fd,
                (char*) &buffer_alias,
                sizeof (buffer_alias),
                i * sizeof (buffer_alias));
        if (anz_char_read <= 0) {
            /* no data read - at end of data if return vallue 0, reading error
             * < 0*/
            LOGERR("Status: message buffer name not found, number of mess_buffs: %d", i);
            ret = -EACMBUFFNAME;
            goto end;
        }
        if (strcmp(buffer, acmdrv_buff_alias_alias_read(&buffer_alias)) == 0) {
            /* input buffer name found */
            ret = acmdrv_buff_alias_idx_read(&buffer_alias);
            goto end;
        }
    }
    /* name not found - for loop wasn't interrupted */
    ret = -EACMBUFFNAME;
    LOGERR("Status: message buffer name not found, number of mess_buffs: %d", i);
end:
    /* close file */
    close(fd);
    return ret;
}

int64_t __must_check status_read_buffer_locking_vector(void) {
    char path_name[SYSFS_PATH_LENGTH];
    uint64_t lock_vector;
    int ret;

    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONTROL_GROUP),
            __stringify(ACM_SYSFS_LOCK_BUFFMASK));
    if (ret != 0) {
        return ret;
    }

    lock_vector = read_buffer_sysfs_item(path_name, (char*) &lock_vector, sizeof (lock_vector), 0);

    return lock_vector;
}

int64_t __must_check status_read_item(enum acm_module_id module_id, enum acm_status_item id) {
    char *filename;
    int64_t ret;

    if (module_id >= ACM_MODULES_COUNT) {
        LOGERR("Status: module_id out of range: %d", module_id);
        return -EINVAL;
    }
    if (id >= STATUS_ITEM_NUM) {
        LOGERR("Status: acm_status_item out of range: %d", id);
        return -EINVAL;
    }

    ret = asprintf(&filename,
            ACMDEV_BASE "%s/%s_M%d",
            status_subpath[id].group,
            status_subpath[id].subpath,
            module_id);
    if (ret < 0) {
        LOGERR("Status: out of memory");
        return -ENOMEM;
    }

    /* read status from sysfs device */
    ret = read_uint64_sysfs_item(filename);
    free(filename);
    return ret;
}

struct acm_diagnostic *__must_check status_read_diagnostics(enum acm_module_id module_id) {
    struct acmdrv_diagnostics packed_diag_values;
    int ret;
    char *filename;
    struct acm_diagnostic *unpacked_diagnostics = NULL;

    ret = asprintf(&filename, ACMDEV_BASE "%s/%s_M%d", __stringify(ACMDRV_SYSFS_DIAG_GROUP),
            __stringify(ACM_SYSFS_DIAGNOSTICS_FILE), module_id);
    if (ret < 0) {
        LOGERR("Status: out of memory");
        return NULL;
    }

    /* read data from file */
    ret = read_buffer_sysfs_item(filename,
            &packed_diag_values,
            sizeof(packed_diag_values),
            0);

    /* check and convert read data */
    if (ret == 0) {
        unpacked_diagnostics = acm_zalloc(sizeof(struct acm_diagnostic));
        if (!unpacked_diagnostics) {
            LOGERR("Status: out of memory");
        } else {
            convert_diag2unpacked(&packed_diag_values, unpacked_diagnostics);
        }
    } else {
        /* problem while reading data */
        LOGERR("Status: problem reading data from file %s", filename);
    }

    free(filename);
    return unpacked_diagnostics;
}

int __must_check status_set_diagnostics_poll_time(enum acm_module_id module_id,
        uint16_t interval_ms) {
    int ret;
    char *value;
    char *filename;

    ret = asprintf(&filename, ACMDEV_BASE "%s/%s_M%d", __stringify(ACMDRV_SYSFS_DIAG_GROUP),
            __stringify(ACM_SYSFS_DIAG_POLL_TIME), module_id);
    if (ret < 0) {
        LOGERR("Status: out of memory");
        return -ENOMEM;
    }

    /* convert value to string and write it with string end to file which content is deleted before */
    ret = asprintf(&value, "%d", interval_ms);
    if (ret < 0) {
        LOGERR("Status: out of memory");
        ret = -ENOMEM;
        goto out;
    }

    sysfs_delete_file_content(filename);
    ret = write_file_sysfs(filename, value, strlen(value) + 1, 0);
    free(value);
out:
    free(filename);
    return ret;
}

int32_t __must_check status_read_capability_item(enum acm_capability_item id) {
    int ret = -EINVAL;

    switch (id) {
        case CAP_MIN_SCHEDULE_TICK:
            ret = calc_tick_duration();
            break;
        case CAP_MAX_MESSAGE_BUFFER_SIZE:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_SIZE));
            break;
        case CAP_CONFIG_READBACK:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_READ_BACK));
            break;
        case CAP_DEBUG:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_DEBUG));
            break;
        case CAP_MAX_ANZ_MESSAGE_BUFFER:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_COUNT));
            break;
        case CAP_MESSAGE_BUFFER_BLOCK_SIZE:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH));
            break;
        case CAP_REDUNDANCY_RX:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_RX_REDUNDANCY));
            break;
        case CAP_INDIV_RECOVERY:
            ret = get_int32_status_value(__stringify(ACM_SYSFS_INDIV_RECOV));
            break;
        default:
            LOGERR("Status: unknown capability item id: %d", id);
    }
    return ret;
}

int64_t __must_check status_read_time_freq() {
    char path_name[SYSFS_PATH_LENGTH];
    int ret;

    /* construct path name */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_STATUS_GROUP),
            __stringify(ACM_SYSFS_TIME_FREQ_FILE));
    if (ret != 0) {
        return ret;
    }

    return read_uint64_sysfs_item(path_name);
}

static char *get_string_status_value(const char *filename) {
    int ret;
    char path_name[SYSFS_PATH_LENGTH];
    char buffer[SYSFS_PATH_LENGTH] = { 0 };

    ret = sysfs_construct_path_name(path_name, sizeof(path_name),
            __stringify(ACMDRV_SYSFS_STATUS_GROUP), filename);
    if (ret < 0)
        return NULL;

    ret = read_buffer_sysfs_item(path_name, buffer, sizeof(buffer), 0);
    if (ret < 0)
        return NULL;

    /* truncate at CR or LF */
    buffer[strcspn(buffer, "\n\r")] = 0;
    return strdup(buffer);
}

char* __must_check status_get_ip_version(void) {
    int ret;
    char *device_id;
    char *version_id;
    char *revision_id;
    char *ip_version = NULL;

    device_id = get_string_status_value(__stringify(ACM_SYSFS_DEVICE));
    if (!device_id)
        return NULL;
    version_id = get_string_status_value(__stringify(ACM_SYSFS_VERSION));
    if (!version_id)
        goto free_device_id;
    revision_id = get_string_status_value(__stringify(ACM_SYSFS_REVISION));
    if (!revision_id)
        goto free_version_id;

    ret = asprintf(&ip_version, "%s-%s-%s", device_id, version_id, revision_id);
    if (ret < 0) {
        LOGERR("Status: out of memory");
        ip_version = NULL;
    }

    free(revision_id);
free_version_id:
    free(version_id);
free_device_id:
    free(device_id);

    return ip_version;
}

int32_t __must_check get_int32_status_value(const char *filename) {
    char path_name[SYSFS_PATH_LENGTH];
    int ret;

    /* construct path name */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_STATUS_GROUP),
            filename);
    if (ret != 0) {
        return ret;
    }

    return (int32_t) read_uint64_sysfs_item(path_name);
}

void convert_diag2unpacked(struct acmdrv_diagnostics *source, struct acm_diagnostic *destination) {
    /* copy from packed to unpacked structure must be done item per item */
    int i;

    for (i = 0; i < ACM_MAX_LOOKUP_SIZE; i++) {
        destination->additionalFilterMismatchCounter[i] =
                source->additionalFilterMismatchCounter[i];
        destination->ingressWindowClosedCounter[i] = source->ingressWindowClosedCounter[i];
        destination->noFrameReceivedCounter[i] = source->noFrameReceivedCounter[i];
        destination->recoveryCounter[i] = source->recoveryCounter[i];
    }
    destination->additionalFilterMismatchFlags = source->additionalFilterMismatchFlags;
    destination->ingressWindowClosedFlags = source->ingressWindowClosedFlags;
    destination->noFrameReceivedFlags = source->noFrameReceivedFlags;
    destination->recoveryFlags = source->recoveryFlags;
    destination->rxFramesCounter = source->rxFramesCounter;
    destination->scheduleCycleCounter = source->scheduleCycleCounter;
    destination->timestamp.tv_sec = source->timestamp.tv_sec;
    destination->timestamp.tv_nsec = source->timestamp.tv_nsec;
    destination->txFramesCounter = source->txFramesCounter;

    return;
}

int32_t __must_check calc_tick_duration() {
    double time_freq, tick_duration = 10;

    time_freq = status_read_time_freq();

    if (time_freq <= 0) {
        return time_freq;
    }
    tick_duration = 1E9 / time_freq; // multiply with 1E9 to get nanoseconds
    return tick_duration;
}
