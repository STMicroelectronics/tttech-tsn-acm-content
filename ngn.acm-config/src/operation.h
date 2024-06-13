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
#ifndef OPERATION_H_
#define OPERATION_H_

#include <stdint.h>
#include <pthread.h>
#include "list.h"
#include "libacmconfig_def.h"

/**
 * @brief number automatically created operations at stream creation
 */
#define NUM_AUTOGEN_OPS     3U

/**
 * @brief types of operations
 *
 * enum acm_operation_code is used to have an indication of which type an operation is so that it
 * is possible to derive valid properties of the operation from this info.
 */
enum acm_operation_code {
    INSERT, /**< insert specified data into a buffer - time, event, recovery and redundant TX streams */
    INSERT_CONSTANT, /**< insert constant data into a stream - time, event, recovery and redundant TX streams */
    PAD, /**< insert padding data into a stream - time, event, recovery and redundant TX streams */
    FORWARD, /**< forward specified data of a stream - only event streams */
    READ, /**< read specified data from a buffer - only ingress and redundant RX streams */
    FORWARD_ALL, /**< forward all data of a stream - only ingress streams */
    OPERATION_MAX,
};

/**
 * @brief structure for handling of queue properties: access lock, number of queue items, list of items
 */
struct operation;
ACMLIST_HEAD(operation_list, operation);

/**
 * @brief structure which holds all the relevant operation data
 */
struct operation {
    enum acm_operation_code opcode; /**< specifies which type the operation has */
    uint16_t offset; /**< only relevant for "forward" and "read", offset from start-of-frame of the ingress frame */
    uint16_t length; /**< relevant for all operations except "forward_all", number of bytes the operation applies on */
    char *buffer_name; /**< only relevant for "insert" and "read" */
    char *data; /**< only relevant for "insert constant" and "pad", points to data operation shall insert */
    uint16_t data_size; /**< only relevant for "insert constant" and "pad", length of the data */
    ACMLIST_ENTRY(operation_list, operation) entry; /**< links to next and previous operation and operation_list header of the stream */
    bool ownership_set; /**< flag to handle if operation was added to a list or not */
    struct sysfs_buffer *msg_buf; /**< only relevant for "insert" and "read"  */
    uint16_t const_buff_offset; /**< only relevant for "insert constant"; offset in constant buffer on HW */
};

/**
 * @brief helper for initializing an operation - non default value for all operation items possible
 *
 * especially for module test
 */
#define OPERATION_INITIALIZER(_opcode, _offset, _length, _buffer_name, _data,   \
        _data_size, _msg_buf, _const_buff_offset)                                           \
{                                                                                           \
    .opcode = (_opcode),                                                                    \
    .offset = (_offset),                                                                    \
    .length = (_length),                                                                    \
    .buffer_name = (_buffer_name),                                                          \
    .data = (_data),                                                                        \
    .data_size = (_data_size),                                                              \
    .entry = ACMLIST_ENTRY_INITIALIZER,                                                     \
    .ownership_set = false,                                                                 \
    .msg_buf = (_msg_buf),                                                                  \
    .const_buff_offset = (_const_buff_offset)                                               \
}

/**
 * @brief helper for initializing an insert operation - especially for module test
 */
#define INSERT_OPERATION_INITIALIZER(_length, _buffer_name, _msg_buf) \
    OPERATION_INITIALIZER(INSERT, 0, _length, _buffer_name, NULL, 0, _msg_buf, 0)
/**
 * @brief helper for initializing a forward-all operation - especially for module test
 */
#define FORWARD_ALL_OPERATION_INITIALIZER \
    OPERATION_INITIALIZER(FORWARD_ALL, 0, 0, NULL, NULL, 0, NULL, 0)
/**
 * @brief helper for initializing an insert-constant operation - especially for module test
 */
#define INSERT_CONSTANT_OPERATION_INITIALIZER(_data, _data_size) \
    OPERATION_INITIALIZER(INSERT_CONSTANT, 0, _data_size, NULL, _data, _data_size, NULL, 0)
/**
 * @brief helper for initializing a pad operation - especially for module test
 */
#define PAD_OPERATION_INITIALIZER(_length, _data) \
    OPERATION_INITIALIZER(PAD, 0, _length, NULL, _data, 1, NULL, 0)
/**
 * @brief helper for initializing a forward operation - especially for module test
 */
#define FORWARD_OPERATION_INITIALIZER(_offset, _length) \
    OPERATION_INITIALIZER(FORWARD, _offset, _length, NULL, NULL, 0, NULL, 0)
/**
 * @brief helper for initializing a read operation - especially for module test
 */
#define READ_OPERATION_INITIALIZER(_offset, _length, _buffer_name) \
    OPERATION_INITIALIZER(READ, _offset, _length, _buffer_name, NULL, 0, NULL, 0)

/**
 * @brief helper for initializing a operation_list - especially for module test
 */
#define OPERATION_LIST_INITIALIZER(oplist) ACMLIST_HEAD_INITIALIZER(oplist)

/**
 * @ingroup acmoperation
 * @brief create an "insert"-operation
 *
 * The function checks the input parameters length and buffer name.
 * If they are okay the operation item and the associated message buffer
 * item are created.
 *
 * @param length number of bytes to insert from the buffer
 * @param buffer_name name of the character device. The device will be created when applying
 * the configuration.
 *
 * @return the function will return a pointer to the created "operation" item in case
 * of success. The value NULL represents an error.
 */
struct operation __must_check* operation_create_insert(uint16_t length, const char *buffer_name);

