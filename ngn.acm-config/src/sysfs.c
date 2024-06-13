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
 * @brief Enable GNU extensions: used for asprintf
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "sysfs.h"
#include "logging.h"
#include "tracing.h"
#include "memory.h"
#include "buffer.h"
#include "status.h"

int __must_check read_buffer_sysfs_item(const char *path_name,
        void *buffer,
        size_t buffer_length,
        off_t offset) {
    int fd, ret;

    TRACE3_ENTER();
    /* open file */
    fd = open(path_name, O_RDONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE3_MSG("Fail");
        return -errno;
    }
    /* read file */
    ret = pread(fd, buffer, buffer_length, offset);
    /* close file */
    close(fd);

    /* check success of read data */
    if (ret < 0) {
        LOGERR("Sysfs: problem reading data %s", path_name);
        TRACE3_MSG("Fail");
        return -errno;
    }
    if (ret != buffer_length) {
        LOGGING_INFO("Sysfs: less data read than expected from file %s. expected %d, read %d",
                path_name,
                buffer_length,
                ret);
    }
    TRACE3_EXIT();
    return 0;
}

STATIC int write_buffer_config_sysfs_item(const char *file_name,
        char *buffer,
        int32_t buffer_length,
        int32_t offset) {

    char path_name[SYSFS_PATH_LENGTH];
    int ret;

    TRACE2_ENTER();
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            file_name);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    ret = write_file_sysfs(path_name, buffer, buffer_length, offset);
    TRACE2_EXIT();
    return ret;
}

void sysfs_delete_file_content(const char *path_name) {
    FILE *file;

    file = fopen(path_name, "w");
    fclose(file);
}

int __must_check write_file_sysfs(const char *path_name,
        void *buffer,
        size_t buffer_length,
        off_t offset) {
    int fd, ret;

    TRACE2_ENTER();
    /* open file */
    fd = open(path_name, O_WRONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        return -errno;
    }
    /* write file */
    ret = pwrite(fd, buffer, buffer_length, offset);
    /* close file */
    close(fd);

    /* check success of write data */
    if (ret < 0) {
        LOGERR("Sysfs: problem writing data %s", path_name);
        TRACE2_MSG("Fail");
        return -errno;
    }
    if (ret != buffer_length) {
        LOGERR("Sysfs: less data written than expected. expected %d, written %d",
                buffer_length,
                ret);
        return -EIO;
    }
    TRACE2_EXIT();
    return 0;
}

int64_t __must_check read_uint64_sysfs_item(const char *path_name) {
    int fd;
    ssize_t read_length;
    int64_t read_data;
    char buffer[80] = { 0 };
    char *conversion_end_ptr;

    TRACE2_ENTER();
    /* open file */
    fd = open(path_name, O_RDONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        return -ENODEV;
    }
    /* read file */
    read_length = read(fd, buffer, sizeof (buffer));
    /* close file */
    close(fd);

    /* check read data */
    if (read_length <= 0) {
        /* no data read */
        LOGERR("Sysfs: read error or no data available at file %s", path_name);
        TRACE2_MSG("Fail");
        return -EACMSYSFSNODATA;
    }

    errno = 0;
    read_data = strtoull(buffer, &conversion_end_ptr, 0);
    if ( (errno != 0) ||
    		((read_data == 0) && (conversion_end_ptr == buffer))) {
        LOGERR("Sysfs: problem converting %s to integer", buffer);
        TRACE2_MSG("Fail");
        return errno != 0 ? -errno : -EINVAL;
    }
    TRACE2_EXIT();
    return read_data;
}

