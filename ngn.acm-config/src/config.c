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

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"
#include "logging.h"
#include "tracing.h"
#include "module.h"
#include "memory.h"
#include "application.h"
#include "libacmconfig_def.h"
#include "validate.h"
#include "sysfs.h"
#include "buffer.h"
#include "status.h"

struct acm_config __must_check* config_create() {
    struct acm_config *config;
    int ret;
    TRACE2_ENTER();
    config = acm_zalloc(sizeof (*config));

    if (!config) {
        LOGERR("Config: Out of memory");
        return NULL;
    }

    config->config_applied = false;
    ret = buffer_init_list(&config->msg_buffs);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return NULL;
    }
    TRACE2_EXIT();
    return config;
}

void config_destroy(struct acm_config *config) {
    TRACE2_ENTER();
    if (!config)
        return;

    for (int i = 0; i < ACM_MODULES_COUNT; i++)
        module_destroy(config->bypass[i]);
    buffer_empty_list(&config->msg_buffs);

    acm_free(config);
    TRACE2_EXIT();
}

int __must_check config_add_module(struct acm_config *config, struct acm_module *module) {
    int ret;

    TRACE2_ENTER();

    if (!config || !module) {
        LOGERR("Config: Invalid module input");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    if (config->config_applied) {
        LOGERR("Config: Configuration already applied to ACM HW");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    if (module->module_id >= ACM_MODULES_COUNT) {
        LOGERR("Config: Invalid module id");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    if (config->bypass[module->module_id] != NULL) {
        LOGERR("Config: Configuration already has a module with this id configured");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    if (module->config_reference != NULL) {
        LOGERR("Config: Module already added to other configuration");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    config->bypass[module->module_id] = module;
    module->config_reference = config;

    ret = validate_config(config, false);
    if (ret != 0) {
        config->bypass[module->module_id] = NULL;
        module->config_reference = config;
    }
    TRACE2_EXIT();
    return ret;
}

int __must_check config_enable(struct acm_config *config, uint32_t identifier) {
    int ret;

    TRACE2_ENTER();
    if (!config) {
        LOGERR("Config: Configuration not defined");
        return -EINVAL;
    }
    // check if configuration identifier is different to 0
    if (identifier == 0) {
        LOGERR("Config: Configuration identifier 0 not allowed");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }
    // final config validation
    ret = validate_config(config, true);
    if (ret) {
        config->config_applied = false;
        LOGERR("Config: final validation before applying config to HW failed");
        TRACE2_MSG("Fail");
        return ret;
    }

    ret = apply_configuration(config, identifier);
    if (ret != 0) {
        LOGERR("Config: applying configuration to HW failed");
        TRACE2_MSG("Fail");
        return ret;
    }
    // write new configuration id to HW
    ret = sysfs_write_configuration_id(identifier);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    config->config_applied = true;

    TRACE2_EXIT();
    return 0;
}

int __must_check config_schedule(struct acm_config *config,
        uint32_t identifier,
        uint32_t identifier_expected) {
    int ret;

    TRACE2_ENTER();
    if (!config) {
        LOGERR("Config: Configuration not defined");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }
    // check if new configuration identifier is different to 0
    if (identifier == 0) {
        LOGERR("Config: Configuration identifier 0 not allowed");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }

    // read identifier from current config on HW
    ret = sysfs_read_configuration_id();
    //check if from HW read identifier is equal to identifier_expected. If not return with error
    if (ret >= 0) {
        if (ret != identifier_expected) {
            LOGERR("Config: read identifier %d not equal expected identifier %d",
                    ret,
                    identifier_expected);
            TRACE2_MSG("Fail");
            return -EINVAL;
        }
    } else {
        TRACE2_MSG("Fail");
        return ret;
    }
    /* if configuration identifier was verified, it can be assumed that there is
     already a configuration applied */

    ret = validate_config(config, true);
    if (ret) {
        LOGERR("Config: final validation before applying schedule to HW failed");
        TRACE2_MSG("Fail");
        return ret;
    }
    ret = apply_schedule(config);
    if (ret != 0) {
        LOGERR("Config: applying schedule to HW failed");
        TRACE2_MSG("Fail");
        return ret;
    }
    // write new configuration id to HW
    ret = sysfs_write_configuration_id(identifier);
    TRACE2_EXIT();
    return ret;
}

int __must_check config_disable(void) {
    return remove_configuration();
}

int __must_check clean_and_recalculate_hw_msg_buffs(struct acm_config *config) {
    int ret, i;

    TRACE3_ENTER();
    /* remove all links from operations to items in buffer_list msg_buffs */
    for (i = 0; i < ACM_MODULES_COUNT; i++) {
        if (config->bypass[i] != NULL) {
            module_clean_msg_buff_links(config->bypass[i]);
        }
    }

    /* remove and delete all items of buffer_list msg_buffs*/
    buffer_empty_list(&config->msg_buffs);
    /* calculate and create buffer_list msg_buffs newly */
    ret = create_hw_msg_buf_list(config);
    TRACE3_EXIT();
    return ret;
}

STATIC uint16_t get_oplen(const struct operation *operation) {
    uint16_t len;

    len = operation->length;
    /*
     * for read operations the message buffer has to be
     * extended by the size of the timestamp which is added
     *  automatically
     */
    if (operation->opcode == READ)
        len += SIZE_TIMESTAMP;

    return len;
}

STATIC int create_hw_msg_buf_list_module(struct acm_module *bypass,
        struct buffer_list *msgbuflist,
        int32_t granularity,
        uint8_t *index,
        uint16_t *offset) {
    int ret = 0;
    uint16_t buffer_size;
    int32_t total_msg_buff_size;
    bool reuse;
    struct acm_stream *stream;
    struct stream_list *streamlist;
    struct operation *operation;
    struct sysfs_buffer *new_msg_buf, *reuse_msg_buff;

    /* skip if no module is referenced */
    if (!bypass)
        return 0;

    // iterate through all streams
    streamlist = &bypass->streams;
    ACMLIST_LOCK(&bypass->streams);
    ACMLIST_FOREACH(stream, streamlist, entry) {
        struct operation_list *oplist = &stream->operations;

        ACMLIST_LOCK(oplist);
        ACMLIST_FOREACH(operation, oplist, entry) {
            if ((operation->opcode != READ) &&
                (operation->opcode != INSERT))
                continue;
                // create hw msg buff item
            buffer_size = get_oplen(operation) / granularity;
            if ((get_oplen(operation) % granularity) > 0) {
                buffer_size++;
            }
            new_msg_buf = buffer_create(operation, *index,
                    *offset, buffer_size);
            if (!new_msg_buf) {
                // no item created
                ret = -ENOMEM;
                break;
            }
            reuse = false;
            ret = buffername_check(msgbuflist, new_msg_buf, &reuse, &reuse_msg_buff);
            if (ret < 0) {
                /* buffer name not okay */
                buffer_destroy(new_msg_buf);
                break;
            }
            if (reuse) {
                // adapt buffer size of reuse_msg_buff and offset of all following msg_buffs in queue
                if (new_msg_buf->buff_size > reuse_msg_buff->buff_size) {
                    *offset += (new_msg_buf->buff_size - reuse_msg_buff->buff_size);
                    reuse_msg_buff->buff_size = new_msg_buf->buff_size;
                    update_offset_after_buffer(msgbuflist, reuse_msg_buff);
                }
                buffer_destroy(new_msg_buf);
                // set link of operation item to hw msg buff item
                operation->msg_buf  = reuse_msg_buff;
            } else {
                (*index)++;
                *offset += buffer_size;
                // insert new hw msg buff item into list
                buffer_add_list(msgbuflist, new_msg_buf);
                // set link of operation item to hw msg buff item
                operation->msg_buf = new_msg_buf;
            }
        }
        ACMLIST_UNLOCK(oplist);
        if (ret < 0)
            break;
    }
    ACMLIST_UNLOCK(&bypass->streams);
    /* check if size of all message buffers is < 16kbyte, do check
     * only if no problem occurred so far */

    total_msg_buff_size = get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_SIZE));
    if (ret == 0) {
        if ((*offset + 1) * granularity >= total_msg_buff_size) {
            LOGERR("Config: configured message buffers %d bigger than available %d",
                    (*offset + 1) * granularity, total_msg_buff_size);
            ret = -EPERM;
        }
    }

    return ret;
}

int __must_check create_hw_msg_buf_list(struct acm_config *config) {
    int ret = 0;
    int i;
    const int32_t msg_buf_granularity =
            get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH));
    uint8_t buffer_index = 0;
    uint16_t buffer_offset = 0;

    TRACE3_ENTER();

    if (msg_buf_granularity <= 0) {
        LOGERR("Config: read size of message buffer blocks is invalid: %d",
                msg_buf_granularity);
        ret = -ENODEV;
    }

    /* calculate and create buffer_list msg_buffs newly */
    for (i = 0; (ret == 0) && (i < ACM_MODULES_COUNT); ++i)
        ret = create_hw_msg_buf_list_module(config->bypass[i], &config->msg_buffs,
                msg_buf_granularity, &buffer_index, &buffer_offset);

    TRACE3_MSG("return value = %d", ret);
    TRACE3_EXIT();
    return ret;
}