/**
 * @ingroup acmoperation
 * @brief create an "insertconstant"-operation
 *
 * The function checks the length of the data to be inserted.
 * If it is okay the operation item is created and the content data is
 * stored.
 *
 * @param data pointer to data to be inserted into the frame
 * @param data_size size of the data
 *
 * @return the function will return a pointer to the created "operation" item in case
 * of success. The value NULL represents an error.
 */
struct operation __must_check* operation_create_insertconstant(const char *data, uint16_t data_size);

/**
 * @ingroup acmoperation
 * @brief create a "pad"-operation
 *
 * The function checks if the number of bytes to be padded is within
 * limits.
 * If this is okay the operation item is created and the padding value is
 * stored.
 *
 * @param length number of bytes to padded in the frame
 * @param pad_value padding value used to insert(0-255)
 *
 * @return the function will return a pointer to the created "operation" item in case
 * of success. The value NULL represents an error.
 */
struct operation __must_check* operation_create_pad(uint16_t length, char pad_value);

/**
 * @ingroup acmoperation
 * @brief create a "forward"-operation
 *
 * The function checks if the input parameters fulfill given conditions.
 * If values of input parameters okay, the operation item is created and
 * parameter values are stored within it.
 *
 * @param offset offset from start-of-frame of the ingress frame. Maximal 19 bytes can
 * be skipped when defining the offset, i.e. when the operation shall insert data to the
 * egress frame on byte-position 20 it is possible to forward from an ingress offset of
 * 39. Skipping more bytes is not possible as the data would not have been received by the
 * device yet.
 * @param length number of bytes to be forwarded
 *
 * @return the function will return a pointer to the created "operation" item in case
 * of success. The value NULL represents an error.
 */
struct operation __must_check* operation_create_forward(uint16_t offset, uint16_t length);

/**
 * @ingroup acmoperation
 * @brief create a "read"-operation
 *
 * The function checks the input parameters length and buffer name.
 * If they are okay the operation item and the associated message buffer
 * item are created.
 *
 * @param offset offset from start-of-frame of the ingress frame.
 * @param length number of bytes to be read to the character device
 * @param buffer_name name of the message buffer for the read operation.
 *
 * @return the function will return a pointer to the created "operation" item in case
 * of success. The value NULL represents an error.
 */
struct operation __must_check* operation_create_read(uint16_t offset,
        uint16_t length,
        const char *buffer_name);

/**
 * @ingroup acmoperation
 * @brief create a "forwardall"-operation
 *
 * The function creates the operation item. Only if there isn't
 * enough memory left over for the operation item, NULL is returned.
 *
 * @return the function will return a pointer to the created "operation" item in case
 * of success. The value NULL represents an error.
 */
struct operation __must_check *operation_create_forwardall();

/**
 * @ingroup acmoperation
 * @brief delete an operation item
 *
 * The function deletes all sub-items (buffer, constant data in case of
 * "insertconstant" operation item) linked to the operation item and
 * the operation item itself.
 *
 * @param operation operation item to be deleted
 */
void operation_destroy(struct operation *operation);

/**
 * @ingroup acmoperation
 * @brief initialize an operation list
 *
 * The function initializes administration values of a new
 * operation list: first and last list item, number of items
 *
 * @param list operation list to be initialized
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check operation_list_init(struct operation_list *list);

/**
 * @ingroup acmoperation
 * @brief add an operation to an operation list
 *
 * The function adds the operation to the end of the operation list. The function
 * returns an error, if  the validation of the extended operation list
 * doesn't succeed. (Validation is done depending on linking status
 * of the operation list up to configuration.)
 *
 * @param list operation list where to insert the operation
 * @param op operation to be inserted into operation list
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check operation_list_add_operation(struct operation_list *list, struct operation *op);

/**
 * @ingroup acmoperation
 * @brief remove an operation from an operation list
 *
 * The function iterates through all items of list and checks if the list item is equal to
 * operation in parameter op. If operation op is found, it is removed from the list. If operation op
 * is not found, the function doesn't change the list.
 *
 * @param op operation to remove from list
 * @param list operation list where to look for the operation
 */
void operation_list_remove_operation(struct operation_list *list, struct operation *op);

/**
 * @ingroup acmoperation
 * @brief delete all items in an operation list
 *
 * The function deletes all operation items in oplist. oplist has then same
 * status as after initialization.
 *
 * @param list operation list to empty
 */
void operation_list_flush(struct operation_list *list);

/**
 * @ingroup acmoperation
 * @brief delete all user created items in an operation list
 *
 * The function deletes all operation items in oplist except those which were
 * created automatically when the associated stream was created
 * (stream_set_egress_header).
 *
 * @param list operation list to empty
 */
void operation_list_flush_user(struct operation_list *list);

/**
 * @ingroup acmoperation
 * @brief set smac values of "insert constant" operations to mac of module
 *
 * Note: When an operation is created for a stream, the stream might not yet be added to a module.
 * In this case the MAC of the module can't be written to the operation (a placeholder is written
 * instead). This has to be done then, when the stream is added to a module.
 *
 * The function reads the MAC of the module. Then it iterates through the list of operations. For
 * each operation with operation code equal "insert constant" it checks if source MAC has the
 * placeholder value. Placeholder value is replaced by the MAC of the module.
 *
 * @param list operation list to work on
 * @param id module_id relevant for the operation list
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check operation_list_update_smac(struct operation_list *list, enum acm_module_id id);

#endif /* OPERATION_H_ */
