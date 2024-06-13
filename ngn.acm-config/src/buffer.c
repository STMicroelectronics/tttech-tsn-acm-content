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

#include "buffer.h"
#include "logging.h"
#include "tracing.h"
#include "stream.h"
#include "memory.h"
#include "libacmconfig_def.h"
#include "validate.h"


struct sysfs_buffer __must_check* buffer_create(struct operation *operation,
        uint8_t buffer_index,
        uint16_t buffer_offset,
        uint16_t buffer_size) {
    struct sysfs_buffer *msg_buf;
    TRACE3_ENTER();
    if (!operation) {
        LOGERR("Buffer: pointer to operation is null");
        TRACE3_MSG("Fail");
        return NULL;
    }
    /* Check input values: just check opcode. All other values are checked in operations.c */
    if (operation->opcode != READ && operation->opcode != INSERT) {
        LOGERR("Buffer: Wrong operation code");
        TRACE3_MSG("Fail");
        return NULL;
    }

    msg_buf = acm_zalloc(sizeof (*msg_buf));

    if (!msg_buf) {
        LOGERR("Buffer: Out of memory");
        TRACE3_MSG("Fail");
        return NULL;
    }

    msg_buf->msg_buff_name = acm_strdup(operation->buffer_name);
    if (!msg_buf->msg_buff_name) {
        LOGERR("Buffer: Problem when copying buffer name. errno: %d", errno);
        TRACE3_MSG("Fail");
        acm_free(msg_buf);
        return NULL;
    }

    msg_buf->msg_buff_index = buffer_index;
    msg_buf->msg_buff_offset = buffer_offset;
    msg_buf->reset = false;
    if (operation->opcode == READ) {
        msg_buf->stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
    } else {
        msg_buf->stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
    }
    msg_buf->buff_size = buffer_size;
    msg_buf->timestamp = true;
    msg_buf->valid = true;

    TRACE3_EXIT();
    return msg_buf;
}

void buffer_destroy(struct sysfs_buffer *message_buffer) {
    TRACE3_MSG("Executing");
    if (!message_buffer)
        return;

    acm_free(message_buffer->msg_buff_name);
    acm_free(message_buffer);

}

int __must_check buffer_init_list(struct buffer_list *buffer_list) {
    TRACE3_MSG("Executing");
    if (!buffer_list)
        return -EINVAL;
    ACMLIST_INIT(buffer_list);
    return 0;
}

void buffer_add_list(struct buffer_list *buffer_list, struct sysfs_buffer *buffer) {
    if (!buffer_list || !buffer) {
        LOGERR("Buffer: Pointer to buffer_list or buffer is null");
        return;
    }
    TRACE3_MSG("Adding buffer %s to config-list", buffer->msg_buff_name);
    ACMLIST_INSERT_TAIL(buffer_list, buffer, entry);
}

void buffer_empty_list(struct buffer_list *bufferlist) {
    TRACE3_MSG("Executing");
    if (!bufferlist)
        return;

    ACMLIST_LOCK(bufferlist);

    while (!ACMLIST_EMPTY(bufferlist)) {
        struct sysfs_buffer *msg_buf;

        msg_buf = ACMLIST_FIRST(bufferlist);
        _ACMLIST_REMOVE(bufferlist, msg_buf, entry);

        buffer_destroy(msg_buf);
    }

    ACMLIST_UNLOCK(bufferlist);
}

void update_offset_after_buffer(struct buffer_list *bufferlist, struct sysfs_buffer *message_buffer) {
    struct sysfs_buffer *buffer;
    bool buffer_found = false;
    uint16_t changed_offset = 0;

    TRACE2_ENTER();
    if (!bufferlist || !message_buffer) {
        LOGERR("Buffer: Pointer to buffer_list %d or message_buffer %d is null",
                bufferlist,
                message_buffer);
        return;
    }
    ACMLIST_LOCK(bufferlist);
    ACMLIST_FOREACH(buffer, bufferlist, entry)
    {
        if (buffer_found) {
            /* correct buffer offset */
            buffer->msg_buff_offset = changed_offset;
            changed_offset = changed_offset + buffer->buff_size;
        } else {
            /* check if actual buffer is the one we are looking for */
            if (buffer == message_buffer) {
                buffer_found = true;
                changed_offset = buffer->msg_buff_offset + buffer->buff_size;
            }
        }
    }
    ACMLIST_UNLOCK(bufferlist);
    TRACE2_EXIT();
    return;
}
