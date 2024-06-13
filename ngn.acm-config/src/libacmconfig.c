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
#include <errno.h>
#include <net/ethernet.h> /* for ETHER_ADDR_LEN */

#include "libacmconfig.h"
#include "libacmconfig_def.h"
#include "operation.h"
#include "schedule.h"
#include "config.h"
#include "lookup.h"
#include "stream.h"
#include "status.h"
#include "logging.h"
#include "tracing.h"
#include "validate.h"
#include "sysfs.h"
#include "hwconfig_def.h"

#define ACMAPI __attribute__((visibility("default")))

ACMAPI int64_t __must_check acm_read_status_item(enum acm_module_id module_id,
        enum acm_status_item status_id) {
    TRACE1_MSG("Executing. module_id=%d, status_id=%d", module_id, status_id);
    return status_read_item(module_id, status_id);
}

ACMAPI int64_t __must_check acm_read_config_identifier() {
    TRACE1_MSG("Executing");
    return status_read_config_identifier();
}

ACMAPI struct acm_diagnostic* __must_check acm_read_diagnostics(enum acm_module_id module_id) {
    TRACE1_MSG("Executing. module_id=%d", module_id);
    return status_read_diagnostics(module_id);
}


ACMAPI int __must_check acm_set_diagnostics_poll_time(enum acm_module_id module_id,
        uint16_t interval_ms) {
    TRACE1_MSG("Executing. module_id=%d, interval=%d", module_id, interval_ms);
    return status_set_diagnostics_poll_time(module_id, interval_ms);
}

ACMAPI int32_t __must_check acm_read_capability_item(enum acm_capability_item item_id) {
    TRACE1_MSG("Executing. item_id=%d", item_id);
    return status_read_capability_item(item_id);
}

ACMAPI const char* __must_check acm_read_lib_version() {
    TRACE1_MSG("Executing.");
    return GIT_VERSION_STR;
}

ACMAPI char* __must_check acm_read_ip_version() {
    TRACE1_MSG("Executing.");
    return status_get_ip_version();
}

ACMAPI int __must_check acm_get_buffer_id(const char *buffer) {
    TRACE1_MSG("buffer=%s", buffer);
    return status_get_buffer_id_from_name(buffer);
}

ACMAPI int64_t __must_check acm_read_buffer_locking_vector() {
    TRACE1_MSG("Executing.");
    return status_read_buffer_locking_vector();
}

ACMAPI int __must_check acm_set_buffer_locking_mask(uint64_t locking_vector) {
    TRACE1_MSG("vector=0x%x", locking_vector);
    return sysfs_write_buffer_control_mask(locking_vector, __stringify(ACM_SYSFS_LOCK_BUFFMASK));
}

ACMAPI int __must_check acm_set_buffer_unlocking_mask(uint64_t unlocking_vector) {
    TRACE1_MSG("vector=0x%x", unlocking_vector);
    return sysfs_write_buffer_control_mask(unlocking_vector, __stringify(ACM_SYSFS_UNLOCK_BUFFMASK));
}

ACMAPI int __must_check acm_add_stream_operation_insert(struct acm_stream *stream,
        uint16_t length,
        const char *buffer) {
    struct operation *operation;
    int ret;

    TRACE1_MSG("length=%d, buffer=%s", length, buffer);
    if (stream_config_applied(stream)) {
        LOGERR("Libacmconfig: configuration of stream already applied to HW");
        return -EPERM;
    }
    operation = operation_create_insert(length, buffer);
    if (!operation)
        return -ENOMEM;

    ret = stream_add_operation(stream, operation);
    if (ret != 0) {
        operation_destroy(operation);
    } else {
        /* operation was added successfully - recalculate Gather DMA indizes
         * if stream was already added to a module */
        if (ACMLIST_REF(stream, entry)) {
            calculate_gather_indizes(ACMLIST_REF(stream, entry));
        }
    }
    return ret;
}