int __must_check create_event_sysfs_items(struct schedule_entry *schedule_item,
        struct acm_module *module,
        int tick_duration,
        uint16_t gather_dma_index,
        uint8_t redundand_index) {
    struct fsc_command *fsc_schedule;
    int num_items, i;
    uint32_t abs_cycle;
    enum acm_linkspeed speed;

    TRACE2_ENTER();
    /* calculate how many items must be  created */
    num_items = module->cycle_ns / schedule_item->period_ns;

    for (i = 0; i < num_items; i++) {
        int64_t help;

        /* calculate values and write them to fsc_schedule item */
        speed = module->speed;
        help = schedule_item->send_time_ns + i * schedule_item->period_ns;
        help = help - (module->module_delays[speed].chip_eg + module->module_delays[speed].phy_eg);
        if (help < 0) {
            /* for-loop has to be executed more often to add items at end
             * of module cycle */
            num_items++;
            continue;
        }

        /* request memory */
        fsc_schedule = acm_zalloc(sizeof (*fsc_schedule));
        if (!fsc_schedule) {
            LOGERR("Sysfs: Out of memory");
            TRACE2_MSG("Fail");
            return -ENOMEM;
        }
        help = (schedule_item->send_time_ns + i * schedule_item->period_ns
                - module->module_delays[speed].chip_eg - module->module_delays[speed].phy_eg);
        abs_cycle = DIV_ROUND_CLOSEST(help, tick_duration);
        fsc_schedule->hw_schedule_item.abs_cycle = abs_cycle;
        fsc_schedule->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(gather_dma_index,
                0,
                redundand_index,
                ACMDRV_SCHED_TBL_TRIG_MODE_STAND_ALONE,
                false,
                false,
                false,
                false);
        /* set reference to schedule item */
        fsc_schedule->schedule_reference = schedule_item;
        /* insert element into sorted list */
        add_fsc_to_module_sorted(&module->fsc_list, fsc_schedule);
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check create_window_sysfs_items(struct schedule_entry *schedule_item,
        struct acm_module *module,
        int tick_duration,
        uint16_t gather_dma_index,
        uint8_t lookup_index,
        bool recovery) {
    struct fsc_command *fsc_schedule;
    int num_items, i;
    uint32_t abs_cycle;
    enum acm_linkspeed speed;

    TRACE2_ENTER();
    /* calculate how many items must be  created */
    num_items = module->cycle_ns / schedule_item->period_ns;

    for (i = 0; i < num_items; i++) {
        int64_t help;

        //create start window item
        /* request memory */
        fsc_schedule = acm_zalloc(sizeof (*fsc_schedule));
        if (!fsc_schedule) {
            LOGERR("Sysfs: Out of memory");
            TRACE2_MSG("Fail");
            return -ENOMEM;
        }
        /* calculate values and write them to fsc_schedule item */
        speed = module->speed;
        switch (module->mode) {
            case CONN_MODE_PARALLEL:
                abs_cycle =
                        (schedule_item->time_start_ns + i * schedule_item->period_ns
                                + module->module_delays[speed].chip_in
                                // automatically rounds down
                                + module->module_delays[speed].phy_in) / tick_duration;
                break;
            case CONN_MODE_SERIAL:
                abs_cycle = (schedule_item->time_start_ns + i * schedule_item->period_ns
                        + module->module_delays[speed].chip_in + module->module_delays[speed].phy_in
                        // automatically rounds down
                        + module->module_delays[speed].ser_switch) / tick_duration;
                break;
            default:
                acm_free(fsc_schedule);
                LOGERR("Sysfs: connection mode has undefined value: %d", module->mode);
                TRACE2_MSG("Fail");
                return -EACMINTERNAL;
        }
        if (abs_cycle >= (module->cycle_ns / tick_duration)) {
            abs_cycle = abs_cycle - (module->cycle_ns / tick_duration);
        }
        fsc_schedule->hw_schedule_item.abs_cycle = abs_cycle;
        fsc_schedule->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(0,
                lookup_index,
                0,
                ACMDRV_SCHED_TBL_TRIG_MODE_NO_TRIG,
                false,
                true,
                false,
                false);
        /* set reference to schedule item */
        fsc_schedule->schedule_reference = schedule_item;
        /* insert element into sorted list */
        add_fsc_to_module_sorted(&module->fsc_list, fsc_schedule);

        // create end window item
        /* request memory */
        fsc_schedule = acm_zalloc(sizeof (*fsc_schedule));
        if (!fsc_schedule) {
            LOGERR("Sysfs: Out of memory");
            TRACE2_MSG("Fail");
            return -ENOMEM;
        }
        /* calculate values and write them to fsc_schedule item */
        speed = module->speed;
        help = schedule_item->time_end_ns + i * schedule_item->period_ns
                + module->module_delays[speed].chip_in + module->module_delays[speed].phy_in;
        abs_cycle = DIV_ROUND_UP(help, tick_duration);
        if (abs_cycle >= (module->cycle_ns / tick_duration)) {
            abs_cycle = abs_cycle - (module->cycle_ns / tick_duration);
        }
        fsc_schedule->hw_schedule_item.abs_cycle = abs_cycle;
        if (recovery) {
            fsc_schedule->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(gather_dma_index,
                    lookup_index,
                    0,
                    ACMDRV_SCHED_TBL_TRIG_MODE_FIRST_STAGE,
                    true,
                    false,
                    false,
                    false);
        } else {
            fsc_schedule->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(0,
                    lookup_index,
                    0,
                    ACMDRV_SCHED_TBL_TRIG_MODE_NO_TRIG,
                    true,
                    false,
                    false,
                    false);
        }
        /* set reference to schedule item */
        fsc_schedule->schedule_reference = schedule_item;
        /* insert element into sorted list */
        add_fsc_to_module_sorted(&module->fsc_list, fsc_schedule);
    }
    TRACE2_EXIT();
    return 0;
}

void add_fsc_to_module_sorted(struct fsc_command_list *fsc_list, struct fsc_command *fsc_schedule) {
    struct fsc_command *fsc_item;
    struct fsc_command *last_item;

    TRACE2_ENTER();
    ACMLIST_LOCK(fsc_list);
    if (ACMLIST_EMPTY(fsc_list)) {
        _ACMLIST_INSERT_TAIL(fsc_list, fsc_schedule, entry);
        ACMLIST_UNLOCK(fsc_list);
        TRACE2_EXIT();
        return;
    }

    last_item = ACMLIST_LAST(fsc_list, fsc_command_list);
    if (fsc_schedule->hw_schedule_item.abs_cycle >= last_item->hw_schedule_item.abs_cycle) {
        _ACMLIST_INSERT_TAIL(fsc_list, fsc_schedule, entry);
        ACMLIST_UNLOCK(fsc_list);
        TRACE2_EXIT();
        return;
    }

    ACMLIST_FOREACH(fsc_item, fsc_list, entry)
    {
        if (fsc_schedule->hw_schedule_item.abs_cycle < fsc_item->hw_schedule_item.abs_cycle) {
            _ACMLIST_INSERT_BEFORE(fsc_list, fsc_item, fsc_schedule, entry);
            break;
        }
    }
    ACMLIST_UNLOCK(fsc_list);
    TRACE2_EXIT();
    return;
}

STATIC int write_fsc_nop_command(int fd,
        uint32_t delta_cycle,
        enum acm_module_id module_index,
        int table_index,
        int item_index) {
    struct acmdrv_sched_tbl_row local_fsc;
    int ret = 0;

    local_fsc.cmd = acmdrv_sched_tbl_cmd_create(0, 0, 0, ACMDRV_SCHED_TBL_TRIG_MODE_NO_TRIG,
    false, false, false, false);
    local_fsc.delta_cycle = delta_cycle;
    local_fsc.padding = 0;
    ret = pwrite(fd,
            &local_fsc,
            sizeof (local_fsc),
            (ACMDRV_SCHED_TBL_ROW_COUNT * module_index * ACMDRV_SCHED_TBL_COUNT +
            ACMDRV_SCHED_TBL_ROW_COUNT * table_index + item_index) * sizeof (local_fsc));
    if (ret < 0) {
        LOGERR("Sysfs: problem writing NOP schedule items");
        ret = -errno;
    }

    return ret;
}

STATIC int update_fsc_indexes(struct fsc_command *fsc_command_item) {
    bool win_close, win_open, ngn_disable, ngn_enable;
    enum acmdrv_sched_tbl_trig_mode trigger;
    struct acm_stream *stream;
    struct schedule_list *schedulelist;
    uint32_t fsc_cmd;

    if (!fsc_command_item->schedule_reference) {
        LOGERR("Sysfs: fsc_schedule without reference to schedule item");
        return -EACMINTERNAL;
    }
    schedulelist = ACMLIST_REF(fsc_command_item->schedule_reference, entry);
    if (!schedulelist) {
        LOGERR("Sysfs: schedule item without reference to schedule list");
        return -EACMINTERNAL;
    }
    stream = schedulelist_to_stream(schedulelist);
    fsc_cmd = fsc_command_item->hw_schedule_item.cmd;
    /*
     * TODO delete: replace call of following RVAL macros by inline function
     * when existing
     */
    win_close = RVAL(ACMDRV_SCHED_TBL_CMD_WIN_CLOSE_BIT, fsc_cmd);
    win_open = RVAL(ACMDRV_SCHED_TBL_CMD_WIN_OPEN_BIT, fsc_cmd);
    ngn_disable = RVAL(ACMDRV_SCHED_TBL_CMD_NGN_DISABLE_BIT, fsc_cmd);
    ngn_enable = RVAL(ACMDRV_SCHED_TBL_CMD_NGN_ENABLE_BIT, fsc_cmd);
    trigger = RVAL(ACMDRV_SCHED_TBL_CMD_DMA_TRIGGER_BIT, fsc_cmd);
    switch (trigger) {
        case ACMDRV_SCHED_TBL_TRIG_MODE_NO_TRIG:
            fsc_command_item->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(0,
                    stream->lookup_index,
                    0,
                    trigger,
                    win_close,
                    win_open,
                    ngn_disable,
                    ngn_enable);
            break;
        case ACMDRV_SCHED_TBL_TRIG_MODE_STAND_ALONE:
            if (stream->type == REDUNDANT_STREAM_TX) {
                fsc_command_item->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(stream
                        ->gather_dma_index,
                        0,
                        stream->redundand_index,
                        trigger,
                        win_close,
                        win_open,
                        ngn_disable,
                        ngn_enable);
            } else {
                fsc_command_item->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(stream
                        ->gather_dma_index,
                        0,
                        0,
                        trigger,
                        win_close,
                        win_open,
                        ngn_disable,
                        ngn_enable);
            }
            break;
        case ACMDRV_SCHED_TBL_TRIG_MODE_FIRST_STAGE:
            if (!stream->reference) {
                LOGERR("Sysfs: Ingress Triggered Stream misses Event Stream");
                return -EACMINTERNAL;
            }
            if (!stream->reference->reference) {
                LOGERR("Sysfs: Event Stream misses Recovery Stream");
                return -EACMINTERNAL;
            }
            fsc_command_item->hw_schedule_item.cmd = acmdrv_sched_tbl_cmd_create(stream->reference
                    ->reference->gather_dma_index,
                    stream->lookup_index,
                    0,
                    trigger,
                    win_close,
                    win_open,
                    ngn_disable,
                    ngn_enable);
            break;
        default:
            LOGERR("Sysfs: Undefined trigger command in fsc schedule item");
            return -EACMINTERNAL;
    }

    return 0;
}

int __must_check write_fsc_schedules_to_HW(struct acm_module *module, int table_index) {
    char path_name[SYSFS_PATH_LENGTH];
    struct fsc_command_list *fsc_list;
    struct fsc_command *fsc_item, *previous_item = NULL;
    struct acmdrv_sched_tbl_row local_fsc;
    int ret, fd, i;
    uint32_t delta_cycle;

    TRACE2_ENTER();
    /* construct path name */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_SCHED_TAB));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // open file
    fd = open(path_name, O_WRONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        return -errno;
    }

    // write data
    fsc_list = &module->fsc_list;
    i = 0;
    ACMLIST_LOCK(fsc_list);
    ACMLIST_FOREACH(fsc_item, fsc_list, entry)
    {
        /* There are 4 scheduling tables of size ACMDRV_SCHED_TBL_ROW_COUNT. The
         * first two are for module 0, the next two are for module 1. The
         * table_index specifies which one of the two for a module has to be
         * used for writing. */

        /* The items in the table contain absolute times, which is necessary to
         * be able to sort the items according to time. But on hardware just the
         * difference to the previous item must be written. So this delta value
         * has to be calculated here before writing */
        if (fsc_item == ACMLIST_FIRST(fsc_list)) {
            delta_cycle = fsc_item->hw_schedule_item.abs_cycle;
            if (delta_cycle != 0) {
                /* first command is a NOP command with waiting time till first real
                 * command. If first cycle time is greater than UINT32_T_MAX
                 * then several NOP commands have to be written */
                while (delta_cycle > UINT16_T_MAX) {
                    ret = write_fsc_nop_command(fd,
                            NOP_DELTA_CYCLE,
                            module->module_id,
                            table_index,
                            i);
                    if (ret < 0) {
                        LOGERR("Sysfs: problem writing to %s ", path_name);
                        goto end;
                    }
                    i++;
                    delta_cycle = delta_cycle - NOP_DELTA_CYCLE;
                }
                /* write NOP command with rest of cycle time */
                ret = write_fsc_nop_command(fd, delta_cycle, module->module_id, table_index, i);
                i++;
                if (ret < 0) {
                    LOGERR("Sysfs: problem writing to %s ", path_name);
                    //ret = -errno;
                    goto end;
                }
            }
            /* remember first command from list for next cycle in while
             * loop */
            previous_item = fsc_item;
        } else {
            /* not first command - write command */
            delta_cycle = fsc_item->hw_schedule_item.abs_cycle
                    - previous_item->hw_schedule_item.abs_cycle;
            /* if delta cycle is longer than maximum value, which can be
             * written to HW, several commands have to be written with time
             * slices of NOP_DELTA_CYCLE and the rest at the end.
             * first the command itself is written and afterwards the
             * necessary number of NOP commands is inserted */
            if (delta_cycle > UINT16_T_MAX) {
                local_fsc.delta_cycle = NOP_DELTA_CYCLE;
            } else {
                local_fsc.delta_cycle = delta_cycle;
            }
            delta_cycle = delta_cycle - local_fsc.delta_cycle;
            ret = update_fsc_indexes(previous_item);
            if (ret < 0) {
                LOGERR("Sysfs: problem updating indexes of fsc schedule item ");
                goto end;
            }
            local_fsc.cmd = previous_item->hw_schedule_item.cmd;
            local_fsc.padding = 0;
            ret = pwrite(fd,
                    (char*) &local_fsc,
                    sizeof (local_fsc),
                    (ACMDRV_SCHED_TBL_ROW_COUNT * module->module_id * ACMDRV_SCHED_TBL_COUNT +
                    ACMDRV_SCHED_TBL_ROW_COUNT * table_index + i) * sizeof (local_fsc));
            if (ret < 0) {
                LOGERR("Sysfs: problem writing to %s ", path_name);
                ret = -errno;
                goto end;
            }
            i++;
            /* write necessary number of NOP commands */
            while (delta_cycle > 0) {
                /* We don't use max value 0xFFFF (65535) but value 60000 to
                 * avoid smaller time distances than min tick at the end of
                 * the period */
                if (delta_cycle > UINT16_T_MAX) {
                    ret = write_fsc_nop_command(fd,
                            NOP_DELTA_CYCLE,
                            module->module_id,
                            table_index,
                            i);
                    delta_cycle = delta_cycle - NOP_DELTA_CYCLE;
                } else {
                    /* last NOP command for this schedule item */
                    ret = write_fsc_nop_command(fd, delta_cycle, module->module_id, table_index, i);
                    delta_cycle = 0;
                }
                if (ret < 0) {
                    LOGERR("Sysfs: problem writing to %s ", path_name);
                    goto end;
                }
                i++;
            }
            /* remember command for next run in while loop */
            previous_item = fsc_item;
        }
    }
    /* write last command with default minimum waiting time of 8 tick;
     * previous_fsc has only a value if list contains items */
    if (previous_item) {
        ret = update_fsc_indexes(previous_item);
        if (ret < 0) {
            LOGERR("Sysfs: problem updating indexes of fsc schedule item ");
            goto end;
        }
        local_fsc.cmd = previous_item->hw_schedule_item.cmd;
        local_fsc.delta_cycle = ANZ_MIN_TICKS;
        ret = pwrite(fd,
                (char*) &local_fsc,
                sizeof (local_fsc),
                (ACMDRV_SCHED_TBL_ROW_COUNT * module->module_id * ACMDRV_SCHED_TBL_COUNT +
                ACMDRV_SCHED_TBL_ROW_COUNT * table_index + i) * sizeof (local_fsc));
        if (ret < 0) {
            LOGERR("Sysfs: problem writing to %s ", path_name);
            ret = -errno;
            goto end;
        }
    }

    end:
    ACMLIST_UNLOCK(fsc_list);
    // close file
    close(fd);
    TRACE2_EXIT();
    return ret;
}

