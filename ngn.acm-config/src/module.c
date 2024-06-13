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

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "module.h"
#include "config.h"
#include "logging.h"
#include "tracing.h"
#include "stream.h"
#include "memory.h"
#include "libacmconfig_def.h"
#include "validate.h"
#include "sysfs.h"
#include "status.h"

struct acm_module __must_check* module_create(enum acm_connection_mode mode,
        enum acm_linkspeed speed,
        enum acm_module_id module_id) {
    struct acm_module *module;

    TRACE2_ENTER();
    if (module_id >= ACM_MODULES_COUNT) {
        LOGERR("Module: module_id out of range: %d", module_id);
        TRACE2_MSG("Fail");
        return NULL;
    }

    module = acm_zalloc(sizeof (*module));
    if (!module) {
        LOGERR("Module: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    module->mode = mode;
    module->module_id = module_id;
    module->config_reference = NULL;
    module->speed = speed;
    // set init values are for module cycle and start time
    module->cycle_ns = 0;
    module->start.tv_nsec = 0;
    module->start.tv_sec = 0;

    /* module->speed must be set before initialization of delays */
    module_init_delays(module);

    ACMLIST_INIT(&module->streams);
    ACMLIST_INIT(&module->fsc_list);

    TRACE2_EXIT();
    return module;
}

void module_destroy(struct acm_module *module) {
    TRACE3_ENTER();
    if (!module)
        return;

    fsc_command_empty_list(&module->fsc_list);
    stream_empty_list(&module->streams);
    acm_free(module);
    TRACE3_EXIT();
}

int __must_check module_add_stream(struct acm_module *module, struct acm_stream *stream) {
    int ret, ret_unimportant;
    struct schedule_list *winlist;
    struct schedule_entry *window;

    TRACE2_ENTER();
    if (!module || !stream) {
        LOGERR("Module: module or stream id invalid; module: %d, stream: %d", module, stream);
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    ret = stream_add_list(&module->streams, stream);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* create HW representatives for schedules in stream. Validation of
     * module is done in function module_add_schedules */
    winlist = &stream->windows;
    ACMLIST_LOCK(winlist);

    ACMLIST_FOREACH(window, winlist, entry)
    {
        ret = module_add_schedules(stream, window);
        if (ret != 0) {
            ACMLIST_UNLOCK(winlist);
            goto problem_fsc_creation;
        }
    }
    ACMLIST_UNLOCK(winlist);

    TRACE2_EXIT();
    return 0;

problem_fsc_creation:
    /* There was a problem at creation of HW representatives for schedule items.
     * All already created fsc schedule items have to be deleted again. In loop
     * "ACMLIST_FOREACH(window, winlist, entry)" above only the fsc
     * schedule items of the last window worked on, are deleted.
     * Also the stream has to be removed from the module again. */
    remove_schedule_sysfs_items_stream(stream, module);
    stream_remove_list(&module->streams, stream);
    ret_unimportant = calculate_indizes_for_HW_tables(&module->streams, stream);
    if (ret_unimportant != 0) {
        LOGWARN("Module: some inconsistency caused by removing stream again");
    }

    return ret;
}

int __must_check module_set_schedule(struct acm_module *module,
        uint32_t cycle_ns,
        struct timespec start) {
    TRACE2_ENTER();
    if (!module) {
        LOGERR("Module: Invalid module input: %d", module);
        TRACE2_MSG("Fail");
        return -EINVAL;
    }
    if (cycle_ns == 0) {
        LOGERR("Module: Cycle Time needs value greater 0");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    module->cycle_ns = cycle_ns;
    module->start = start;

    TRACE2_EXIT();
    return 0;
}

int __must_check module_add_schedules(struct acm_stream *stream, struct schedule_entry *schedule) {

    struct stream_list *streamlist;
    int result, tick_duration;
    bool recovery;
    uint16_t gather_dma_index;

    TRACE2_ENTER();
    if (!ACMLIST_REF(stream, entry)) {
        LOGERR("Module: streamlist reference not set in stream");
        TRACE2_MSG("Fail");
        return -EFAULT;
    } else {
        streamlist = ACMLIST_REF(stream, entry);
    }

    tick_duration = calc_tick_duration();
    if (tick_duration <= 0) {
        LOGERR("Module: Invalid value for tick duration: %d", tick_duration);
        TRACE2_MSG("Fail");
        return -EINVAL;
    }
    switch (stream->type) {
        case TIME_TRIGGERED_STREAM:
        case REDUNDANT_STREAM_TX:
            result = create_event_sysfs_items(schedule,
                    streamlist_to_module(streamlist),
                    tick_duration,
                    stream->gather_dma_index,
                    stream->redundand_index);
            break;
        case INGRESS_TRIGGERED_STREAM:
        case REDUNDANT_STREAM_RX:
            recovery = false;
            gather_dma_index = 0;
            if (stream->reference) {
                if (stream->reference->reference) {
                    /* Ingress Triggered Stream has an Event Stream and a
                     * Recovery Stream*/
                    recovery = true;
                    gather_dma_index = stream->reference->reference->gather_dma_index;
                }
            }
            result = create_window_sysfs_items(schedule,
                    streamlist_to_module(streamlist),
                    tick_duration,
                    gather_dma_index,
                    stream->lookup_index,
                    recovery);
            break;
        default:
            LOGERR("Module: Invalid stream type: %d", stream->type);
            TRACE2_MSG("Fail");
            return -EPERM;
    }

    if (result != 0) {
        LOGERR("Module: problem at creation of sysfs schedules");
        remove_schedule_sysfs_items_schedule(schedule, streamlist_to_module(streamlist));
        TRACE2_MSG("Fail");
        return result;
    }

    result = validate_stream(stream, false);
    if (result != 0) {
        LOGERR("Module: Validation not successful");
        remove_schedule_sysfs_items_schedule(schedule, streamlist_to_module(streamlist));
        TRACE2_MSG("Fail");
        return result;
    }
    // everything okay
    TRACE2_EXIT();
    return 0;
}

void remove_schedule_sysfs_items_schedule(struct schedule_entry *schedule_item,
        struct acm_module *module) {
    struct fsc_command_list *fsc_list;
    struct fsc_command *fsc_item;

    TRACE2_ENTER();
    fsc_list = &module->fsc_list;
    ACMLIST_LOCK(fsc_list);
    ACMLIST_FOREACH(fsc_item, fsc_list, entry)
    {
        if (fsc_item->schedule_reference == schedule_item) {
            _ACMLIST_REMOVE(fsc_list, fsc_item, entry);
            acm_free(fsc_item);
        }
    }
    ACMLIST_UNLOCK(fsc_list);
    TRACE2_EXIT();
    return;
}

void remove_schedule_sysfs_items_stream(struct acm_stream *stream, struct acm_module *module) {
    struct schedule_list *winlist;
    struct schedule_entry *window;

    TRACE2_ENTER();
    if (!stream) {
        LOGERR("Module: no stream as input in %s", __func__);
        TRACE2_MSG("Fail");
        return;
    }
    if (!module) {
        LOGERR("Module: no module as input in %s", __func__);
        TRACE2_MSG("Fail");
        return;
    }

    winlist = &stream->windows;
    ACMLIST_LOCK(winlist);

    ACMLIST_FOREACH(window, winlist, entry)
    {
        remove_schedule_sysfs_items_schedule(window, module);
    }
    ACMLIST_UNLOCK(winlist);
    TRACE2_EXIT();
    return;
}

void fsc_command_empty_list(struct fsc_command_list *fsc_list) {
    TRACE2_ENTER();
    if (!fsc_list)
        return;

    ACMLIST_LOCK(fsc_list);

    while (!ACMLIST_EMPTY(fsc_list)) {
        struct fsc_command *fsc_item;

        fsc_item = ACMLIST_FIRST(fsc_list);
        _ACMLIST_REMOVE(fsc_list, fsc_item, entry);

        acm_free(fsc_item);
    }

    ACMLIST_UNLOCK(fsc_list);

    TRACE2_EXIT();
}

int __must_check write_module_data_to_HW(struct acm_module *module) {
    int ret;

    TRACE2_ENTER();
    // write constant buffer data from  operations insert_constant to HW
    ret = sysfs_write_data_constant_buffer(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write lookup tables and stream triggers
    ret = sysfs_write_lookup_tables(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // write scatters DMA command table
    ret = sysfs_write_scatter_dma(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* write  Gather DMA command table and Prefetch command table
     * It is important that this table is written after constant buffer data.
     * Because constant buffer offset needed for writing Gather DMA are only
     * calculated at writing constant buffer data to HW. */
    ret = sysfs_write_prefetcher_gather_dma(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // 5.1 write connection mode
    ret = sysfs_write_connection_mode_to_HW(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // write redundancy control table to HW
    ret = sysfs_write_redund_ctrl_table(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write base recovery timer
    ret = sysfs_write_individual_recovery(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    /* write control table/control block items, which aren't written together
     * with other topics - only speed left */
    ret = sysfs_write_cntl_speed(module);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // enable the module
    ret = sysfs_write_module_enable(module, true);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check write_module_schedule_to_HW(struct acm_module *module) {
    int ret, free_table;
    struct acmdrv_sched_emerg_disable emergency_disable;

    TRACE2_ENTER();
    //  find free schedule table
    ret = sysfs_read_schedule_status(module, &free_table);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // write fsc schedule table
    ret = write_fsc_schedules_to_HW(module, free_table);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* write module cycle time, start time; after writing start time, scheduler
     starts automatically */
    ret = write_module_schedules_to_HW(module, free_table);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // Flash Emergency-Disable
    emergency_disable.eme_dis = 0;
    ret = sysfs_write_emergency_disable(module, &emergency_disable);
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    TRACE2_EXIT();
    return 0;
}

void module_clean_msg_buff_links(struct acm_module *module) {
    struct acm_stream *stream;
    struct stream_list *streamlist;

    TRACE2_ENTER();
    if (!module) {
        TRACE2_EXIT();
        return;   // not an error, but no operations with links to HW message buffers
    }
    streamlist = &module->streams;
    ACMLIST_LOCK(&module->streams);;
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        stream_clean_msg_buff_links(stream);
    }
    ACMLIST_UNLOCK(&module->streams);;
    TRACE2_EXIT();
}

void module_init_delays(struct acm_module *module) {
    int ret;

    /* read delay values from configuration file, if no reliable value got
     * from configuration file, use default delay values from schedule_delays */
    ret = module_get_delay_value(KEY_CHIP_IN_100MBps,
            &module->module_delays[SPEED_100MBps].chip_in);
    if (ret < 0) {
        module->module_delays[SPEED_100MBps].chip_in = schedule_delays[SPEED_100MBps].chip_in;
    }
    ret = module_get_delay_value(KEY_CHIP_EG_100MBps,
            &module->module_delays[SPEED_100MBps].chip_eg);
    if (ret < 0) {
        module->module_delays[SPEED_100MBps].chip_eg = schedule_delays[SPEED_100MBps].chip_eg;
    }
    ret = module_get_delay_value(KEY_PHY_IN_100MBps, &module->module_delays[SPEED_100MBps].phy_in);
    if (ret < 0) {
        module->module_delays[SPEED_100MBps].phy_in = schedule_delays[SPEED_100MBps].phy_in;
    }
    ret = module_get_delay_value(KEY_PHY_EG_100MBps, &module->module_delays[SPEED_100MBps].phy_eg);
    if (ret < 0) {
        module->module_delays[SPEED_100MBps].phy_eg = schedule_delays[SPEED_100MBps].phy_eg;
    }
    ret = module_get_delay_value(KEY_SER_BYPASS_100MBps,
            &module->module_delays[SPEED_100MBps].ser_bypass);
    if (ret < 0) {
        module->module_delays[SPEED_100MBps].ser_bypass = schedule_delays[SPEED_100MBps].ser_bypass;
    }
    ret = module_get_delay_value(KEY_SER_SWITCH_100MBps,
            &module->module_delays[SPEED_100MBps].ser_switch);
    if (ret < 0) {
        module->module_delays[SPEED_100MBps].ser_switch = schedule_delays[SPEED_100MBps].ser_switch;
    }

    ret = module_get_delay_value(KEY_CHIP_IN_1GBps, &module->module_delays[SPEED_1GBps].chip_in);
    if (ret < 0) {
        module->module_delays[SPEED_1GBps].chip_in = schedule_delays[SPEED_1GBps].chip_in;
    }
    ret = module_get_delay_value(KEY_CHIP_EG_1GBps, &module->module_delays[SPEED_1GBps].chip_eg);
    if (ret < 0) {
        module->module_delays[SPEED_1GBps].chip_eg = schedule_delays[SPEED_1GBps].chip_eg;
    }
    ret = module_get_delay_value(KEY_PHY_IN_1GBps, &module->module_delays[SPEED_1GBps].phy_in);
    if (ret < 0) {
        module->module_delays[SPEED_1GBps].phy_in = schedule_delays[SPEED_1GBps].phy_in;
    }
    ret = module_get_delay_value(KEY_PHY_EG_1GBps, &module->module_delays[SPEED_1GBps].phy_eg);
    if (ret < 0) {
        module->module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;
    }
    ret = module_get_delay_value(KEY_SER_BYPASS_1GBps,
            &module->module_delays[SPEED_1GBps].ser_bypass);
    if (ret < 0) {
        module->module_delays[SPEED_1GBps].ser_bypass = schedule_delays[SPEED_1GBps].ser_bypass;
    }
    ret = module_get_delay_value(KEY_SER_SWITCH_1GBps,
            &module->module_delays[SPEED_1GBps].ser_switch);
    if (ret < 0) {
        module->module_delays[SPEED_1GBps].ser_switch = schedule_delays[SPEED_1GBps].ser_switch;
    }

}

int __must_check module_get_delay_value(const char *config_item, uint32_t *config_value) {
    char config_str_value[12];
    int ret;

    ret = sysfs_get_configfile_item(config_item, config_str_value, sizeof (config_str_value));
    if (ret == 0) {
        uint64_t help;

        help = strtoul(config_str_value, NULL, 10);
        if (help > UINT32_T_MAX) {
            ret = -EINVAL;
            LOGERR("Module: unable to convert value %s of configuration item %s",
                    config_str_value,
                    config_item);
            *config_value = 0;
        } else {
            *config_value = (uint32_t) help;
            ret = 0;
        }
    }
    return ret;
}

uint __must_check calc_nop_schedules_for_long_cycles(struct fsc_command_list *fsc_list) {
    uint num_nops = 0;
    int previous_time = 0;
    struct fsc_command *fsc_item;

    TRACE2_ENTER();
    if (!fsc_list)
        return num_nops;

    ACMLIST_LOCK(fsc_list);
    ACMLIST_FOREACH(fsc_item, fsc_list, entry)
    {
        int delta_time;

        delta_time = fsc_item->hw_schedule_item.abs_cycle - previous_time;
        if (delta_time > UINT16_T_MAX) {
            num_nops = num_nops + delta_time / NOP_DELTA_CYCLE;
        }
        previous_time = fsc_item->hw_schedule_item.abs_cycle;
    }
    ACMLIST_UNLOCK(fsc_list);

    TRACE2_EXIT();
    return num_nops;
}