ACMAPI int __must_check acm_add_stream_operation_insertconstant(struct acm_stream *stream,
        const char *content,
        uint16_t size) {
    struct operation *operation;
    int ret;

    TRACE1_MSG("size=%d", size);
    if (stream_config_applied(stream)) {
        LOGERR("Libacmconfig: configuration of stream already applied to HW");
        return -EPERM;
    }
    operation = operation_create_insertconstant(content, size);
    if (!operation)
        return -ENOMEM;

    ret = stream_add_operation(stream, operation);
    if (ret != 0) {
        operation_destroy(operation);
    } else {
        /* operation was added successfully - recalculate Gather DMA indizes
         * if stream was already added to a module */
        if (ACMLIST_REF(stream, entry)) {
            calculate_gather_indizes(ACMLIST_REF(stream, entry));
        }
    }
    return ret;
}

ACMAPI int __must_check acm_add_stream_operation_pad(struct acm_stream *stream,
        uint16_t length,
        const char value) {
    struct operation *operation;
    int ret;

    TRACE1_MSG("length=%d, value=0x%02x", length, value);
    if (stream_config_applied(stream)) {
        LOGERR("Libacmconfig: configuration of stream already applied to HW");
        return -EPERM;
    }
    operation = operation_create_pad(length, value);
    if (!operation)
        return -ENOMEM;

    ret = stream_add_operation(stream, operation);
    if (ret != 0) {
        operation_destroy(operation);
    } else {
        /* operation was added successfully - recalculate Gather DMA indizes
         * if stream was already added to a module */
        if (ACMLIST_REF(stream, entry)) {
            calculate_gather_indizes(ACMLIST_REF(stream, entry));
        }
    }
    return ret;
}

ACMAPI int __must_check acm_add_stream_operation_forward(struct acm_stream *stream,
        uint16_t offset,
        uint16_t length) {
    struct operation *operation;
    int ret;

    TRACE1_MSG("offset=%d, length=%d", offset, length);
    if (stream_config_applied(stream)) {
        LOGERR("Libacmconfig: configuration of stream already applied to HW");
        return -EPERM;
    }
    operation = operation_create_forward(offset, length);
    if (!operation)
        return -ENOMEM;

    ret = stream_add_operation(stream, operation);
    if (ret != 0) {
        operation_destroy(operation);
    } else {
        /* operation was added successfully - recalculate Gather DMA indizes
         * if stream was already added to a module */
        if (ACMLIST_REF(stream, entry)) {
            calculate_gather_indizes(ACMLIST_REF(stream, entry));
        }
    }

    return ret;
}

ACMAPI int __must_check acm_add_stream_operation_read(struct acm_stream *stream,
        uint16_t offset,
        uint16_t length,
        const char *buffer) {
    struct operation *operation;
    int ret;

    TRACE1_MSG("offset=%d, length=%d, buffer=%s", offset, length, buffer);
    if (stream_config_applied(stream)) {
        LOGERR("Libacmconfig: configuration of stream already applied to HW");
        return -EPERM;
    }
    operation = operation_create_read(offset, length, buffer);
    if (!operation)
        return -ENOMEM;

    ret = stream_add_operation(stream, operation);
    if (ret != 0) {
        operation_destroy(operation);
    } else {
        /* operation was added successfully - recalculate Gather DMA indizes
         * if stream was already added to a module */
        if (ACMLIST_REF(stream, entry)) {
            calculate_scatter_indizes(ACMLIST_REF(stream, entry));
        }
    }
    return ret;
}

ACMAPI int __must_check acm_add_stream_operation_forwardall(struct acm_stream *stream) {
    struct operation *operation;
    int ret;

    TRACE1_MSG("Executing.");
    if (stream_config_applied(stream)) {
        LOGERR("Libacmconfig: configuration of stream already applied to HW");
        return -EPERM;
    }
    operation = operation_create_forwardall();
    if (!operation)
        return -ENOMEM;

    ret = stream_add_operation(stream, operation);
    if (ret != 0) {
        operation_destroy(operation);
    } else {
        /* operation was added successfully - recalculate Gather DMA indizes
         * if stream was already added to a module */
        if (ACMLIST_REF(stream, entry)) {
            calculate_gather_indizes(ACMLIST_REF(stream, entry));
        }
    }
    return ret;
}