int __must_check write_module_schedules_to_HW(struct acm_module *module, int table_index) {
    char path_name[SYSFS_PATH_LENGTH];
    struct acmdrv_sched_cycle_time cycle_time;
    struct acmdrv_timespec64 start_time;
    int ret, fd;

    TRACE2_ENTER();
    // WRITE MODULE CYCLE TIME
    /* construct path name */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_SCHED_CYCLE));
    if (ret < 0)
        goto out;

    // open file
    fd = open(path_name, O_WRONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        ret = -errno;
        goto out;
    }
    // write cycle time
    cycle_time.ns = module->cycle_ns;
    cycle_time.subns = 0;
    ret = pwrite(fd,
            (char *) &cycle_time,
            sizeof (cycle_time),
            (module->module_id * ACMDRV_SCHED_TBL_COUNT + table_index) * sizeof (cycle_time));

    if (ret < 0) {
        LOGERR("Sysfs: problem writing to %s ", path_name);
        ret = -errno;
        goto out_close;
    }
    // close file
    close(fd);

    // WRITE MODULE START TIME
    /* construct path name */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_SCHED_START));
    if (ret < 0)
        goto out;

    // open file
    fd = open(path_name, O_WRONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        ret = -errno;
        goto out;
    }
    // write schedule start time
    start_time.tv_nsec = module->start.tv_nsec;
    start_time.tv_sec = module->start.tv_sec;
    ret = pwrite(fd,
            (char *) &start_time,
            sizeof (start_time),
            (module->module_id * ACMDRV_SCHED_TBL_COUNT + table_index) * sizeof (start_time));
    if (ret < 0) {
        LOGERR("Sysfs: problem writing to %s ", path_name);
        ret = -errno;
    }

    // close file
out_close:
    close(fd);
out:
    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_read_schedule_status(struct acm_module *module, int *free_table) {
    char path_name[SYSFS_PATH_LENGTH];
    struct acmdrv_sched_tbl_status sched_status[ACMDRV_SCHED_TBL_COUNT];
    int ret, fd, i, can_be_used, status_in_use;

    TRACE2_ENTER();
    /* construct path name */
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_SCHED_STATUS));
    if (ret < 0)
        goto out;

    // open file
    fd = open(path_name, O_RDONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        ret = -errno;
        goto out;
    }
    // read schedule status
    ret = pread(fd,
            (char *) &sched_status,
            sizeof (sched_status),
            sizeof (sched_status) * module->module_id);
    close(fd);

    if (ret < 0) {
        LOGERR("Sysfs: problem reading %s ", path_name);
        ret = -errno;
        goto out;
    }
    ret = -EACMNOFREESCHEDTAB;
    for (i = 0; i < ACMDRV_SCHED_TBL_COUNT; i++) {
        can_be_used = acmdrv_sched_tbl_status_can_be_used_read(&sched_status[i]);
        status_in_use = acmdrv_sched_tbl_status_in_use_read(&sched_status[i]);
        if ( (!can_be_used) && (!status_in_use)) {
            *free_table = i;
            ret = 0;
            break;
        }
    }
    if (ret < 0) {
        LOGERR("Sysfs: no free schedule table found to apply schedule");
    }

