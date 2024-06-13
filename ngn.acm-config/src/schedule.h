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
#ifndef SCHEDULE_H_
#define SCHEDULE_H_

#include <stdint.h>
#include <pthread.h>
#include "list.h"
#include "libacmconfig_def.h"

/**
 * @brief structure for handling of queue properties: access lock, number of queue items, list of items
 */
struct schedule_entry;
ACMLIST_HEAD(schedule_list, schedule_entry);

/**
 * @brief helper for initializing a schedule_list - especially for module test
 */
#define SCHEDULE_LIST_INITIALIZER(schedlist) ACMLIST_HEAD_INITIALIZER(schedlist)

/**
 * @brief structure which holds all the relevant data of schedules
 */
struct schedule_entry {
    uint32_t time_start_ns; /**< only relevant for schedule window: time of window start in nanoseconds */
    uint32_t time_end_ns; /**< only relevant for schedule window: time of window end in nanoseconds */
    uint32_t send_time_ns; /**< only relevant for schedule events: time of the event in nanoseconds */
    uint32_t period_ns; /**< period length of the schedule in nanoseconds */
    ACMLIST_ENTRY(schedule_list, schedule_entry) entry; /**< links to next and previous schedule_entry and schedule_list header of the stream */
};

/**
 * @brief helper for initializing a schedule entry with default values - especially for module test
 */
#define SCHEDULE_ENTRY_INITIALIZER     \
{                                      \
    .time_start_ns = 0,                \
    .time_end_ns = 0,                  \
    .send_time_ns = 0,                 \
    .period_ns = 0,                    \
    .entry = ACMLIST_ENTRY_INITIALIZER \
}

/**
 * @ingroup acmschedule
 * @brief create a new schedule
 *
 * The function create a new schedule. It returns NULL on failure or a properly initialized
 * schedule_entry object on success.
 *
 * @param time_start Defines the time in nano seconds when the window will be
 *          opened. (relevant for schedules for Ingress Triggered Stream)
 * @param time_end
 * @param send_time the time when the frame shall be on the wire (in nano seconds,
 *          relevant for schedules for Time Triggered Stream)
 * @param period the period for the schedule (in nano seconds)
 *
 * @return the function will return a schedule_entry object in case of success. A NULL pointer
 * represents an error.
 */
struct schedule_entry *schedule_create(uint32_t time_start,
        uint32_t time_end,
        uint32_t send_time,
        uint32_t period);

/**
 * @ingroup acmschedule
 * @brief delete a schedule item
 *
 * The function deletes the schedule item - frees the memory.
 *
 * @param schedule schedule item to be deleted
 */
void schedule_destroy(struct schedule_entry *schedule);

/**
 * @ingroup acmschedule
 * @brief initializes the list of schedules
 *
 * The function sets the number of list elements and the first and last list
 * elements to initial values.
 *
 * @param winlist address of the list of schedules of a stream
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check schedule_list_init(struct schedule_list *winlist);

/**
 * @ingroup acmschedule
 * @brief inserts a schedule into a schedule list of a stream
 *
 * The function adds the schedule to the end of the schedulelist.
 *
 * @param list address of the list of schedules
 * @param schedule pointer to the new schedule item.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check schedule_list_add_schedule(struct schedule_list *list,
        struct schedule_entry *schedule);

/**
 * @ingroup acmschedule
 * @brief remove a schedule item from a schedule list and delete it
 *
 * The function iterates through all items of list and checks if the list item is equal to
 * schedule item in parameter schedule. If schedule is found, it is removed from the list and
 * deleted. If schedule is not found, the function doesn't change the list.
 *
 * @param list schedule list where to look for the schedule item
 * @param schedule schedule to remove from list
 */
void schedule_list_remove_schedule(struct schedule_list *list, struct schedule_entry *schedule);

/**
 * @ingroup acmschedule
 * @brief deletes all elements in a schedule list
 *
 * The function deletes all schedule items in a list of schedules of a stream.
 *
 * @param list address of the list of schedules
 */

void schedule_list_flush(struct schedule_list *list);

#endif /* SCHEDULE_H_ */