ACMAPI void acm_clean_operations(struct acm_stream *stream) {
    enum stream_type streamtype;
    struct acm_module *module;
    struct stream_list *streamlist;

    TRACE1_MSG("Executing.");
    if (!stream) {
        LOGERR("Libacmconfig: wrong parameter in %s", __func__);
        return;
    }
    /* check if configuration to which the stream belongs to was already
     * applied to the HW. If this is the case the function to clean operations
     * shall fail - not clean anything. */
    streamlist = ACMLIST_REF(stream, entry);
    if (streamlist) {
        module = streamlist_to_module(streamlist);
        if (module->config_reference) {
            struct acm_config *config;

            config = module->config_reference;
            if (config->config_applied) {
                LOGERR("Libacmconfig: Config. already applied to HW.");
                return;
            }
        }
    }
    /* check the stream type: in case of egress streams the
     * first 3 operations have to be kept in the list */
    streamtype = stream->type;
    if ( (streamtype == TIME_TRIGGERED_STREAM) || (streamtype == EVENT_STREAM)
            || (streamtype == RECOVERY_STREAM) || (streamtype == REDUNDANT_STREAM_TX)) {
        operation_list_flush_user(& (stream->operations));
    } else {
        // stream is INGRESS_TRIGGERED_STREAM or REDUNDANT_STREAM_RX
        operation_list_flush(& (stream->operations));
    }

    if (ACMLIST_REF(stream, entry)) {
        /*stream was already added to module, thus indizes for writing operations
         * to hardware have to be calculated newly */
        calculate_scatter_indizes(ACMLIST_REF(stream, entry));
        calculate_gather_indizes(ACMLIST_REF(stream, entry));
    }
}

ACMAPI int __must_check acm_add_stream_schedule_event(struct acm_stream *stream,
        uint32_t period_ns,
        uint32_t send_time_ns) {
    struct schedule_entry *schedule;
    int result;

    TRACE1_MSG("period_ns=%d, send_time_ns=%d", period_ns, send_time_ns);

    if (!stream) {
        LOGERR("Libacmconfig: no stream as input");
        return -EINVAL;
    }

    if ( (stream->type != TIME_TRIGGERED_STREAM) && (stream->type != REDUNDANT_STREAM_TX)) {
        LOGERR("Invalid stream type");
        return -EPERM;
    }

    if (send_time_ns > period_ns) {
        LOGERR("Libacmconfig: send time not within period");
        return -EINVAL;
    }

    schedule = schedule_create(0, 0, send_time_ns, period_ns);
    if (!schedule)
        return -ENOMEM;

    result = schedule_list_add_schedule(&stream->windows, schedule);
    if (result != 0) {
        // There was a problem at creation of schedule item
        return result;
    }

    result = module_add_schedules(stream, schedule);
    if (result != 0) {
        /* There was a problem at creation of fsc items. Schedule item has
         to be removed from stream and deleted */
        schedule_list_remove_schedule(&stream->windows, schedule);
    }
    return result;
}

ACMAPI int __must_check acm_add_stream_schedule_window(struct acm_stream *stream,
        uint32_t period_ns,
        uint32_t time_start_ns,
        uint32_t time_end_ns) {
    struct schedule_entry *schedule;
    int result;

    TRACE1_MSG("period=%d, t_start_ns=%d, t_end_ns=%d", period_ns, time_start_ns, time_end_ns);
    if (!stream) {
        LOGERR("Libacmconfig: wrong parameter in %s", __func__);
        return -EINVAL;
    }
    if ( (stream->type != INGRESS_TRIGGERED_STREAM) && (stream->type != REDUNDANT_STREAM_RX)) {
        LOGERR("Invalid stream type");
        return -EPERM;
    }
    if ( (time_start_ns > period_ns) || (time_end_ns > period_ns)) {
        LOGERR("Libacmconfig: window start or window end not within period");
        return -EINVAL;
    }
    /* if (time_start_ns > time_end_ns) {
     LOGERR("Libacmconfig: window start later than window end");
     return -EINVAL;
     } -> check removed as it is a reasonable case that end time is before start	*/

    schedule = schedule_create(time_start_ns, time_end_ns, 0, period_ns);
    if (!schedule)
        return -ENOMEM;

    result = schedule_list_add_schedule(&stream->windows, schedule);
    if (result != 0) {
        // There was a problem at creation of schedule item
        return result;
    }

    result = module_add_schedules(stream, schedule);
    if (result != 0) {
        /* There was a problem at creation of fsc items. Schedule item has
         to be removed from stream and deleted */
        schedule_list_remove_schedule(&stream->windows, schedule);
    }
    return result;
}

