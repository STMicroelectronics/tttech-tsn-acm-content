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
#ifndef MODULE_H_
#define MODULE_H_

#include <stddef.h>
#include "stream.h"
#include "libacmconfig_def.h"
#include "hwconfig_def.h"
#include "list.h"

/**
 * @brief structure for handling of queue properties: access lock, number of queue items, list of items
 */
ACMLIST_HEAD(fsc_command_list, fsc_command);

/**
 * @brief helper for initializing a list of fsc commands - especially for module test
 */
#define COMMANDLIST_INITIALIZER(cmdlist) ACMLIST_HEAD_INITIALIZER(cmdlist)

/**
 * @brief structure which holds all the relevant module data
 */
struct acm_module {
    struct stream_list streams; /**< list of streams defined for the module */
    enum acm_connection_mode mode; /**< connection mode of the module (parallel, serial) */
    enum acm_linkspeed speed; /**< linkspeed of the module */
    enum acm_module_id module_id; /**< identity of the module, unique within a configuratin*/
    uint32_t cycle_ns; /**< length of the period of the module in nanoseconds */
    struct timespec start; /**< start start-time of the configuration */
    struct acm_config *config_reference; /**< pointer to configuration the module was added to */
    struct fsc_command_list fsc_list; /**< list of fsc_commands for the module, calculated from schedules of streams*/
    struct hw_dependent_delay module_delays[2]; /**< one set of delays for each linkspeed (SPEED_100MBps, SPEED_1GBps) */
};

/**
 * @brief helper for initializing a module - especially for module test
 */
#define MODULE_INITIALIZER(_module, _mode, _speed, _id, _config) \
{                                                                \
    .streams = STREAMLIST_INITIALIZER((_module).streams),        \
    .mode = (_mode),                                             \
    .speed = (_speed),                                           \
    .module_id = (_id),                                          \
    .cycle_ns = 0,                                               \
    .start = { 0, 0 },                                           \
    .config_reference = (_config),                               \
    .fsc_list = COMMANDLIST_INITIALIZER((_module).fsc_list),     \
    .module_delays = {                                           \
            { 0, 0, 0, 0, 0, 0 },                                \
            { 0, 0, 0, 0, 0, 0 }                                 \
    }                                                            \
}

/* each stream list is contained in a module */
static inline struct acm_module *streamlist_to_module(const struct stream_list *list) {
    return (void *)((char *)(list) - offsetof(struct acm_module, streams));
}

/**
 * @ingroup acmmodule
 * @brief create a module
 *
 * The function checks the parameter module_id. It allocates memory for
 * the module, sets the module properties to input parameter values
 * and initializes the stream list as empty list.
 * If there occurs any error an error message is written to the errorlog
 * and allocated items are released again.
 *
 * @param mode connection mode of the module (serial or parallel mode)
 * @param speed connection speed the module shall operate at.
 * @param module_id The module id defines the communication direction.
 *	   - MODULE_0 will allow streams from port sw0p3 to port sw0p2
 *	   - MODULE_1 will allow streams from port sw0p2 to port sw0p3
 *
 * @return handle to the new module or NULL if there occurred any error during
 * 		of the module
 */
struct acm_module __must_check* module_create(enum acm_connection_mode mode,
        enum acm_linkspeed speed,
        enum acm_module_id module_id);

 /**
  * @ingroup acmmodule
  * @brief destroy a module
  *
  * The function deletes all streams in the streamlist of the module and then
  * the module itself.
  * If the module has a reference to a configuration, which means it was added
  * to a configuration, nothing is deleted. The module is kept as it is and an
  * error message is written to the errorlog.
  *
  * @param module pointer to module to be destroyed
  */