out:
    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_construct_path_name(char *path_name,
        size_t length,
        const char *group,
        const char *file) {
    int ret;

    TRACE2_ENTER();
    ret = snprintf(path_name, length, ACMDEV_BASE "%s/%s", group, file);
    if (ret < 0) {
        LOGERR("Sysfs: problem creating pathname to access sysfs");
        TRACE2_MSG("Fail");
        return ret;
    }
    if (ret >= length) {
        LOGERR("Sysfs: pathname of sysfs device too long");
        TRACE2_MSG("Fail");
        return -ENOMEM;
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check sysfs_read_configuration_id(void) {
    char path_name[SYSFS_PATH_LENGTH];
    int32_t read_identifier = 0;
    int ret;

    TRACE2_ENTER();
    ret = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_CONFIG_ID));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // read identifier from current config on HW
    ret = read_buffer_sysfs_item(path_name, &read_identifier, sizeof (read_identifier), 0);
    if (ret == 0) {
        TRACE2_EXIT();
        return read_identifier;
    }
    TRACE2_MSG("Fail");
    return ret;
}

int __must_check sysfs_write_configuration_id(int32_t identifier) {

    TRACE2_MSG("Executing");
    // write identifier of config to HW
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_CONFIG_ID),
            (char*) &identifier,
            sizeof(int32_t),
            0);
}

int __must_check sysfs_write_emergency_disable(struct acm_module *module,
        struct acmdrv_sched_emerg_disable *value) {

    TRACE2_MSG("Executing");
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_EMERGENCY),
            (char*) value,
            sizeof(struct acmdrv_sched_emerg_disable),
            module->module_id * sizeof(struct acmdrv_sched_emerg_disable));
}

int __must_check sysfs_write_connection_mode_to_HW(struct acm_module *module) {
    enum acmdrv_conn_mode mode;
    uint32_t mode_write_value;

    TRACE2_ENTER();
    switch (module->mode) {
        case CONN_MODE_SERIAL:
            mode = ACMDRV_BYPASS_CONN_MODE_SERIES;
            break;
        case CONN_MODE_PARALLEL:
            mode = ACMDRV_BYPASS_CONN_MODE_PARALLEL;
            break;
        default:
            LOGERR("Sysfs: module mode has undefined value");
            TRACE2_MSG("Fail");
            return -EINVAL;
    }
    mode_write_value = acmdrv_bypass_conn_mode_create(mode);
    TRACE2_EXIT();
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_CONN_MODE),
            (char*) &mode_write_value,
            sizeof (mode_write_value),
            module->module_id * sizeof (mode_write_value));
}

int __must_check sysfs_write_config_status_to_HW(enum acmdrv_status status) {

    TRACE2_MSG("Executing");
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_CONFIG_STATE),
            (char*) &status,
            sizeof (status),
            0);
}

int __must_check sysfs_write_module_enable(struct acm_module *module, bool enable) {
    uint32_t enable_value;

    TRACE2_MSG("Executing");
    enable_value = acmdrv_bypass_ctrl_enable_create(enable);
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_MODULE_ENABLE),
            (char*) &enable_value,
            sizeof (enable_value),
            module->module_id * sizeof (enable_value));
}

int __must_check sysfs_write_data_constant_buffer(struct acm_module *module) {
    struct acmdrv_bypass_const_buffer constant_buffer;
    struct acm_stream *stream;
    struct operation *operation;
    unsigned int offset = 0;

    TRACE2_ENTER();
    memset(&constant_buffer.data[offset], 0, sizeof (constant_buffer.data));
    // iterate through all streams of the module
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, &module->streams, entry)
    {
        struct operation_list *oplist = & (stream->operations);

        ACMLIST_LOCK(oplist);
        ACMLIST_FOREACH(operation, oplist, entry)
        {
            if (operation->opcode == INSERT_CONSTANT) {
                /* in case of operation insert_constant: write data from operation to
                 * constant Buffer and write offset in constant buffer to operation */
                memcpy(&constant_buffer.data[offset], operation->data, operation->data_size);
                operation->const_buff_offset = offset;
                offset = offset + operation->data_size;
            }
        }
        ACMLIST_UNLOCK(oplist);
    }
    ACMLIST_UNLOCK(&module->streams);

    TRACE2_EXIT();
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_CONST_BUFFER),
            (char*) &constant_buffer,
            sizeof (constant_buffer),
            module->module_id * sizeof (constant_buffer));
}

