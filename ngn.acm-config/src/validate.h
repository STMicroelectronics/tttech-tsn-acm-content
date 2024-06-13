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

#ifndef SRC_VALIDATE_H_
#define SRC_VALIDATE_H_

#include <stdint.h>
#include <stdbool.h>

#include "buffer.h"
#include "stream.h"
#include "module.h"
#include "config.h"
#include "libacmconfig_def.h"

/**
 * @ingroup acmvalidate
 * @brief validate a stream
 *
 * In case of final validation, the function first executes the validation
 * of all substructures and then the validations of stream.
 * In case of non final validation, the function first executes the validations
 * of stream and then the validation of the superior structure, which is
 * the streamlist.
 * Validations for a stream:
 * - check if framesize >= minimum frame size (64) - only at final validation
 * - check if redundant streams are added to same configuration
 * - check if redundant streams are added to different modules
 * - check if payload size is lower than maximum (1518)
 * - check truncation of forward operations.
 * - check if number of insert operations <= maximum (8)
 *
 * @param stream pointer to the stream to be validated
 * @param final_validate flag which specifies if a final validation
 * 				should be done or not
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check validate_stream(struct acm_stream *stream, bool final_validate);

/**
 * @ingroup acmvalidate
 * @brief validate a stream list
 *
 * In case of final validation, the function first executes the validation
 * of all substructures and then the validations of stream list general.
 * In case of non final validation, the function first executes the validations
 * of stream list and then the validation of the superior structure, which is
 * the module.
 * There are no specific validations on the stream list.
 *
 * @param stream_list pointer to the stream list to be validated
 * @param final_validate flag which specifies if a final validation
 * 				should be done or not
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check validate_stream_list(struct stream_list *stream_list,
bool final_validate);

/**
 * @ingroup acmvalidate
 * @brief validate a module
 *
 * In case of final validation, the function first executes the validation
 * of all substructures and then the validations of the module.
 * In case of non final validation, the function first executes the validations
 * of module and then the validation of the superior structure, which is
 * the configuration.
 * Validations for a module:
 * - check if size of all constant buffers <= maximum (4096)
 * - check if number of redundant streams <= maximum (31 external, 32 internal)
 * - check if number of schedules <= maximum (1024)
 * - check if module period was set
 * - check if all schedule periods are compatible with module period
 * - check if time distance between 2 scheduling events >= minimum (8 ticks)
 * - check if number of egress operations (gather) <= maximum (254 external, 256 internal)
 * - check if number of ingress operations (scatter) <= maximum (255 external, 256 internal)
 * - check if number of lookup entries <= maximum (16)
 *
 * @param module pointer to the module to be validated
 * @param final_validate flag which specifies if a final validation
 * 				should be done or not
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check validate_module(struct acm_module *module, bool final_validate);

/**
 * @ingroup acmvalidate
 * @brief validate a configuration
 *
 * In case of final validation, the function first executes the validation
 * of all substructures and then the validations of the configuration.
 * In case of non final validation, the function just executes the validations
 * of configuration.
 * Validations for a configuration:
 * - check if each stream has at least one operation - only at final validation
 * - check if number of message buffers <= configured value
 *
 * @param config pointer to the configuration to be validated
 * @param final_validate flag which specifies if a final validation
 * 				should be done or not
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check validate_config(struct acm_config *config, bool final_validate);

/**
 * @ingroup acmvalidate
 * @brief calculates sum of framesizes of egress operations of a stream
 *
 * The function iterates through all egress operations of the stream and sums up
 * their egress framesizes.
 *
 * @param stream pointer to the stream for which sum of egress framesizes is
 * 				calculated
 *
 * @return The function will return 0 in case the module has no egress operations.
 * 			Otherwise the calculated sum is returned (number of bytes).
 */
int __must_check calc_stream_egress_framesize(struct acm_stream *stream);

/**
 * @ingroup acmvalidate
 * @brief checks all streams of a module if stream has at least one operation
 *
 * The function iterates through all streams of the module and checks if the
 * stream has at least one operation.
 * If an Ingress Triggered Stream has no operation but a reference to an Event
 * Stream with an operation, than it is handled as a stream which has an operation.
 *
 * @param module pointer to the module for which the existence of operations
 * 				is to be checked
 *
 * @return The function returns false in case one or more streams of the module
 * 				have no operation. The function returns true if all streams of
 * 				the module have at least one operation.
 */