void module_destroy(struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief Add a stream to a module
 *
 * The function adds the stream only to the module, if all conditions on
 * module level are still fulfilled. If the module was already added to a
 * configuration, then the stream is only added to the module if all conditions
 * on configuration level are also still fulfilled.
 *
 * @param module module where to add the stream configuration to
 * @param stream stream configuration to be added to a module
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check module_add_stream(struct acm_module *module, struct acm_stream *stream);

/**
 * @ingroup acmschedule
 * @brief Set a schedule for a module
 *
 * The function writes the values of the input parameters cycle and start to the
 * module referenced by the input parameter module.
 * If the value of the input parameter module is NULL, an error message is
 * written to error log and the return value is set accordingly.
 *
 * @param module module where to set the schedule to
 * @param cycle schedule cycle duration in nano-seconds
 * @param start start-time of the configuration in seconds and nano-seconds since
 * 			epoch (January 1, 1970 (midnight UTC/GMT), not counting leap seconds)
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check module_set_schedule(struct acm_module *module,
        uint32_t cycle,
        struct timespec start);

/**
 * @ingroup acmmodule
 * @brief create HW schedule items for a logical schedule entry
 *
 * The function prepares all the necessary HW schedule items and puts them into
 * the appropriate list of the module.
 * If there occurs any problem during the creation of the items, all so far created
 * items are removed and deleted again.
 *
 * @param stream stream which was recently added to the module and contains the
 * 			logical schedule item
 * @param schedule_item logical schedule item for which the HW schedule items
 * 			are created
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check module_add_schedules(struct acm_stream *stream,
        struct schedule_entry *schedule_item);

/**
 * @ingroup acmmodule
 * @brief delete HW schedule items for a logical schedule entry
 *
 * The function iterates through all HW schedule items of the module and deltes
 * the HW schedule items which have a reference to parameter "schedule item".
 *
 * @param schedule_item logical schedule item for which the HW schedule items
 * 			are deleted
 * @param module module from which the HW schedule items are deleted
 *
 */
void remove_schedule_sysfs_items_schedule(struct schedule_entry *schedule_item,
        struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief delete HW schedule items for a logical schedule entries of a stream
 *
 * The function iterates through all logical schedule items of a stream. For
 * each logical schedule item all HW schedule items are deleted. (All HW schedule
 * items of the module, which have a reference to any logical schedule item of
 * the stream, are deleted.)
 *
 * @param stream stream for which all HW schedule items
 * 			are deleted
 * @param module module from which the HW schedule items are deleted
 *
 */
void remove_schedule_sysfs_items_stream(struct acm_stream *stream, struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief delete all HW schedule items of a list of fsc schedule commands
 *
 * The function iterates through all logical schedule items of a stream. For
 * each logical schedule item all HW schedule items are deleted. (All HW schedule
 * items of the module, which have a reference to any logical schedule item of
 * the stream, are deleted.)
 *
 * @param fsc_list list from which all HW schedule items are deleted
 *
 */
void fsc_command_empty_list(struct fsc_command_list *fsc_list);

/**
 * @ingroup acmmodule
 * @brief calculate number of nop commands necessary for the module
 *
 * The function iterates through the complete fsc command list and checks for each if the time
 * difference to the next fsc command is greater than UINT16_T_MAX. If this is the case, than the
 * counter for nop commands is increased by the integer part of time difference/NOP_DELTA_CYCLE.
 *
 * @param fsc_list pointer to the fsc command list of a module
 *
 * @return the function will return the number of nop commands. Zero if list is empty or pointer to
 * list is Null.
 */
uint __must_check calc_nop_schedules_for_long_cycles(struct fsc_command_list *fsc_list);

/**
 * @ingroup acmmodule
 * @brief write module specific data to hardware
 *
 * The function writes all module specific data, except schedules, to hardware. These are:
 *     - constant buffer data
 *     - lookup tables and stream triggers
 *     - scatters DMA commands
 *     - gather DMA commands and prefetch commands
 *     - connection mode
 *     - delays for both speeds
 *     - redundancy control table
 *     - individual recovery array
 *     - speed
 *     - module enable flag
 *
 * @param module pointer to the module which data shall be written to HW
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check write_module_data_to_HW(struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief write schedule data of the module to hardware
 *
 * The function first looks for the free HW schedule table and then writes all module specific
 * schedule data to hardware. These are:
 *     - all schedule items of all streams of the module
 *     - cycle time and start time of the module
 *     - emergency disable flag
 *
 * @param module pointer to the module which schedule data shall be written to HW
 *
 * @return the function will return 0 in case of success. Negative values represent an error.
 */
int __must_check write_module_schedule_to_HW(struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief delete links from operations to message buffers for module
 *
 * The function iterates through all streams of the module and calls for each stream a function to
 * delete links of all its operations to message buffers.
 *
 * @param module pointer to the module which message buffer links shall be cleaned
 */
void module_clean_msg_buff_links(struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief initialize delay values of the module
 *
 * The function checks for each of the following delays if there is a configuration value in the
 * configuration file. If this is the case, the configuration value is written to the module delay
 * variable. If no configuration value is found for a delay, the default value for this delay is
 * written to the module delay variable.
 *      - chip_in
 *      - chip_eg
 *      - phy_in
 *      - phy_eg
 *      - KEY_SER_BYPASS_100MBps
 *      - KEY_SER_SWITCH_100MBps
 * Delays for all speeds are initialized independent of the actually configured speed in the module.
 *
 * @param module pointer to the module which delays shall be initialized.
 */
void module_init_delays(struct acm_module *module);

/**
 * @ingroup acmmodule
 * @brief read a delay value from configuration file
 *
 * The function reads a value with name config_item from the configuration file and assigns it
 * to parameter config_value.
 *
 * @param config_item name of a configuration item in the configuration file
 * @param config_value value of a configuration item in the configuration file, only valid when
 *              function returned 0
 *
 * @return the function will return 0 in case of success. Negative values represent an error.
 */
int __must_check module_get_delay_value(const char *config_item, uint32_t *config_value);

#endif /* MODULE_H_ */