int __must_check sysfs_write_lookup_tables(struct acm_module *module) {
    struct acm_stream *stream;
    struct acmdrv_bypass_stream_trigger stream_trigger;
    struct acmdrv_bypass_layer7_check mask, pattern;
    struct acmdrv_bypass_lookup header_mask, header_pattern;
    int ret;
    uint16_t ingress_control, lookup_enable, layer7_enable;
    uint8_t layer7_len = 0;

    ingress_control = 0;
    lookup_enable = 0;
    layer7_enable = 0;

    TRACE2_ENTER();
    /* Iterate through all streams of the module and write lookup date for
     * Ingress Triggered Streams.*/
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, &module->streams, entry)
    {
        if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == REDUNDANT_STREAM_RX)) {
            // write layer 7 mask
            memset(mask.data, 0, sizeof (mask.data));
            memcpy(mask.data, stream->lookup->filter_mask, stream->lookup->filter_size);
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LAYER7_MASK),
                    (char*) &mask,
                    sizeof (mask),
                    module->module_id * sizeof (mask) * ACM_MAX_LOOKUP_ITEMS
                            + stream->lookup_index * sizeof (mask));
            if (ret != 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
            // write layer 7 pattern
            memset(pattern.data, 0, sizeof (pattern.data));
            memcpy(pattern.data, stream->lookup->filter_pattern, stream->lookup->filter_size);
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LAYER7_PATTERN),
                    (char*) &pattern,
                    sizeof (pattern),
                    module->module_id * sizeof (pattern) * ACM_MAX_LOOKUP_ITEMS
                            + stream->lookup_index * sizeof (pattern));
            if (ret != 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
            // write header mask (lookup mask)
            memcpy(&header_mask, stream->lookup->header_mask, sizeof (header_mask));
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LOOKUP_MASK),
                    (char*) &header_mask,
                    sizeof (header_mask),
                    module->module_id * sizeof (header_mask) * ACM_MAX_LOOKUP_ITEMS
                            + stream->lookup_index * sizeof (header_mask));
            if (ret != 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
            // write header pattern (lookup pattern)
            memcpy(&header_pattern, stream->lookup->header, sizeof (header_pattern));
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LOOKUP_PATTERN),
                    (char*) &header_pattern,
                    sizeof (header_pattern),
                    module->module_id * sizeof (header_pattern) * ACM_MAX_LOOKUP_ITEMS
                            + stream->lookup_index * sizeof (header_pattern));
            if (ret != 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
            // write lookup trigger
            uint16_t tmp_gather_index;
            if (stream->reference) {
                /* Ingress triggered stream has an Event stream therefore we use
                 * the gather_dma_index from the event stream */
                tmp_gather_index = stream->reference->gather_dma_index;
            } else {
                tmp_gather_index = stream->gather_dma_index;
            }
            stream_trigger.trigger = acmdrv_bypass_stream_trigger_create(true,
            false, tmp_gather_index, stream->scatter_dma_index, stream->redundand_index);
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_STREAM_TRIGGER),
                    (char*) &stream_trigger,
                    sizeof (stream_trigger),
                    module->module_id * sizeof (stream_trigger) * MAX_LOOKUP_TRIGGER_ITEMS
                            + stream->lookup_index * sizeof (stream_trigger));
            if (ret != 0) {
                TRACE2_MSG("Fail");
                return ret;
            }
            // prepare bit masks for Control table
            lookup_enable = lookup_enable | (1 << stream->lookup_index);
            if (stream->lookup->filter_size > layer7_len) {
                layer7_len = stream->lookup->filter_size;
            }
            if (stream->lookup->filter_size > 0) {
                layer7_enable = layer7_enable | (1 << stream->lookup_index);
            }
            if ( (stream_has_operation_x(stream, READ))
                    || ( (stream->reference != NULL)
                            && (stream_has_operation_x(stream->reference, INSERT)))) {
                /* bit is set only if Ingress Triggered Stream has no READ
                 * operations and not event stream */
                ingress_control = ingress_control | (1 << stream->lookup_index);
            }
        }
    }
    ACMLIST_UNLOCK(&module->streams);
    // write lookup control table items
    ret = sysfs_write_lookup_control_block(module->module_id,
            ingress_control,
            lookup_enable,
            layer7_enable,
            layer7_len);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write rule 16 (17th element) for lookup trigger table
    if (module->mode == CONN_MODE_SERIAL) {
        stream_trigger.trigger = acmdrv_bypass_stream_trigger_create(true,
        false, GATHER_FORWARD_IDX, SCATTER_NOP_IDX, 0);
    } else {
        // connection mode is CONN_MODE_PARALLEL
        stream_trigger.trigger = acmdrv_bypass_stream_trigger_create(true,
        true, GATHER_NOP_IDX, SCATTER_NOP_IDX, 0);
    }
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_STREAM_TRIGGER),
            (char*) &stream_trigger,
            sizeof (stream_trigger),
            module->module_id * sizeof (stream_trigger) * MAX_LOOKUP_TRIGGER_ITEMS +
            ACM_MAX_LOOKUP_ITEMS * sizeof (stream_trigger));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check sysfs_write_scatter_dma(struct acm_module *module) {
    struct acm_stream *stream;
    int ret, scatter_index = 0;
    struct acmdrv_bypass_dma_command scatter_command;
    struct operation_list *oplist;
    struct operation *operation, *next_op;
    bool last_item;

    TRACE2_ENTER();
    /* first table entry is NOP, used by all streams which have no
     * operation of this type
     * command length 0 indicates NOP */
    scatter_command.cmd = acmdrv_bypass_dma_cmd_s_move_with_timestamp_create(true,
    false, 0, 0, 0);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_SCATTER),
            (char*) &scatter_command,
            sizeof (scatter_command),
            module->module_id * sizeof (scatter_command) * ACM_MAX_INGRESS_OPERATIONS
                    + scatter_index * sizeof (scatter_command));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // iterate all streams
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, &module->streams, entry)
    {
        if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == REDUNDANT_STREAM_RX)) {
            scatter_index = stream->scatter_dma_index;

            // for Ingress Triggered Stream iterate all operations
            oplist = & (stream->operations);
            ACMLIST_LOCK(oplist);
            ACMLIST_FOREACH(operation, oplist, entry)
            {
                if (operation->opcode == READ) {
                    /* for each read operation create a Scatter DMA item; to set the "last"-flag
                     * correctly it has to be checked if the stream has a further read operation
                     * all read operations are with timestamp */
                    next_op = ACMLIST_NEXT(operation, entry);
                    while ( (next_op != NULL) && (next_op->opcode != READ)) {
                        next_op = ACMLIST_NEXT(next_op, entry);
                    }
                    if (next_op == NULL) {
                        last_item = true;
                    } else {
                        /* another read operation was found */
                        last_item = false;
                    }
                    scatter_command.cmd =
                            acmdrv_bypass_dma_cmd_s_move_with_timestamp_create(last_item,
                                    false,
                                    operation->offset,
                                    operation->length,
                                    operation->msg_buf->msg_buff_index);
                    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_SCATTER),
                            (char*) &scatter_command,
                            sizeof (scatter_command),
                            module->module_id * sizeof (scatter_command)
                                    * ACM_MAX_INGRESS_OPERATIONS
                                    + scatter_index * sizeof (scatter_command));
                    if (ret != 0) {
                        goto error_exit;
                    }
                    scatter_index++;
                }
            }
error_exit:
            ACMLIST_UNLOCK(oplist);
        }
    }
    ACMLIST_UNLOCK(&module->streams);
    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_write_prefetcher_gather_dma(struct acm_module *module) {
    struct acm_stream *stream;
    int ret, gather_index = 0;
    struct acmdrv_bypass_dma_command gather_command, prefetch_command;

    TRACE2_ENTER();
    /* first table entry is NOP, used by all streams which have no
     * operation of this type
     * NOP command must be written to gather dma table and to prefetcher table */
    gather_command.cmd = acmdrv_bypass_dma_cmd_g_move_fr_buff_create(true, 0, 0);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_GATHER),
            (char*) &gather_command,
            sizeof (gather_command),
            module->module_id * sizeof (gather_command) * ACM_MAX_EGRESS_OPERATIONS
                    + gather_index * sizeof (gather_command));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    prefetch_command.cmd = acmdrv_bypass_dma_cmd_p_nop_create();
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_PREFETCH),
            (char*) &prefetch_command,
            sizeof (prefetch_command),
            module->module_id * sizeof (prefetch_command) * ACM_MAX_EGRESS_OPERATIONS
                    + gather_index * sizeof (prefetch_command));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    /* for lookup rule 16 (17) a forward all command must be written to
     * second position (GATHER_FORWARD_IDX)
     * In principle only needed for connection mode serial, but done generally
     * to keep things more simple. */
    gather_index++;
    gather_command.cmd = acmdrv_bypass_dma_cmd_g_forward_create();
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_GATHER),
            (char*) &gather_command,
            sizeof (gather_command),
            module->module_id * sizeof (gather_command) * ACM_MAX_EGRESS_OPERATIONS
                    + gather_index * sizeof (gather_command));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    /*write NOP to prefetch table in parallel to gather table */
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_PREFETCH),
            (char*) &prefetch_command,
            sizeof (prefetch_command),
            module->module_id * sizeof (prefetch_command) * ACM_MAX_EGRESS_OPERATIONS
                    + gather_index * sizeof (prefetch_command));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // iterate all streams
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, &module->streams, entry)
    {
        gather_index = stream->gather_dma_index;
        if (stream->type == INGRESS_TRIGGERED_STREAM) {
            /* iterate operations list for Forward all commands*/
            ret = write_gather_ingress(gather_index, module->module_id, stream);
            if (ret != 0) {
                LOGERR("Failed to write gather ingress");
                goto error_exit;
            }
        } else if (stream->type != REDUNDANT_STREAM_RX) {
            /* stream type: TIME_TRIGGERED_STREAM, EVENT_STREAM,
             * RECOVERY_STREAM, REDUNDANT_STREAM_TX;
             * can have different operations: insert, insert const,
             * pad, forward (only event stream)
             * iterate operations list for the commands
             * (REDUNDANT_STREAM_RX can have only scatter command READ)*/
            ret = write_gather_egress(gather_index, module->module_id, stream);
            if (ret != 0) {
                LOGERR("Failed to write gather engress");
                goto error_exit;
            }
        }
    }
error_exit:
    ACMLIST_UNLOCK(&module->streams);
    TRACE2_EXIT();
    return ret;
}

int __must_check write_gather_ingress(int gather_index,
        uint32_t module_id,
        struct acm_stream *stream) {
    int ret = 0;
    struct acmdrv_bypass_dma_command gather_command;
    struct operation_list *oplist;
    struct operation *operation;

    TRACE2_ENTER();
    oplist = & (stream->operations);
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        /* We have an Ingress Triggerd Stream, which has only READ and
         * FORWARD_ALL operations. READ operations are written to Scatter DMA
         * table. Here we only have to handle FORWARD_ALL operations for
         * Gather DMA table*/
        if (operation->opcode == FORWARD_ALL) {
            /* A stream can have only one forward_all operation! If we have
             * found one, the iteration through the operation list can be
             * stopped. */
            gather_command.cmd = acmdrv_bypass_dma_cmd_g_forward_create();
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_GATHER),
                    (char*) &gather_command,
                    sizeof (gather_command),
                    module_id * sizeof (gather_command) * ACM_MAX_EGRESS_OPERATIONS
                            + gather_index * sizeof (gather_command));
            /*	if (ret == 0) {
             (*gather_index)++;
             }*/
            goto exit;
        }
    }
    exit:
    ACMLIST_UNLOCK(oplist);
    TRACE2_EXIT();
    return ret;
}

