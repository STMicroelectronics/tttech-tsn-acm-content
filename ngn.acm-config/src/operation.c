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
#include <string.h>
#include <errno.h>

#include "operation.h"
#include "logging.h"
#include "tracing.h"
#include "memory.h"
#include "libacmconfig_def.h"
#include "validate.h"
#include "sysfs.h"

/**
 * @brief structure for checking the specified length values of operations
 */
struct allowed_operation_size {
    unsigned int min; /**< minimum allowed value for length of operation */
    unsigned int max; /**< minimum allowed value for length of operation */
};

static const struct allowed_operation_size op_boundary[] = {
        [INSERT] = { .min =	3, .max = 1500 },
        [INSERT_CONSTANT] = { .min = 1, .max = 1500 },
        [PAD] = { .min = 1, .max = 1500 },
        [FORWARD] = { .min = 2, .max = 1508 },
        [READ] = { .min = 4, .max = 1528 }, };

struct operation __must_check* operation_create_insert(uint16_t length, const char *buffer_name) {
    struct operation *operation;
    int ret;

    TRACE2_ENTER();
    TRACE2_MSG("length=%d, buffer_name=%s", length, buffer_name);

    /* Check input values */
    if (!buffer_name || strlen(buffer_name) > ACM_MAX_NAME_SIZE || strlen(buffer_name) == 0) {
        LOGERR("Operation: invalid buffer name - length");
        TRACE2_MSG("Fail");
        return NULL;
    }

    ret = check_buff_name_against_sys_devices(buffer_name);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (length < op_boundary[INSERT].min) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (length > op_boundary[INSERT].max) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation = acm_zalloc(sizeof (*operation));

    if (!operation) {
        LOGERR("Operation: Out of memory");
        return NULL;
    }

    /* create operation */

    operation->opcode = INSERT;
    operation->length = length;
    operation->buffer_name = acm_strdup(buffer_name);
    if (!operation->buffer_name) {
        acm_free(operation);
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    TRACE2_EXIT();
    return operation;
}

struct operation __must_check* operation_create_insertconstant(const char *data, uint16_t data_size) {
    struct operation *operation;

    TRACE2_ENTER();
    TRACE2_MSG("Called, size: %d", data_size);

    if (data_size < op_boundary[INSERT_CONSTANT].min) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (data_size > op_boundary[INSERT_CONSTANT].max) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }
    operation = acm_zalloc(sizeof (*operation));

    if (!operation) {
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation->opcode = INSERT_CONSTANT;
    operation->length = data_size;
    operation->data = acm_zalloc(data_size);
    if (!operation->data) {
        acm_free(operation);
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }
    memcpy(operation->data, data, data_size);
    operation->data_size = data_size;

    TRACE2_EXIT();
    return operation;
}

struct operation __must_check* operation_create_pad(uint16_t length, char pad_value) {
    struct operation *operation;

    TRACE2_ENTER();
    //TRACE2_MSG("length=%d, pad_value=%c", length, pad_value);

    if (length < op_boundary[PAD].min) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (length > op_boundary[PAD].max) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation = acm_zalloc(sizeof (*operation));

    if (!operation) {
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation->opcode = PAD;
    operation->length = length;

    operation->data = acm_zalloc(1);
    if (!operation->data) {
        acm_free(operation);
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }
    operation->data[0] = pad_value;
    operation->data_size = 1;

    TRACE2_EXIT();
    return operation;
}

struct operation __must_check* operation_create_forward(uint16_t offset, uint16_t length) {
    struct operation *operation;

    TRACE2_ENTER();
    TRACE2_MSG("offset=%d, length=%d", offset, length);

    if (length < op_boundary[FORWARD].min) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (length > op_boundary[FORWARD].max) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if ( (offset + length) > ACM_MAX_FRAME_SIZE) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation = acm_zalloc(sizeof (*operation));

