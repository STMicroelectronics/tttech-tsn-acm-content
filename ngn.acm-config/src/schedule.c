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
#include <stdlib.h>
#include <errno.h>

#include "schedule.h"
#include "logging.h"
#include "tracing.h"
#include "memory.h"

int __must_check schedule_list_init(struct schedule_list *winlist) {
    TRACE3_ENTER();
    if (!winlist) {
        TRACE3_MSG("Fail");
        return -EINVAL;
    }

    ACMLIST_INIT(winlist);
    TRACE3_EXIT();
    return 0;
}

int __must_check schedule_list_add_schedule(struct schedule_list *list,
        struct schedule_entry *schedule) {
    int ret = 0;
    TRACE3_ENTER();

    if (!list || !schedule) {
        LOGERR("%s: parameters must be non-null");
        ret = -EINVAL;
        goto out;
    }
    ACMLIST_INSERT_TAIL(list, schedule, entry);

out:
    TRACE3_EXIT();
    return ret;
}

void schedule_list_remove_schedule(struct schedule_list *list, struct schedule_entry *schedule) {

    TRACE3_ENTER();

    ACMLIST_LOCK(list);
    _ACMLIST_REMOVE(list, schedule, entry);
    schedule_destroy(schedule);
    ACMLIST_UNLOCK(list);

    TRACE3_EXIT();
}


void schedule_list_flush(struct schedule_list *list) {
    TRACE3_ENTER();
    if (!list) {
        TRACE3_MSG("Fail");
        goto out;
    }

    ACMLIST_LOCK(list);
    while (!ACMLIST_EMPTY(list)) {
        struct schedule_entry *schedule;

        schedule = ACMLIST_FIRST(list);
        _ACMLIST_REMOVE(list, schedule, entry);

        schedule_destroy(schedule);
    }
    ACMLIST_UNLOCK(list);

out:
    TRACE3_EXIT();
}

struct schedule_entry *schedule_create(uint32_t time_start,
        uint32_t time_end,
        uint32_t send_time,
        uint32_t period) {
    struct schedule_entry *schedule;

    if (period == 0) {
        LOGERR("Schedulewindow: Nonzero period needed");
        return NULL;
    }

    schedule = acm_zalloc(sizeof (*schedule));
    if (!schedule) {
        LOGERR("%s: Out of memory", __func__);
        return NULL;
    }

    schedule->time_start_ns = time_start;
    schedule->time_end_ns = time_end;
    schedule->send_time_ns = send_time;
    schedule->period_ns = period;
    ACMLIST_ENTRY_INIT(&schedule->entry);

    return schedule;
}

void schedule_destroy(struct schedule_entry *schedule) {
    acm_free(schedule);
}