int __must_check write_gather_egress(int start_index, uint32_t module_id, struct acm_stream *stream) {
    int ret, gather_index, prefetch_index;
    struct acmdrv_bypass_dma_command gather_command, prefetch_command;
    struct operation_list *oplist;
    struct operation *operation;
    struct operation *last_insert_op = NULL;
    bool last_item, last_prefetch = false;
    struct acmdrv_msgbuf_lock_ctrl lock_vector;

    TRACE2_ENTER();
    ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&lock_vector);
    gather_index = start_index;
    /*first positions in prefetch table for lock vector - therefore increase
     * prefetch_index */
    prefetch_index = gather_index;
    ret = 0;
    oplist = &(stream->operations);
    ACMLIST_LOCK(oplist);
    /* find last insert operation */
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if (operation->opcode == INSERT) {
            last_insert_op = operation;
            /* prepare lock vector for writing prefetch lock commands after
             * handling of all operations */
            ACMDRV_MSGBUF_LOCK_CTRL_SET(operation->msg_buf->msg_buff_index, &lock_vector);
        }
    }

    if (ACMDRV_MSGBUF_LOCK_CTRL_COUNT(&lock_vector) != 0) {
        /* write prefetch lock commands */
        int i;
        bool dual_lock = false;

        if (stream->type == REDUNDANT_STREAM_TX) {
            dual_lock = true;
        }
        // write bits 0 - 15 of lock vector
        /* 4 prefetch lock commands are written. Each command contains 16 bits
         * of the lock vector which is 64 bits long. */
        for (i = 0; i < NUM_PREFETCH_LOCK_COMMANDS; i++) {
            struct acmdrv_msgbuf_lock_ctrl mask;
            uint16_t data;

            ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(&mask, (i + 1) * 16 - 1, i * 16);
            data = ACMDRV_MSGBUF_LOCK_CTRL_FIELD_GET(&mask, &lock_vector);
            if (data == 0)
                continue;

            prefetch_command.cmd = acmdrv_bypass_dma_cmd_p_lock_msg_buff_create(i, dual_lock, data);
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_PREFETCH),
                    (char*) &prefetch_command, sizeof (prefetch_command),
                    module_id * sizeof (prefetch_command) * ACM_MAX_EGRESS_OPERATIONS
                            + prefetch_index * sizeof (prefetch_command));
            if (ret != 0)
                break;

            prefetch_index++;
        }
    } else {
        /* write a NOP command to prefetch table */
        prefetch_command.cmd = acmdrv_bypass_dma_cmd_p_nop_create();
        ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_PREFETCH),
                (char*) &prefetch_command,
                sizeof (prefetch_command),
                module_id * sizeof (prefetch_command) * ACM_MAX_EGRESS_OPERATIONS
                        + prefetch_index * sizeof (prefetch_command));
    }
    if (ret != 0)
        goto error_exit;

    ACMLIST_FOREACH(operation, oplist, entry)
    {
        /* We have an Egress  Stream, which has at least 3 automatic operations:
         * 2 insert constant operations and one forward operation
         * insert operation need a special handling: for them prefetch commands
         * have to be written too. */
        if (ACMLIST_NEXT(operation, entry) == NULL) {
            last_item = true;
            if ( (stream->type == REDUNDANT_STREAM_TX)
                    && ( (gather_index - start_index) == (NUM_AUTOGEN_OPS - 1))) {
                /* The last automatic operation is handled and there are no
                 * other following. As we have to add an R-Tag command, we can't
                 * set the last flag now, but have to set it in the R-Tag command.*/
                last_item = false;
            }
        } else {
            /* another operation follows */
            last_item = false;
        }

        ret = 0;
        switch (operation->opcode) {
            case INSERT_CONSTANT:
                gather_command.cmd = acmdrv_bypass_dma_cmd_g_move_cnst_buff_create(last_item,
                        operation->length,
                        operation->const_buff_offset);
                break;
            case PAD:
                gather_command.cmd = acmdrv_bypass_dma_cmd_g_const_byte_create(last_item,
                        operation->length,
                        (uint8_t) *operation->data);
                break;
            case FORWARD:
                gather_command.cmd = acmdrv_bypass_dma_cmd_g_move_fr_buff_create(last_item,
                        operation->length,
                        operation->offset);
                break;
            case INSERT:
                gather_command.cmd = acmdrv_bypass_dma_cmd_g_move_pref_create(last_item);
                /* in case of insert operation a prefetch command has to be
                 * written too */
                if (operation == last_insert_op) {
                    last_prefetch = true;
                }
                prefetch_command.cmd = acmdrv_bypass_dma_cmd_p_mov_msg_buff_create(last_prefetch,
                        0,
                        operation->length,
                        operation->msg_buf->msg_buff_index);
                ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_PREFETCH),
                        (char*) &prefetch_command,
                        sizeof (prefetch_command),
                        module_id * sizeof (prefetch_command) * ACM_MAX_EGRESS_OPERATIONS
                                + prefetch_index * sizeof (prefetch_command));
                prefetch_index++;
                break;
            default:
                ret = -EACMINTERNAL;
                LOGERR("Sysfs: egress Stream with operation other than INSERT, INSERT_CONSTANT, PAD, FORWARD");
                break;
        }
        if (ret != 0)
            break;
        ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_GATHER),
                (char*) &gather_command,
                sizeof (gather_command),
                module_id * sizeof (gather_command) * ACM_MAX_EGRESS_OPERATIONS
                        + (gather_index) * sizeof (gather_command));
        if (ret != 0)
            break;

        gather_index++;
        /*if stream is a redundant stream, an R-Tag gather command must
         * be written after the 3 header commands */
        if ( (stream->type == REDUNDANT_STREAM_TX)
                && ( (gather_index - start_index) == NUM_AUTOGEN_OPS)) {
            gather_command.cmd = acmdrv_bypass_dma_cmd_g_r_tag_create();
            /* There is no possibility to write the last flag in R-Tag command.
             * It is expected that there will follow additional commands after
             * the three automatic operations, because otherwise the requirement
             * that stream length is greater or equal 64 bytes wouldn't be
             * fulfilled. */
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_GATHER),
                    (char*) &gather_command,
                    sizeof (gather_command),
                    module_id * sizeof (gather_command) * ACM_MAX_EGRESS_OPERATIONS
                            + gather_index * sizeof (gather_command));
            if (ret != 0)
                break;
            gather_index++;
        }
    }

