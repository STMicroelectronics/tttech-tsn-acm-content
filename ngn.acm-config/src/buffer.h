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

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>
#include <pthread.h>
#include <linux/acm/acmdrv.h>

#include "libacmconfig_def.h"
#include "operation.h"
#include "list.h"

struct sysfs_buffer;
ACMLIST_HEAD(buffer_list, sysfs_buffer);

#define BUFFER_LIST_INITIALIZER(list) ACMLIST_HEAD_INITIALIZER(list)

/**
 * @brief structure to hold message buffer data from user input
 */
struct sysfs_buffer {
    uint8_t msg_buff_index;   /**< index of message buffer (from 0 to 31) */
    uint16_t msg_buff_offset; /**< sum of message buffer sizes of all buffers with lower indexes */
    bool reset; /**< default value 'false' */
    enum acmdrv_buff_desc_type stream_direction; /**< receive or transmit buffer */
    uint16_t buff_size; /**< length of buffer in number of blocks of 4 bytes */
    bool timestamp; /**< default value 'true' */
    bool valid; /**< default value 'true' */
    char *msg_buff_name; /**< buffer name defined by user at creation of operations READ/INSERT */
    ACMLIST_ENTRY(buffer_list, sysfs_buffer) entry; /**< administrative list items */
};

/**
 * @brief helper to initialize item of type struct sysfs_buffer
 */
#define BUFFER_INITIALIZER( _msg_buff_index, _msg_buff_offset, _reset, _stream_direction,   \
        _buff_size, _timestamp, _valid, _msg_buff_name)                                     \
{                                                                                           \
    .msg_buff_index = (_msg_buff_index),                                                    \
    .msg_buff_offset = (_msg_buff_offset),                                                  \
    .reset = (_reset),                                                                      \
    .stream_direction = (_stream_direction),                                                \
    .buff_size = (_buff_size),                                                              \
    .timestamp = (_timestamp),                                                              \
    .valid = (_valid),                                                                      \
    .msg_buff_name = (_msg_buff_name),                                                      \
    .entry = { NULL }                                                                       \
}

/**
 * @brief buffer table type
 *
 * enum buff_table_type is used to differentiate between message buffer alias
 * and message buffer descriptor
*
 */
enum buff_table_type {
    BUFF_DESC = 0, /**< buffer descriptors table */
    BUFF_ALIAS = 1, /**< buffer aliases table */
};

/**
 * @brief create a message buffer item with data for acm driver
 *
 * The function checks the operation code (if it is a valid code for a message
 * buffer) and derives the stream direction from it (tx/rx). The function
 * allocates memory for a new sysfs_buffer item and writes all necessary data
 * to it and returns the sysfs_buffer item to the caller.
 *
 * @param operation pointer to the operation for which a message buffer item
 *              has to be created
 * @param buffer_index index of the new message buffer item in the message
 *              buffer table (0 - 31)
 * @param buffer_offset offset of the new message buffer item in the message
 *              buffer data (depends on the length of the previous items in the
 *              table, complete size: 16K)
 * @param buffer_size length of the message buffer data of the new message
 *              buffer item - already in units for writing to HW (not in units
 *              specified by user)
 *
 * @return pointer to the newly created message buffer item. NULL in case of
 * an error.
 */
struct sysfs_buffer __must_check* buffer_create(struct operation *operation,
        uint8_t buffer_index,
        uint16_t buffer_offset,
        uint16_t buffer_size);

/**
 * @brief free a message buffer item
 *
 * The function releases memory of message buffer item and linked memory of
 * message buffer name. If message buffer item was added to a queue, it has
 * to be removed from the queue before calling buffer_destroy to keep the queue
 * consistent.
 *
 * @param message_buffer pointer to the operation for which a message buffer item
 *              has to be created
 */
void buffer_destroy(struct sysfs_buffer *message_buffer);

/**
 * @brief initializes a queue for items of type struct sysfs_buffer
 *
 * The function initializes all necessary queue parameters for the list (mutex,
 * pointer to first and last item, number of items)
 *
 * @param buffer_list pointer to a list which can store items of type
 *              struct buffer_list
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check buffer_init_list(struct buffer_list *buffer_list);

/**
 * @brief adds an item of type struct sysfs_buffer into a queue
 * of type struct buffer_list
 *
 * The function adds the buffer item at the end of the queue and adapts
 * the number of items in the queue.
 * Items can only be added to initialized queues. It is the responsibility of
 * the caller to use correct values for the parameters.
 *
 * @param buffer_list pointer to a list which can store items of type
 *              struct buffer_list
 * @param buffer pointer to the item to be inserted into the list
 *
 */
void buffer_add_list(struct buffer_list *buffer_list, struct sysfs_buffer *buffer);

/**
 * @brief deletes all items in a list
 *
 * The function removes all items from the list an releases the memory of the
 * items. It also resets the number of list items to zero.
 * If the pointer to the list is NULL or the list doesn't contain any item, the
 * function does nothing.
 *
 * @param buffer_list pointer to a list which can store items of type
 *              struct buffer_list
 */
void buffer_empty_list(struct buffer_list *buffer_list);

/**
 * @brief updates buffer offset values of all items in the list, which are in
 *              the queue after the item message_buffer
 *
 * The function first looks for the item message_buffer in the list buffer_list.
 * When the item is found, then in all items after it, the value offset is newly
 * calculated and the item is updated with it.
 * If the item message_buffer is not found in the list, nothing is changed.
 *
 * @param buffer_list pointer to a list which can store items of type
 *              struct buffer_list
 * @param message_buffer pointer to an item in the list
 */

void update_offset_after_buffer(struct buffer_list *buffer_list,
        struct sysfs_buffer *message_buffer);

#endif /* BUFFER_H_ */