ACMAPI void acm_clean_schedule(struct acm_stream *stream) {
    TRACE1_MSG("Executing.");
    if (!stream) {
        LOGERR("Libacmconfig: wrong parameter in %s", __func__);
        return;
    }
    /*  check if stream was already added to a module. if this is the case,
     then first the fsc_schedule items have to be deleted */
    if (ACMLIST_REF(stream, entry)) {
        struct acm_module *module;

        module = streamlist_to_module(ACMLIST_REF(stream, entry));
        remove_schedule_sysfs_items_stream(stream, module);
    }
    schedule_list_flush(&stream->windows);
}

ACMAPI struct acm_stream __must_check* acm_create_time_triggered_stream(uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan_id,
        uint8_t vlan_priority) {
    struct acm_stream *stream;
    int result;

    TRACE1_MSG("Executing.");
    result = stream_check_vlan_parameter(vlan_id, vlan_priority);
    if (result != 0) {
        return NULL;
    }

    stream = stream_create(TIME_TRIGGERED_STREAM);
    if (!stream) {
        return NULL;
    }

    result = stream_set_egress_header(stream, dmac, smac, vlan_id, vlan_priority);
    if (result != 0) {
        stream_destroy(stream);
        return NULL;
    }
    return stream;
}

ACMAPI struct acm_stream __must_check* acm_create_ingress_triggered_stream(uint8_t header[ACM_MAX_LOOKUP_SIZE],
        uint8_t header_mask[ACM_MAX_LOOKUP_SIZE],
        uint8_t *additional_filter,
        uint8_t *additional_filter_mask,
        uint16_t size) {
    struct acm_stream *stream;
    struct lookup *lookup;

    TRACE1_MSG("Executing.");
    stream = stream_create(INGRESS_TRIGGERED_STREAM);
    if (!stream) {
        return NULL;
    }
    lookup = lookup_create(header, header_mask, additional_filter, additional_filter_mask, size);
    if (!lookup) {
        stream_destroy(stream);
        return NULL;
    }
    stream->lookup = lookup;

    return stream;
}

ACMAPI struct acm_stream __must_check* acm_create_event_stream(uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan,
        uint8_t prio) {
    struct acm_stream *stream;
    int result;

    TRACE1_MSG("Executing.");
    result = stream_check_vlan_parameter(vlan, prio);
    if (result != 0) {
        return NULL;
    }

    stream = stream_create(EVENT_STREAM);
    if (!stream) {
        return NULL;
    }

    result = stream_set_egress_header(stream, dmac, smac, vlan, prio);
    if (result != 0) {
        stream_destroy(stream);
        return NULL;
    }
    return stream;
}

ACMAPI struct acm_stream __must_check* acm_create_recovery_stream(uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan,
        uint8_t prio) {
    struct acm_stream *stream;
    int result;

    TRACE1_MSG("Executing.");
    result = stream_check_vlan_parameter(vlan, prio);
    if (result != 0) {
        return NULL;
    }

    stream = stream_create(RECOVERY_STREAM);
    if (!stream) {
        return NULL;
    }
    result = stream_set_egress_header(stream, dmac, smac, vlan, prio);
    if (result != 0) {
        stream_destroy(stream);
        return NULL;
    }
    return stream;
}

ACMAPI void acm_destroy_stream(struct acm_stream *stream) {
    TRACE1_MSG("Executing.");
    stream_delete(stream);
}