error_exit:
    ACMLIST_UNLOCK(oplist);
    if (ret != 0)
        TRACE2_MSG("Fail");
    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_write_redund_ctrl_table(struct acm_module *module) {
    struct acm_stream *stream;
    int ret, redund_index = 0;
    struct acmdrv_redun_ctrl_entry redund_entry;

    TRACE2_ENTER();
    /* first table entry is NOP, used for instance by all schedule items for
     * streams which are no redundant streams
     * parameter drop_no_rtag only for receive case, thus false*/
    redund_entry.ctrl = acmdrv_redun_ctrltab_entry_create(ACMDRV_REDUN_CTLTAB_SRC_INTSEQNUM,
            ACMDRV_REDUN_CTLTAB_UPD_NOP,
            false,
            0);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_REDUND_CONTR),
            (char*) &redund_entry,
            sizeof (redund_entry),
            module->module_id * sizeof (redund_entry) * ACM_MAX_REDUNDANT_STREAMS
                    + redund_index * sizeof (redund_entry));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // iterate all streams
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, &module->streams, entry)
    {
        if ( (stream->type == REDUNDANT_STREAM_TX) || (stream->type == REDUNDANT_STREAM_RX)) {

            /* create redundancy entry and write it to HW
             * parameter recovery depricated, thus false */
            if (stream->type == REDUNDANT_STREAM_TX) {
                /* parameter drop_no_rtag only for receive case, thus false*/
                redund_entry.ctrl =
                        acmdrv_redun_ctrltab_entry_create(ACMDRV_REDUN_CTLTAB_SRC_INTSEQNUM,
                                ACMDRV_REDUN_CTLTAB_UPD_FIN_BOTH,
                                false,
                                stream->redundand_index);
            } else {
                /* parameter drop_no_rtag only for receive case, thus true*/
                redund_entry.ctrl =
                        acmdrv_redun_ctrltab_entry_create(ACMDRV_REDUN_CTLTAB_SRC_RXSEQNUM,
                                ACMDRV_REDUN_CTLTAB_UPD_MAXNUM,
                                true,
                                stream->redundand_index);
            }
            ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_REDUND_CONTR),
                    (char*) &redund_entry,
                    sizeof (redund_entry),
                    module->module_id * sizeof (redund_entry) * ACM_MAX_REDUNDANT_STREAMS
                            + stream->redundand_index * sizeof (redund_entry));
            if (ret != 0) {
                goto error_exit;
            }
        }
    }
error_exit:
    ACMLIST_UNLOCK(&module->streams);
    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_write_lookup_control_block(uint32_t module_id,
        uint16_t ingress_control,
        uint16_t lookup_enable,
        uint16_t layer7_enable,
        uint8_t layer7_len) {
    /* cntl_connection_mode - written in function sysfs_write_connection_mode_to_HW
     * function called in write_module_data_to_HW before writing delays */
    // cntl_ngn_enable - written in function sysfs_write_module_enable
    // cntl_gather_delay - only used driver internally
    // cntl_output_disable - not used
    int ret;
    uint32_t control_val;

    TRACE2_ENTER();
    // write  ACM_SYSFS_INGRESS_CONTROL cntl_ingress_policing_control
    control_val = acmdrv_bypass_ingress_policing_control_create(ingress_control);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_INGRESS_CONTROL),
            (char*) &control_val,
            sizeof (control_val),
            module_id * sizeof (control_val));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    /* write ACM_SYSFS_INGRESS_ENABLE cntl_ingress_policing_enable - same
     *   as lookup enable! */
    control_val = acmdrv_bypass_ingress_policing_enable_create(lookup_enable);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_INGRESS_ENABLE),
            (char*) &control_val,
            sizeof (control_val),
            module_id * sizeof (control_val));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write ACM_SYSFS_LAYER7_ENABLE cntl_layer7_enable
    control_val = acmdrv_bypass_layer7_enable_create(layer7_enable);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LAYER7_ENABLE),
            (char*) &control_val,
            sizeof (control_val),
            module_id * sizeof (control_val));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write ACM_SYSFS_LAYER7_LENGTH cntl_layer7_length
    control_val = acmdrv_bypass_layer7_length_create(layer7_len);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LAYER7_LENGTH),
            (char*) &control_val,
            sizeof (control_val),
            module_id * sizeof (control_val));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    // write ACM_SYSFS_LOOKUP_ENABLE cntl_lookup_enable
    control_val = acmdrv_bypass_lookup_enable_create(lookup_enable);
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_LOOKUP_ENABLE),
            (char*) &control_val,
            sizeof (control_val),
            module_id * sizeof (control_val));
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check sysfs_write_cntl_speed(struct acm_module *module) {
    // write cntl_speed
    enum acmdrv_bypass_speed_select speed;
    uint32_t speed_value;
    int ret;

    TRACE2_ENTER();
    switch (module->speed) {
        case SPEED_100MBps:
            speed = ACMDRV_BYPASS_SPEED_SELECT_100;
            break;
        case SPEED_1GBps:
            speed = ACMDRV_BYPASS_SPEED_SELECT_1000;
            break;
        default:
            ret = -EACMINTERNAL;
            LOGERR("Sysfs: speed: %d out of range", module->speed);
            TRACE2_MSG("Fail");
            return ret;
    }
    speed_value = acmdrv_bypass_speed_create(speed);
    TRACE2_EXIT();
    return write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_SPEED),
            (char*) &speed_value,
            sizeof (speed_value),
            module->module_id * sizeof (speed_value));
}

int __must_check check_buff_name_against_sys_devices(const char *buffer_name) {
    //char path_name[SYSFS_PATH_LENGTH] = MSGBUF_BASE;
    int ret = 0;
    char praefix[PRAEFIX_LENGTH];

    TRACE2_ENTER();
    /* check if configured filename präfix is part of the buffer_name */
    // read präfix from configuration file
    ret = sysfs_get_configfile_item(KEY_PRAEFIX, praefix, sizeof (praefix));
    if (ret < 0) {
        /* no configuration found - take default value */
        strcpy(praefix, DEFAULT_PRAEFIX);
    }
    /* check if buffer_name begins with praefix */
    ret = strncmp(praefix, buffer_name, strlen(praefix));
    if (ret != 0) {
        LOGERR("Sysfs: message buffer name %s doesn't start with configured/default praefix %s",
                buffer_name,
                praefix);
        TRACE2_MSG("Fail");
        return -EPERM;
    }

    /* read content of directory where message buffer files are stored and
     * check if actual name is already used */
    /*	TODO: discuss if check below shall be done after clear-fpga-all
     DIR *sys_devices;
     struct dirent *file;
     sys_devices = opendir(MSGBUF_BASE);
     if (sys_devices == NULL) {
     ret = -errno;
     LOGERR("Sysfs: can't open directory %s", MSGBUF_BASE);
     TRACE2_MSG("Fail");
     return ret;
     }
     while ((file = readdir(sys_devices)) != NULL) {
     if (strcmp(file->d_name, buffer_name) == 0) {
     // strings are equal ;
     ret = -EPERM;
     LOGERR("Sysfs: file with buffername already exists: %s", file->d_name);
     goto exit;
     }
     }
exit:
     closedir(sys_devices);
     */
    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_write_msg_buff_to_HW(struct buffer_list *bufferlist,
        enum buff_table_type buff_table) {
    char path_name[SYSFS_PATH_LENGTH];
    struct sysfs_buffer *buffer;
    uint32_t descriptor;
    struct acmdrv_buff_alias alias; // more complicated structure including a char array
    int ret, fd, item_size;
    char *hw_item;

    TRACE2_ENTER();
    /* construct path name */
    if (buff_table == BUFF_DESC) {
        ret = sysfs_construct_path_name(path_name,
                SYSFS_PATH_LENGTH,
                __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
                __stringify(ACM_SYSFS_MSGBUFF_DESC));
    } else {
        /* buff_table equal BUFF_ALIAS */
        ret = sysfs_construct_path_name(path_name,
                SYSFS_PATH_LENGTH,
                __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
                __stringify(ACM_SYSFS_MSGBUFF_ALIAS));
    }
    if (ret < 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    // open file
    fd = open(path_name, O_WRONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", path_name);
        TRACE2_MSG("Fail");
        return -errno;
    }

    // write data
    ACMLIST_LOCK(bufferlist);
    ACMLIST_FOREACH(buffer, bufferlist, entry)
    {
        if (buff_table == BUFF_DESC) {
            descriptor = acmdrv_buff_desc_create(buffer->msg_buff_offset,
                    buffer->reset,
                    buffer->stream_direction,
                    buffer->buff_size,
                    buffer->timestamp,
                    buffer->valid);
            hw_item = (char*) &descriptor;
            item_size = sizeof (descriptor);
        } else {
            /* buff_table equal BUFF_ALIAS */
            memset(&alias, 0, sizeof (alias));
            ret = acmdrv_buff_alias_init(&alias, buffer->msg_buff_index, buffer->msg_buff_name);
            if (ret < 0) {
                LOGERR("Sysfs: problem creating msg buffer alias %s ", buffer->msg_buff_name);
                break;
            }
            hw_item = (char*) &alias;
            item_size = sizeof (alias);
        }
        ret = pwrite(fd, hw_item, item_size, buffer->msg_buff_index * item_size);
        if (ret < 0) {
            LOGERR("Sysfs: problem writing to %s ", path_name);
            ret = -errno;
            break;
        }
        /* everything done okay, set ret to zero */
        ret = 0;
    }

    ACMLIST_UNLOCK(bufferlist);
    // close file
    close(fd);

    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_write_buffer_control_mask(uint64_t vector, const char* filename) {
    char path_name[SYSFS_PATH_LENGTH];
    int ret;
    int anz_msg_buff;
    struct acmdrv_msgbuf_lock_ctrl mask;
    int i;

    TRACE2_ENTER();

    /* read number of message buffers */
    anz_msg_buff = get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_COUNT));
    if (anz_msg_buff <= 0) {
        LOGERR("Sysfs: invalid number of message buffers: %d", anz_msg_buff);
        return -EACMNUMMESSBUFF;
    }

    /* ATTENTION: this is for 64bit vectors only! */
    if (vector > GENMASK_ULL(anz_msg_buff - 1, 0)) {
        LOGERR("Sysfs: too many message buffers used. Only %d are available", anz_msg_buff);
        return -EACMNUMMESSBUFF;
    }

    ret = sysfs_construct_path_name(path_name, SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONTROL_GROUP), filename);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* convert to driver's bit array */
    ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&mask);
    for (i = 0; i < anz_msg_buff; ++i)
        if (vector & (1ULL << i))
            ACMDRV_MSGBUF_LOCK_CTRL_SET(i, &mask);

    ret = write_file_sysfs(path_name, &mask, sizeof(mask), 0);
    TRACE2_EXIT();
    return ret;
}