bool __must_check check_module_op_exists(struct acm_module *module);

/**
 * @ingroup acmvalidate
 * @brief calculates the sum of all constant buffers of a stream
 *
 * The function iterates through all operations of the stream. For operations
 * of type INSERT_CONSTANT the length of the inserted constant is added to the
 * sum.
 *
 * @param stream pointer to the stream for which the sum of constant buffers
 * 			is calculated
 *
 * @return The function will return 0 in case the stream has no constant buffer.
 * 			Otherwise the calculated sum is returned (number of bytes).
 */
int __must_check stream_sum_const_buffer(struct acm_stream *stream);

/**
 * @ingroup acmvalidate
 * @brief check all periods of a stream
 *
 * The function iterates through all schedules of the stream. For each schedule
 * the function checks if the value of parameter module_cycle_ns can be divided
 * by the period of the schedule without rest.
 *
 * @param stream pointer to the stream for which the schedule periods are
 * 			checked.
 * @param module_cycle_ns period to check against
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check stream_check_periods(struct acm_stream *stream, uint32_t module_cycle_ns,
		bool final_validate);

/**
 * @ingroup acmvalidate
 * @brief check scheduling time difference between scheduling events of a stream
 *
 * The function iterates through all fsc schedule items (scheduling items
 * preprocessed for transfer to HW - time in ticks instead of nanoseconds) of
 * the module. For each fsc schedule item the function checks if the time
 * difference to the previous item is equal or longer than a minimum value.
 * Precondition is that the list is sorted regarding the time.
 *
 * @param module pointer to the module for which the time differences between
 * 			scheduling events are checked.
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check check_module_scheduling_gaps(struct acm_module *module);

/**
 * @ingroup acmvalidate
 * @brief check payload of a stream
 *
 * The function iterates through all operations of the stream. For operations READ and FORWARD_ALL
 * it does nothing. For operations INSERT, INSERT_CONSTANT and PAD it increments egress-position
 * and ingress-position by the length of the operation.
 * For FORWARD operation it checks if the offset of the operation is higher than the so far
 * calculated ingress_pos plus MAX_TRUNC_BYTES (= 19). In case this condition is not fulfilled, the
 * functions returns an error. In case this condition is fulfilled, the egress-position is
 * incremented by the length of the operation. The ingress-position is set to the offset of the
 * FORWARD operation, if it is smaller than the offset of the FORWARD operation. Afterwards it is
 * incremented by the length of the operation.
 * After iteration through all operations, the function checks if the egress-position exceeds a
 * maximum value of 1518 (= maximum payload size within frame). If the egress-position exceeds the
 * value, an error is returned.
 *
 * @param stream pointer to the stream for which the payload is checked.
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check check_stream_payload(struct acm_stream *stream);

/**
 * @ingroup acmvalidate
 * @brief check buffername against a list of buffers
 *
 * The function iterates through all message buffers in the buffer list and checks if the name of
 * message buffer msg_buf is equal to one of the message buffers in the list.
 * If no message buffer with equal name is found in the list, the function returns 0 (sucess),
 * reuse parameter has value "false".
 * If a message buffer with equal name is found in the list and the found message buffer and
 * message buffer msg_buf have the same direction, the function also returns 0 (sucess), reuse
 * parameter has value "true" and parameter reuse_msg_buff points to the found message buffer
 * in the list.
 * If a message buffer with equal name is found in the list and the found message buffer and
 * message buffer msg_buf have different direction, the function returns an error.
 *
 * @param sysfs_buffer_list pointer to a list of message buffers.
 * @param msg_buf pointer to the message buffer, which is to be checked.
 * @param reuse pointer to boolean value, contains true after function return, if in buffer list a
 *      message buffer was found, which can be reused for message buffer. Otherwise it
 *      contains false.
 * @param reuse_msg_buff pointer to the pointer of the message buffer, which can be reused in case
 *      of reuse is true. Nullpointer otherwise.
  *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check buffername_check(struct buffer_list *sysfs_buffer_list,
        struct sysfs_buffer *msg_buf,
        bool *reuse,
        struct sysfs_buffer **reuse_msg_buff);

#endif /* SRC_VALIDATE_H_ */
