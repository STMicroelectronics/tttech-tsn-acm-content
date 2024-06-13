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
#ifndef CONFIG_H_
#define CONFIG_H_

#include "module.h"
#include "libacmconfig_def.h"
#include "buffer.h"

/**
 * @brief starting point of configuration data, which holds all necessary substructures
 */
struct acm_config {
    struct acm_module *bypass[ACM_MODULES_COUNT]; /**< pointers to the modules added to configuration */
    bool config_applied;        /**< set to true, when configuration was already applied to hardware,
         false otherwise */
    struct buffer_list msg_buffs;       /**< list of message buffers of complete configuration;
        if user defined 2 message buffers for which on hardware only one buffer is necessary,
        then this list only contains one buffer item */
};

/**
 * @ingroup acmconfig
 * @brief helper to initialize item of type struct acm_config
 */
#define CONFIGURATION_INITIALIZER(_configuration, _applied)          \
{                                                                    \
    .bypass = { NULL, NULL },                                        \
    .config_applied = (_applied),                                    \
    .msg_buffs = BUFFER_LIST_INITIALIZER((_configuration).msg_buffs) \
}

/**
 * @ingroup acmconfig
 * @brief create a new configuration
 *
 * Create a new ACM configuration object.
 *
 * @return The function returns a pointer to a new configuration object in case of success. It is
 * the responsibility of the caller to administrate the memory of the object.
 * In case of an error NULL is returned.
 */
struct acm_config __must_check *config_create();

/**
 * @ingroup acmconfig
 * @brief Release memory of a configuration
 *
 * The memory of the configuration and the memory of all items linked to the configuration or any
 * linked item (modules, buffer list, streams, schedules,) is released.
 *
 * @param config ACM configuration to be destroyed/deleted
*/
void config_destroy(struct acm_config *config);

/**
 * @ingroup acmconfig
 * @brief Adds the module to the configuration
 *
 * The function first does some basic checks (for instance module id, free pointer in config for
 * module, ...).
 * Then it executes a complete validation of configuration with the new module. Only if this
 * complete validation succeeds the module is added to the configuration and returns success.
 *
 * @param config ACM configuration to which the module shall be added
 * @param module ACM module which shall be added to the configuration
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check config_add_module(struct acm_config *config, struct acm_module *module);

/**
 * @ingroup acmconfig
 * @brief Apply a complete configuration to hardware
 *
 * The function does some basic checks on the parameter values. Then a final validation of the
 * configuration is executed. Only if all these checks succeed, the configuration and the
 * configuration id are written to hardware. The configuration item value 'config_applied' is set
 * to 'true'.
 *
 * @param config ACM configuration to be applied to the device
 * @param identifier configuration id for verification in case of schedule change
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check config_enable(struct acm_config *config, uint32_t identifier);

/**
 * @ingroup acmconfig
 * @brief Apply a schedule to hardware
 *
 * The function does some basic checks on the parameter values, especially on identifier_expected.
 * Then a final validation of the configuration is executed. Only if all these checks succeed,
 * the schedule of the configuration and the configuration id are written to hardware.
 *
 * @param config ACM configuration which schedule shall be applied to the hardware device
 * @param identifier new configuration id
 * @param identifier_expected expected id of configuration actually applied to hardware
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check config_schedule(struct acm_config *config,
        uint32_t identifier,
        uint32_t identifier_expected);

/**
 * @ingroup acmconfig
 * @brief Remove a configuration from hardware
 *
 * The function deletes configuration actually applied to hardware from hardware and writes
 * default delay values (values of connection mode parallel) to hardware afterwards.
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check config_disable(void);

/**
 * @ingroup acmconfig
 * @brief Creates list of message buffers newly
 *
 * The function deletes all items in the message buffer list of the configuration and all links to
 * message buffer data in operations. Then the function creates the list newly using the modules
 * and substructures actually linked to the configuration.
 *
 * @param config ACM configuration which message buffer list shall be recalculated
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check clean_and_recalculate_hw_msg_buffs(struct acm_config *config);

/**
 * @ingroup acmconfig
 * @brief Creates list of message buffers newly
 *
 * The function assumes an empty message buffer list in the configuration.
 * It reads the granularity of the memory of message buffers on hardware from hardware
 * configuration. Using this, it creates message buffer list items for all modules linked to the
 * configuration.
 *
 * @param config ACM configuration which message buffer list shall be calculated
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check create_hw_msg_buf_list(struct acm_config *config);

#ifndef TEST
#define STATIC static
#else
#define STATIC

/**
 * @ingroup acmconfig
 * @brief Creates message buffer items for a module
 *
 * The function iterates through all streams and operations of the streams. For each READ and each
 * INSERT operation it checks if an already existing message buffer item can be reused. If no item
 * can be reused a new message buffer item is created and inserted into msgbuflist. The values of
 * index and offset are incremented according to the new message buffer item.
 * The function also checks if the maximum value of hardware message buffer size is exceeded.
 *
 * @param module module which message buffer items shall be calculated/created
 * @param msgbuflist address of the list of message buffer items of the configuration the module
 *                      is linked to
 * @param granularity block size of message buffer on hardware
 * @param index next free index of message buffers
 * @param offset next free data position in message buffer memory
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check create_hw_msg_buf_list_module(struct acm_module *bypass,
        struct buffer_list *msgbuflist, int32_t granularity, uint8_t *index, uint16_t *offset);

/**
 * @ingroup acmconfig
 * @brief Calculates length of an operation
 *
 * The function uses the value 'length' stored in item operation as base. For operations of type
 * READ the size of timestamp is added.
 *
 * @param operation operation which length shall be calculated
 *
 * @return The function will return the length of the operation
*/
uint16_t __must_check get_oplen(const struct operation *operation);
#endif

#endif /* CONFIG_H_ */