ACMAPI int __must_check acm_set_reference_stream(struct acm_stream *stream,
        struct acm_stream *reference) {
    TRACE1_MSG("Executing.");
    return stream_set_reference(stream, reference);
}

ACMAPI int __must_check acm_set_rtag_stream(struct acm_stream *stream, uint32_t timeout_ms) {
    TRACE1_MSG("Executing.");
    return stream_set_indiv_recov(stream, timeout_ms);
}

ACMAPI struct acm_module __must_check* acm_create_module(enum acm_connection_mode mode,
        enum acm_linkspeed speed,
        enum acm_module_id module_id) {
    TRACE1_MSG("Executing.");
    return module_create(mode, speed, module_id);
}

ACMAPI void acm_destroy_module(struct acm_module *module) {
    TRACE1_MSG("Executing.");
    if (!module)
        return;
    if (module->config_reference) {
        LOGERR("Module: Destroy not possible - added to config");
        return;
    }
    module_destroy(module);
}

ACMAPI int __must_check acm_set_module_schedule(struct acm_module *module,
        uint32_t cycle_ns,
        struct timespec start) {
    TRACE1_MSG("Executing.");
    return module_set_schedule(module, cycle_ns, start);
}

ACMAPI int __must_check acm_add_module_stream(struct acm_module *module, struct acm_stream *stream) {
    int ret;

    TRACE1_MSG("Executing.");
    if (!module || !stream) {
        LOGERR("Module: Invalid stream input");
        return -EINVAL;
    }
    if (module->config_reference) {
        if (module->config_reference->config_applied) {
            LOGERR("Module: Associated configuration already applied to HW");
            return -EPERM;
        }
    }
    if ( (stream->type != INGRESS_TRIGGERED_STREAM) && (stream->type != TIME_TRIGGERED_STREAM)) {
        LOGERR("Module: only Time Triggered and Ingress Triggered streams can be added");
        return -EINVAL;

    }
    ret = module_add_stream(module, stream);
    if (ret != 0) {
        return ret;
    }
    /* For software internal administrative reasons child streams of Ingress
     * Triggered Streams are also inserted into the streamlist of the module. */
    if ( (stream->type == INGRESS_TRIGGERED_STREAM) && (stream->reference)) {
        struct acm_stream *child;

        ret = module_add_stream(module, stream->reference);
        if (ret != 0) {
            return ret;
        }
        child = stream->reference;
        /* If child stream Event Stream has a child Recovery Stream, Recovery
         * Stream is also inserted into the streamlist of the module */
        if (child->reference) {
            ret = module_add_stream(module, child->reference);
            if (ret != 0) {
                return ret;
            }

        }
    }

    return 0;
}

ACMAPI struct __must_check acm_config* acm_create() {
    TRACE1_MSG("Executing.");
    return config_create();
}

ACMAPI void acm_destroy(struct acm_config *config) {
    TRACE1_MSG("Executing.");
    config_destroy(config);
}

ACMAPI int __must_check acm_add_module(struct acm_config *config, struct acm_module *module) {
    TRACE1_MSG("Executing.");
    return config_add_module(config, module);
}

ACMAPI int __must_check acm_validate_stream(struct acm_stream *stream) {
    TRACE1_MSG("Executing.");
    return validate_stream(stream, true);
}

ACMAPI int __must_check acm_validate_module(struct acm_module *module) {
    TRACE1_MSG("Executing.");
    return validate_module(module, true);
}

ACMAPI int __must_check acm_validate_config(struct acm_config *config) {
    TRACE1_MSG("Executing.");
    return validate_config(config, true);
}

ACMAPI int __must_check acm_apply_config(struct acm_config *config, uint32_t identifier) {
    TRACE1_MSG("Executing.");
    return config_enable(config, identifier);
}

ACMAPI int __must_check acm_apply_schedule(struct acm_config *config,
        uint32_t identifier,
        uint32_t identifier_expected) {
    TRACE1_MSG("Executing.");
    return config_schedule(config, identifier, identifier_expected);
}

ACMAPI int __must_check acm_disable_config(void) {
    TRACE1_MSG("Executing.");
    return config_disable();
}