int get_mac_address(char *ifname, uint8_t *mac) {
    int ret = 0;
    int fd;
    struct ifreq ifr;

    TRACE3_ENTER();
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        TRACE3_MSG("Fail");
        return -errno;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ret = ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (ret == -1) {
        ret = -errno;
        goto out;
    }

    memcpy(mac, (uint8_t*) ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
out: close(fd);
    TRACE3_EXIT();
    return ret;
}

int __must_check sysfs_get_configfile_item(const char *config_item,
        char *config_value,
        int value_length) {
    int fd, ret, end_pos, offset = 0;
    char buffer[100];
    char *string_ptr;
    char config_value_end[] = " \n,\t";
    char nextline[] = "\n";

    TRACE2_ENTER();
    // open file
    fd = open(CONFIG_FILE, O_RDONLY | O_DSYNC);
    if (fd < 0) {
        LOGERR("Sysfs: open file %s failed", CONFIG_FILE);
        TRACE2_MSG("Fail");
        return -errno;
    }

    // read file
    ret = read(fd, buffer, sizeof (buffer));
    while (ret > 0) {
        if (strncmp(config_item, buffer, strlen(config_item)) == 0) {
            /* keyword found */
            end_pos = strcspn(buffer, " \t");
            /* move to end of keyword */
            string_ptr = buffer + end_pos + 1;
            /* skip blanks an tabs */
            string_ptr = strtok(string_ptr, " \t");
            /* find end of configuration value */
            end_pos = strcspn(string_ptr, config_value_end);
            if (end_pos >= value_length) {
                /* provided space for configuration value isn't long enough */
                LOGERR("Sysfs: configuration value %s has %d characters, but only %d supported",
                        config_item,
                        end_pos,
                        value_length - 1);
                ret = -EACMCONFIGVAL;
                goto end;
            }
            /* copy configuration value to provided space and terminat the string */
            strncpy(config_value, string_ptr, end_pos);
            string_ptr = config_value + end_pos;
            *string_ptr = '\0';
            ret = 0;
            goto end;
        } else {
            /* position read pointer after next newline */
            end_pos = strcspn(buffer, nextline);
            end_pos++;
            offset = offset + end_pos;
        }
        ret = pread(fd, buffer, sizeof (buffer), offset);
    }
    /* configuration item  not found */
    ret = -EACMCONFIG;
    LOGGING_INFO("Sysfs: configuration item not found %s", config_item);
    *config_value = '\0';

end:
    // close file
    close(fd);

    TRACE2_EXIT();
    return ret;
}

uint32_t __must_check sysfs_get_recovery_timeout(void) {
    char config_str_value[12] = { 0 };
    int ret;
    uint32_t rec_timeout = DEFAULT_REC_TIMEOUT_MS;

    ret = sysfs_get_configfile_item(KEY_RECOVERY_TIMEOUT_MS,
            config_str_value,
            sizeof (config_str_value));
    if (ret == 0) {
        uint64_t help;

        help = strtoul(config_str_value, NULL, 10);
        if (help > UINT32_T_MAX) {
            LOGERR("Module: unable to convert value %s of configuration item KEY_RECOVERY_TIMEOUT",
                    config_str_value);
        } else {
            rec_timeout = (uint32_t) help;
        }
    }
    return rec_timeout;
}

int __must_check sysfs_write_base_recovery(struct acm_config *config) {
    struct acmdrv_redun_base_recovery base_recovery_array;
    memset(&base_recovery_array, 0, sizeof (base_recovery_array));
    struct acm_stream *stream;
    uint32_t timeout;
    int ret = 0;

    if ( (config->bypass[0] == NULL) || (config->bypass[1] == NULL)) {
        /* there are no redundant streams, nothing to write to  base recovery
         *  table*/
        return 0;
    }
    /* read timeout value */
    timeout = sysfs_get_recovery_timeout();
    /* fill base recovery array */
    // iterate all streams
    ACMLIST_LOCK(& (config->bypass[0]->streams));
    ACMLIST_FOREACH(stream, &config->bypass[0]->streams, entry)
    {
        if ( (stream->type == REDUNDANT_STREAM_TX) || (stream->type == REDUNDANT_STREAM_RX)) {
            base_recovery_array.timeout[stream->redundand_index] = timeout;
        }
    }
    ACMLIST_UNLOCK(& (config->bypass[0]->streams));

    /* write base recovery table to hardware */
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_BASE_RECOV),
            (char*) &base_recovery_array,
            sizeof (base_recovery_array),
            0);

    TRACE2_EXIT();
    return ret;
}

int __must_check sysfs_write_individual_recovery(struct acm_module *module) {
    struct acmdrv_redun_individual_recovery indiv_rec_array;
    memset(&indiv_rec_array, 0, sizeof (indiv_rec_array));
    struct acm_stream *stream;
    int ret = 0;

    /* fill individual recovery array for module */
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, &module->streams, entry)
    {
        if ((stream->type == REDUNDANT_STREAM_RX) || (stream->type == INGRESS_TRIGGERED_STREAM)){
            indiv_rec_array.module[module->module_id].timeout[stream->lookup_index] = stream
                    ->indiv_recov_timeout_ms;
        }
    }
    ACMLIST_UNLOCK(&module->streams);

    /* write individual recovery table to hardware */
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_INDIV_RECOV),
            (char*) &indiv_rec_array.module[module->module_id],
            sizeof (indiv_rec_array.module[module->module_id]),
            module->module_id * sizeof (indiv_rec_array.module[module->module_id]));

    TRACE2_EXIT();
    return ret;
}

int __must_check write_clear_all_fpga(void) {
    int32_t clear_pattern = ACMDRV_CLEAR_ALL_PATTERN;
    int ret;

    TRACE2_ENTER();
    // write data
    ret = write_buffer_config_sysfs_item(__stringify(ACM_SYSFS_CLEAR_ALL_FPGA),
            (char*)&clear_pattern, sizeof(clear_pattern), 0);

    TRACE2_EXIT();
    return ret;
}