    if (!operation) {
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation->opcode = FORWARD;
    operation->offset = offset;
    operation->length = length;

    TRACE2_EXIT();
    return operation;
}

struct operation __must_check* operation_create_read(uint16_t offset,
        uint16_t length,
        const char *buffer_name) {
    struct operation *operation;
    int ret;

    TRACE2_ENTER();
    TRACE2_MSG("offset=%d, length=%d, buffer=%s", offset, length, buffer_name);

    if (!buffer_name || strlen(buffer_name) > ACM_MAX_NAME_SIZE || strlen(buffer_name) == 0) {
        LOGERR("Operation: invalid buffer name - length");
        TRACE2_MSG("Fail");
        return NULL;
    }

    ret = check_buff_name_against_sys_devices(buffer_name);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (length < op_boundary[READ].min) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if (length > op_boundary[READ].max) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    if ( (offset + length) > ACM_MAX_FRAME_SIZE) {
        LOGERR("Operation: Invalid size");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation = acm_zalloc(sizeof (*operation));

    if (!operation) {
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation->opcode = READ;
    operation->length = length;
    operation->offset = offset;

    operation->buffer_name = acm_strdup(buffer_name);
    if (!operation->buffer_name) {
        acm_free(operation);
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    TRACE2_EXIT();
    return operation;
}

struct operation __must_check* operation_create_forwardall() {
    struct operation *operation;

    TRACE2_ENTER();
    operation = acm_zalloc(sizeof (*operation));

    if (!operation) {
        LOGERR("Operation: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    operation->opcode = FORWARD_ALL;

    TRACE2_EXIT();
    return operation;
}

void operation_destroy(struct operation *operation) {
    TRACE2_ENTER();
    if (!operation) {
        TRACE2_MSG("Fail");
        return;
    }

    acm_free(operation->data);
    acm_free(operation->buffer_name);
    acm_free(operation);
    TRACE2_EXIT();
}

int __must_check operation_list_init(struct operation_list *list) {
    TRACE2_ENTER();
    if (!list) {
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    ACMLIST_INIT(list);
    TRACE2_EXIT();
    return 0;
}

int operation_list_add_operation(struct operation_list *list, struct operation *op) {
    int ret;

    TRACE2_ENTER();
    if (!list || !op) {
        LOGERR("Operation: wrong parameter in %s", __func__);
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    if (op->ownership_set) {
        LOGERR("Operation: cannot be added multiple times");
        TRACE2_MSG("Fail");
        return -EPERM;
    }

    ACMLIST_INSERT_TAIL(list, op, entry);
    op->ownership_set = true;

    ret = validate_stream(operationlist_to_stream(list), false);
    if (ret != 0) {
        ACMLIST_REMOVE(list, op, entry);
        op->ownership_set = false;
        TRACE2_MSG("Fail");
        return ret;
    }

    TRACE2_EXIT();
    return 0;
}

void operation_list_remove_operation(struct operation_list *list, struct operation *op) {
    TRACE2_ENTER();
    ACMLIST_REMOVE(list, op, entry);
    TRACE2_EXIT();
}

void operation_list_flush(struct operation_list *list) {

    TRACE2_ENTER();
    if (!list) {
        LOGERR("Operation: wrong parameter in %s", __func__);
        TRACE2_MSG("Fail");
        return;
    }

    ACMLIST_LOCK(list);

    while (!ACMLIST_EMPTY(list)) {
        struct operation *operation;

        operation = ACMLIST_FIRST(list);
        _ACMLIST_REMOVE(list, operation, entry);

        operation_destroy(operation);
    }

    ACMLIST_UNLOCK(list);
    TRACE2_EXIT();
}

void operation_list_flush_user(struct operation_list *oplist) {
    int i;
    struct operation *last_auto_op;

    TRACE2_ENTER();
    // list only contains automatically created operations
    if (ACMLIST_COUNT(oplist) <= NUM_AUTOGEN_OPS) {
        TRACE2_EXIT();
        return;
    }

    /* first 3 (NUM_AUTOGEN_OPS) operations have to be kept in the list */
    ACMLIST_LOCK(oplist);
    last_auto_op = ACMLIST_FIRST(oplist);
    for (i = 1; i < NUM_AUTOGEN_OPS; i++)
        last_auto_op = ACMLIST_NEXT(last_auto_op, entry);

    while (ACMLIST_COUNT(oplist) > NUM_AUTOGEN_OPS) {
        struct operation *operation;

        operation = ACMLIST_NEXT(last_auto_op, entry);
        _ACMLIST_REMOVE(oplist, operation, entry);
        operation_destroy(operation);
    }
    ACMLIST_UNLOCK(oplist);
    TRACE2_EXIT();
}

int __must_check operation_list_update_smac(struct operation_list *list, enum acm_module_id id) {
    int ret = 0;
    struct operation *operation;
    uint8_t mac[6];

    TRACE3_ENTER();
    /* read MAC address of module */
    switch (id) {
        case MODULE_0:
            ret = get_mac_address(PORT_MODULE_0, mac);
            break;
        case MODULE_1:
            ret = get_mac_address(PORT_MODULE_1, mac);
            break;
        default:
            ret = -EACMINTERNAL;
    }
    if (ret < 0) {
        LOGERR("Operation: problem reading MAC address of module");
        TRACE3_MSG("Fail");
        return ret;
    }
    /* check if there are operations in the stream, where the MAC address must
     * be updated */
    ACMLIST_LOCK(list);
    ACMLIST_FOREACH(operation, list, entry)
    {
        if (operation->opcode == INSERT_CONSTANT) {
            ret = memcmp(LOCAL_SMAC_CONST, operation->data, ETHER_ADDR_LEN);
            if (ret == 0) {
                /* replace placeholder-mac by mac address of module */
                memcpy(operation->data, (char*) mac, ETHER_ADDR_LEN);
            }
        }
    }
    ACMLIST_UNLOCK(list);

    TRACE3_EXIT();
    return 0;
}
