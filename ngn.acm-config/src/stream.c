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
#include <net/ethernet.h> /* for ETHER_ADDR_LEN */
#include <string.h>

#include "stream.h"
#include "logging.h"
#include "tracing.h"
#include "memory.h"
#include "lookup.h"
#include "tracing.h"
#include "module.h"
#include "schedule.h"
#include "operation.h"
#include "validate.h"
#include "buffer.h"
#include "sysfs.h"
#include "status.h"

struct acm_stream __must_check* stream_create(enum stream_type type) {
    struct acm_stream *stream;
    int ret;

    TRACE2_MSG("Called. type=%d", type);
    if (type >= MAX_STREAM_TYPE) {
        LOGERR("Stream: Invalid type");
        return NULL;
    }

    stream = acm_zalloc(sizeof (*stream));

    if (!stream) {
        LOGERR("Stream: Out of memory");
        return NULL;
    }

    stream->type = type;
    stream->reference = NULL;
    stream->reference_parent = NULL;
    stream->reference_redundant = NULL;
    ret = operation_list_init(&stream->operations);
    if (ret != 0) {
        LOGERR("Stream: Could not initialize operation list");
        acm_free(stream);
        return NULL;
    }

    ret = schedule_list_init(&stream->windows);
    if (ret != 0) {
        LOGERR("Stream: Could not initialize schedule list");
        operation_list_flush(&stream->operations);
        acm_free(stream);
        return NULL;
    }
    TRACE2_EXIT();
    return stream;
}

void stream_delete(struct acm_stream *stream) {
    TRACE3_ENTER();
    if (!stream)
        return;
    if (ACMLIST_REF(stream, entry)) {
        LOGERR("Stream: Destroy not possible - added to module");
        return;
    }
    if ( ( (stream->type == EVENT_STREAM) || (stream->type == RECOVERY_STREAM))
            && (stream->reference_parent)) {
        /* Recovery Stream has reference to parent Event Stream or Event Stream has
         * reference to parent Ingress Triggered Stream */
        LOGERR("Stream: Destroy not possible - type equal Event or Recovery Stream and reference exists");
        return;
    }

    if (stream->reference) {
        /* Stream is an Ingress Triggered Stream or an Event Stream and has one or two children.
         * Children must be deleted first */
        stream->reference->reference_parent = NULL;
        stream_delete(stream->reference);
    }

    if (stream->reference_redundant) {
        /* Other Redundant Stream shall be kept - just remove reference to stream which shall
         * be destroyed */
        stream->reference_redundant->type = TIME_TRIGGERED_STREAM;
        stream->reference_redundant->reference_redundant = NULL;
    }
    stream_destroy(stream);
    TRACE3_EXIT();
}

void stream_destroy(struct acm_stream *stream) {
    TRACE3_ENTER();
    if (!stream)
        return;

    operation_list_flush(&stream->operations);
    schedule_list_flush(&stream->windows);
    lookup_destroy(stream->lookup);

    acm_free(stream);
    TRACE3_EXIT();
}

void stream_clean_msg_buff_links(struct acm_stream *stream) {
    struct operation_list *oplist;
    struct operation *operation;

    TRACE3_ENTER();
    if (!stream) {
        return;   // not an error, but no operations with message buffers
    }
    oplist = &stream->operations;
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if ( (operation->opcode == READ) || (operation->opcode == INSERT)) {
            operation->msg_buf = NULL;
        }
    }
    ACMLIST_UNLOCK(oplist);
    TRACE3_EXIT();
    return;
}

int __must_check stream_add_operation(struct acm_stream *stream, struct operation *operation) {
    int ret;

    TRACE3_ENTER();
    if (!stream || !operation) {
        LOGERR("Stream: stream or operation is null in %s", __func__);
        return -EINVAL;
    }
    TRACE3_MSG("adding opcode %d to stream-type %d", operation->opcode, stream->type);
    switch (stream->type) {
        case REDUNDANT_STREAM_TX:
        case TIME_TRIGGERED_STREAM:
        case RECOVERY_STREAM:
            if ( (operation->opcode == FORWARD) || (operation->opcode == READ)
                    || (operation->opcode == FORWARD_ALL)) {
                LOGERR("Stream: Cannot add operation to stream");
                return -EINVAL;
            }
            break;
        case EVENT_STREAM:
            if ( (operation->opcode == READ) || (operation->opcode == FORWARD_ALL)) {
                LOGERR("Stream: Cannot add operation to stream");
                return -EINVAL;
            }
            break;
        case INGRESS_TRIGGERED_STREAM:
            if ( (operation->opcode != FORWARD_ALL) && (operation->opcode != READ)) {
                LOGERR("Stream: Cannot add operation to stream");
                return -EINVAL;
            }
            if ( (operation->opcode == FORWARD_ALL) && (stream->reference)) {
                /* Ingress Triggered Stream has already an Event Stream as child */
                LOGERR("Stream: operation FORWARD_ALL not allowed, has an Event Stream");
                return -EPERM;
            }
            if (operation->opcode == FORWARD_ALL) {
                bool b_ret;
                /* check if stream already has a FORWARD_ALL operation */
                b_ret = stream_has_operation_x(stream, operation->opcode);
                if (b_ret) {
                    LOGERR("Stream: operation FORWARD_ALL not possible, has already FORWARD_ALL operation");
                    return -EPERM;
                }
            }
            break;
        case REDUNDANT_STREAM_RX:
            if (operation->opcode != READ) {
                LOGERR("Stream: Cannot add operation to stream. REDUNDANT_STREAM_RX allows only READ operations");
                return -EINVAL;
            }
            break;
        default:
            LOGERR("Stream: Invalid stream type");
            return -EINVAL;
    }

    ret = operation_list_add_operation(&stream->operations, operation);

    TRACE3_MSG("Exit with code ret=%d", ret);
    return ret;
}

int __must_check stream_set_reference(struct acm_stream *stream, struct acm_stream *reference) {
    int ret;
    struct stream_list *streamlist = NULL;

    TRACE2_ENTER();
    if (!stream || !reference) {
        LOGERR("Stream: stream or stream reference is null");
        return -EINVAL;
    }
    TRACE2_MSG("setting reference between type %d and %d", stream->type, reference->type);
    if (stream->reference || reference->reference_parent || stream->reference_redundant
            || reference->reference_redundant) {
        LOGERR("Stream: stream or reference already referenced");
        return -EINVAL;
    }
    /* check if stream was already applied to HW */
    if (stream_config_applied(stream)) {
        LOGERR("Stream: configuration of stream already applied to HW");
        return -EPERM;
    }
    if (ACMLIST_REF(stream, entry)) {
        streamlist = ACMLIST_REF(stream, entry);
    }

    if (stream->type == INGRESS_TRIGGERED_STREAM && reference->type == EVENT_STREAM) {
        /* check if Ingress Triggered Stream already contains a Forward Operation */
        if (stream_has_operation_x(stream, FORWARD_ALL)) {
            LOGERR("Stream: Ingress Triggered Stream already contains a ForwardAll Operation");
            return -EINVAL;
        }
    }
    if (stream->type == INGRESS_TRIGGERED_STREAM && reference->type == INGRESS_TRIGGERED_STREAM) {
        /* check if Ingress Triggered Stream already contains a Forward Operation */
        if ( (stream_has_operation_x(stream, FORWARD_ALL))
                || (stream_has_operation_x(reference, FORWARD_ALL))) {
            LOGERR("Stream: One Ingress Triggered Stream already contains a ForwardAll Operation");
            return -EINVAL;
        }
        /* check if G_RX_REDUNDANCY_EN is set */
        ret = get_int32_status_value(__stringify(ACM_SYSFS_RX_REDUNDANCY));
        if (ret != RX_REDUNDANCY_SET) {
            LOGERR("Stream: RX redundancy not set. Has value %d", ret);
            return -EINVAL;
        }
    }

    if ( (stream->type == EVENT_STREAM && reference->type == RECOVERY_STREAM)
            || (stream->type == INGRESS_TRIGGERED_STREAM && reference->type == EVENT_STREAM)) {
        stream->reference = reference;
        reference->reference_parent = stream;
        /* add reference stream to streamlist if possible */
        if (streamlist) {
            ret = stream_add_list(streamlist, stream->reference);
            if (ret != 0) {
                stream->reference = NULL;
                reference->reference_parent = NULL;
                return ret;
            }
        }
    } else if ( (stream->type == TIME_TRIGGERED_STREAM &&
            reference->type == TIME_TRIGGERED_STREAM)) {
        stream->type = REDUNDANT_STREAM_TX;
        reference->type = REDUNDANT_STREAM_TX;
        stream->reference_redundant = reference;
        reference->reference_redundant = stream;
    } else if ( (stream->type == INGRESS_TRIGGERED_STREAM
            && reference->type == INGRESS_TRIGGERED_STREAM)) {
        stream->type = REDUNDANT_STREAM_RX;
        reference->type = REDUNDANT_STREAM_RX;
        stream->reference_redundant = reference;
        reference->reference_redundant = stream;
    } else {
        LOGERR("Stream: Cannot set stream reference");
        return -EINVAL;
    }

    // validate if setting the reference still results in valid configuration
    ret = validate_stream(stream, false);
    if (ret != 0) {
        // undo reference setting because validation failed
        if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == EVENT_STREAM)) {
            stream->reference = NULL;
            reference->reference_parent = NULL;
            /* also remove reference stream from streamlist if added before */
            if (streamlist) {
                stream_remove_list(streamlist, reference);
            }
        } else if ( (stream->type == REDUNDANT_STREAM_TX)
                && (reference->type == REDUNDANT_STREAM_TX)) {
            // stream is Time Triggered/Redundant Stream TX
            stream->type = TIME_TRIGGERED_STREAM;
            reference->type = TIME_TRIGGERED_STREAM;
            stream->reference_redundant = NULL;
            reference->reference_redundant = NULL;
        } else {
            // stream is Ingress Triggered/Redundant Stream RX
            stream->type = INGRESS_TRIGGERED_STREAM;
            reference->type = INGRESS_TRIGGERED_STREAM;
            stream->reference_redundant = NULL;
            reference->reference_redundant = NULL;
        }
        LOGERR("Stream: Cannot set stream reference - validation failed");
        return -EINVAL;
    }
    /* validation was okay, Update Indexes for HW tables: indexes are already
     * updated for parent/child references in stream_add_list. For redundant
     * streams indexes have to be updated here:
     * - recalculate redundancy indexes as necessary as there are additional
     * 		items
     * - recalculate gather indexes for REDUNDANT_STREAM_TX, because for these
     * 		streams the additional R-Tag command in gather table must be
     * 		considered. */
    if ((ACMLIST_REF(stream, entry))
            && ( (stream->type == REDUNDANT_STREAM_TX) || (stream->type == REDUNDANT_STREAM_RX))) {
        calculate_redundancy_indizes(ACMLIST_REF(stream, entry));
        if (stream->type == REDUNDANT_STREAM_TX) {
            calculate_gather_indizes(ACMLIST_REF(stream, entry));
        }
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check stream_set_egress_header(struct acm_stream *stream,
        uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan,
        uint8_t prio) {
    uint8_t empty_mac[ETHER_ADDR_LEN] = { 0, 0, 0, 0, 0, 0 };
    uint8_t set_mac[ETHER_ADDR_LEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    uint8_t vlan_tag[4] = { 0x81, 0x00, 0x00, 0x00 };
    struct operation *operation_dmac = 0;
    struct operation *operation_smac = 0;
    struct operation *operation_vlan = 0;
    uint16_t help;
    uint16_t id_mask = 0x0fff;
    int ret;

    TRACE2_ENTER();
    /* structure of vlan tag: 2 bytes with value 0x8100, 3 bit vlan prio,
     * 1 bit with value zero, 12 bit vlan id */
    help = (prio) << 13;
    help = help | (vlan & id_mask);
    vlan_tag[3] = (uint8_t) help;
    vlan_tag[2] = (uint8_t) (help >> 8);

    if (!stream) {
        LOGERR("Stream: parameter stream is NULL in %s", __func__);
        return -EINVAL;
    }

    if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == REDUNDANT_STREAM_RX)) {
        LOGERR("Stream: operation not supported for this stream-type");
        return -EINVAL;
    }

    //DMAC
    if ( (memcmp(dmac, set_mac, ETHER_ADDR_LEN) == 0) && (stream->type == EVENT_STREAM)) {

        /* create forward operation, that forwards destination MAC address
         * from the incoming frame */
        operation_dmac = operation_create_forward(OFFSET_DEST_MAC_IN_FRAME,
        ETHER_ADDR_LEN);
    } else
        operation_dmac = operation_create_insertconstant((const char*) dmac,
        ETHER_ADDR_LEN);
    ret = stream_add_operation(stream, operation_dmac);
    if (ret < 0) {
        acm_free(operation_dmac);
        return ret;
    }

    //SMAC
    if ( (memcmp(smac, set_mac, ETHER_ADDR_LEN) == 0) && (stream->type == EVENT_STREAM)) {
        /* create forward operation, that forwards source MAC address
         * from the incoming frame */
        operation_smac = operation_create_forward(OFFSET_SOURCE_MAC_IN_FRAME,
        ETHER_ADDR_LEN);
    } else {
        /* if smac is equal to 00:00:00:00:00:00 then the mac source address
         * has to be set to the mac address of the associated port. The
         * associated port can be determined only after the stream was added
         * to a module. Therefore at this point a placeholder is inserted.
         * The placeholder is replaced by the correct value when stream is
         * added to a module */
        if (memcmp(smac, empty_mac, ETHER_ADDR_LEN) == 0) {
            operation_smac = operation_create_insertconstant(LOCAL_SMAC_CONST,
            ETHER_ADDR_LEN);
        } else {
            operation_smac = operation_create_insertconstant((const char*) smac, ETHER_ADDR_LEN);
        }
    }
    ret = stream_add_operation(stream, operation_smac);
    if (ret < 0) {
        /* take out operations of operation_list and release memory */
        operation_list_remove_operation(&stream->operations, operation_dmac);
        acm_free(operation_dmac);
        acm_free(operation_smac);
        return ret;
    }

    //VLAN
    if (vlan == ACM_VLAN_ID_MAX) {
        if (stream->type == EVENT_STREAM) {
            /* configure forward operation, which forwards vlan tag from
             * incoming frame */
            operation_vlan = operation_create_forward(OFFSET_VLAN_TAG_IN_FRAME, sizeof (vlan_tag));
        } else {
            LOGERR("Stream: No VLAN-ID defined");
            /* take out operations of operation_list and release memory */
            operation_list_remove_operation(&stream->operations, operation_dmac);
            operation_list_remove_operation(&stream->operations, operation_smac);
            acm_free(operation_dmac);
            acm_free(operation_smac);
            return -EINVAL;
        }
    } else
        operation_vlan = operation_create_insertconstant((char*) &vlan_tag, sizeof (vlan_tag));
    ret = stream_add_operation(stream, operation_vlan);
    if (ret < 0) {
        /* take out operations of operation_list and release memory */
        operation_list_remove_operation(&stream->operations, operation_dmac);
        operation_list_remove_operation(&stream->operations, operation_smac);
        acm_free(operation_dmac);
        acm_free(operation_smac);
        acm_free(operation_vlan);
        return ret;
    }

    TRACE2_EXIT();
    return 0;
}

int __must_check stream_add_list(struct stream_list *stream_list, struct acm_stream *stream) {
    int ret;

    TRACE2_ENTER();
    if (!stream_list || !stream) {
        LOGERR("Stream: stream or streamlist is NULL in %s", __func__);
        return -EINVAL;
    }

    if (ACMLIST_REF(stream, entry)) {
        LOGERR("Stream: Cannot be added a second time");
        return -EPERM;
    }

    ACMLIST_INSERT_TAIL(stream_list, stream, entry);
    ACMLIST_REF(stream, entry) = stream_list;

    /* as stream is added to module now, it is possible to find out the port
     * associated to the module, which may be important for some operations
     * of the stream. */
    ret = operation_list_update_smac(&stream->operations,
            streamlist_to_module(stream_list)->module_id);
    if (ret != 0) {
        stream_remove_list(stream_list, stream);
        return ret;
    }

    ret = validate_stream_list(stream_list, false);
    if (ret != 0) {
        stream_remove_list(stream_list, stream);
        return ret;
    }

    /* Update Indizes for HW tables. */
    ret = calculate_indizes_for_HW_tables(stream_list, stream);
    if (ret != 0) {
        stream_remove_list(stream_list, stream);
        TRACE2_MSG("Fail");
        return ret;
    }

    TRACE2_EXIT();
    return 0;
}

void stream_empty_list(struct stream_list *stream_list) {
    TRACE2_ENTER();
    if (!stream_list)
        return;

    ACMLIST_LOCK(stream_list);

    while (!ACMLIST_EMPTY(stream_list)) {
        struct acm_stream *stream;

        stream = ACMLIST_FIRST(stream_list);
        _ACMLIST_REMOVE(stream_list, stream, entry);

        stream_destroy(stream);
    }

    ACMLIST_UNLOCK(stream_list);
    TRACE2_EXIT();
}

void stream_remove_list(struct stream_list *stream_list, struct acm_stream *stream) {

    TRACE3_ENTER();
    if (stream_in_list(stream_list, stream)) {
        ACMLIST_REMOVE(stream_list, stream, entry);
        ACMLIST_REF(stream, entry) = NULL;
    }
    TRACE3_EXIT();
    return;
}

static bool out_of_range(int val, int min, int max) {
    return (val < min) || (val > max);
}

int __must_check stream_check_vlan_parameter(uint16_t vlan_id, uint8_t vlan_prio) {

    TRACE3_ENTER();
    if (out_of_range(vlan_id, ACM_VLAN_ID_MIN, ACM_VLAN_ID_MAX)) {
        LOGERR("Stream: VLAN id out of range.");
        return -EINVAL;
    }
    if (out_of_range(vlan_prio, ACM_VLAN_PRIO_MIN, ACM_VLAN_PRIO_MAX)) {
        LOGERR("Stream: VLAN priority out of range.");
        return -EINVAL;
    }

    TRACE3_EXIT();
    return 0;
}

bool __must_check stream_config_applied(struct acm_stream *stream) {
    struct acm_module *module;
    struct stream_list *streamlist;
    struct acm_config *configuration;

    TRACE3_ENTER();
    if (!stream)
        goto out;

    streamlist = ACMLIST_REF(stream, entry);
    if (!streamlist)
        goto out;

    module = streamlist_to_module(streamlist);

    configuration = module->config_reference;
    if (configuration) {
        TRACE3_EXIT();
        return configuration->config_applied;
    }

out:
    TRACE3_MSG("Fail");
    return false; // as stream not added to config, config isn't applied
}

bool __must_check stream_has_operation_x(struct acm_stream *stream, enum acm_operation_code opcode) {
    bool b_ret = false;
    struct operation_list *oplist;
    struct operation *operation;

    TRACE3_ENTER();
    if (stream) {
        oplist = & (stream->operations);
    } else {
        return b_ret;
    }

    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if (operation->opcode == opcode) {
            b_ret = true;
            break;
        }
    }
    ACMLIST_UNLOCK(oplist);
    TRACE3_EXIT();
    return b_ret;
}

int __must_check stream_num_operations_x(struct acm_stream *stream, enum acm_operation_code opcode) {
    uint ret = 0;
    struct operation_list *oplist;
    struct operation *operation;

    TRACE3_ENTER();
    oplist = & (stream->operations);
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if (operation->opcode == opcode) {
            ret++;
        }
    }
    ACMLIST_UNLOCK(oplist);
    TRACE3_EXIT();
    return ret;
}

int __must_check calculate_indizes_for_HW_tables(struct stream_list *stream_list,
        struct acm_stream *stream) {
    int ret = 0;

    TRACE3_ENTER();
    switch (stream->type) {
        case INGRESS_TRIGGERED_STREAM:
            calculate_lookup_indizes(stream_list);
            calculate_scatter_indizes(stream_list);
            calculate_gather_indizes(stream_list);
            break;
        case REDUNDANT_STREAM_RX:
            /* only read operation is allowed in redundant stream RX, thus
             * calculation of gather indexes like for ingress triggered streams
             * isn't done. */
            calculate_lookup_indizes(stream_list);
            calculate_redundancy_indizes(stream_list);
            calculate_scatter_indizes(stream_list);
            break;
        case TIME_TRIGGERED_STREAM:
        case REDUNDANT_STREAM_TX:
            calculate_redundancy_indizes(stream_list);
            calculate_gather_indizes(stream_list);
            break;
        case EVENT_STREAM:
            calculate_gather_indizes(stream_list);
            break;
        case RECOVERY_STREAM:
            calculate_gather_indizes(stream_list);
            break;
        default:
            ret = -EACMINTERNAL;
            LOGERR("Stream: stream without a stream type ");
    }
    TRACE3_EXIT();
    return ret;
}

void calculate_lookup_indizes(struct stream_list *stream_list) {
    uint8_t lookup_index = LOOKUP_START_IDX;
    struct acm_stream *stream;

    TRACE3_ENTER();
    if (!stream_list)
        return;  // nothing to do
    /* lookup table size is not checked here, but in validate_module */
    ACMLIST_LOCK(stream_list);
    ACMLIST_FOREACH(stream, stream_list, entry)
    {
        if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == REDUNDANT_STREAM_RX)) {
            stream->lookup_index = lookup_index;
            lookup_index++;
        } else {
            stream->lookup_index = 0;
        }
    }
    ACMLIST_UNLOCK(stream_list);
    TRACE3_EXIT();
    return;
}

void calculate_redundancy_indizes(struct stream_list *stream_list) {
    uint8_t redund_index = REDUNDANCY_START_IDX;
    struct acm_stream *stream;

    TRACE3_ENTER();
    if (!stream_list) {
        TRACE3_EXIT();
        return;  // nothing to do
    }
    /* redundancy table size is not checked here, but in validate_module */
    /* the redundancy indexes of two redundant streams must be equal. Therefore
     * the index of both streams is set at the same time */
    ACMLIST_LOCK(stream_list);
    ACMLIST_FOREACH(stream, stream_list, entry)
    {
        if ( (stream->type == REDUNDANT_STREAM_TX) || (stream->type == REDUNDANT_STREAM_RX)) {
            stream->redundand_index = redund_index;
            stream->reference_redundant->redundand_index = redund_index;
            redund_index++;
        } else {
            stream->redundand_index = 0;
        }
    }
    ACMLIST_UNLOCK(stream_list);
    TRACE3_EXIT();
    return;
}

void calculate_gather_indizes(struct stream_list *stream_list) {
    uint16_t gather_dma_index = GATHER_START_IDX;
    struct acm_stream *stream;

    TRACE3_ENTER();
    if (!stream_list)
        return;  // nothing to do
    /* gather DMA table size is not checked here, but in validate_module */
    ACMLIST_LOCK(stream_list);
    ACMLIST_FOREACH(stream, stream_list, entry)
    {
        uint16_t num_ops, num_prefetch_commands;

        num_ops = stream_num_gather_ops(stream);
        num_prefetch_commands = stream_num_prefetch_ops(stream);
        if (num_ops == 0) {
            stream->gather_dma_index = GATHER_NOP_IDX;
        } else {
            if ( (num_ops == 1) && (stream_has_operation_x(stream, FORWARD_ALL))) {
                stream->gather_dma_index = GATHER_FORWARD_IDX;
            } else {
                stream->gather_dma_index = gather_dma_index;
                if (num_ops >= num_prefetch_commands) {
                    gather_dma_index = gather_dma_index + num_ops;
                } else {
                    gather_dma_index = gather_dma_index + num_prefetch_commands;
                }
            }
        }
        TRACE3_MSG("Stream type %d has index %d", stream->type, stream->gather_dma_index);
    }
    ACMLIST_UNLOCK(stream_list);
    TRACE3_EXIT();
    return;
}

void calculate_scatter_indizes(struct stream_list *stream_list) {
    uint16_t scatter_dma_index = SCATTER_START_IDX;
    uint16_t num_ops;
    struct acm_stream *stream;

    TRACE3_ENTER();
    if (!stream_list)
        return;  // nothing to do
    /* scatter DMA table size is not checked here, but in validate_module */
    ACMLIST_LOCK(stream_list);
    ACMLIST_FOREACH(stream, stream_list, entry)
    {
        if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == REDUNDANT_STREAM_RX)) {
            num_ops = stream_num_scatter_ops(stream);
            if (num_ops == 0) {
                stream->scatter_dma_index = SCATTER_NOP_IDX;
            } else {
                stream->scatter_dma_index = scatter_dma_index;
                scatter_dma_index = scatter_dma_index + num_ops;
            }
        } else {
            stream->scatter_dma_index = SCATTER_NOP_IDX;
        }
    }
    ACMLIST_UNLOCK(stream_list);
    TRACE3_EXIT();
    return;
}

bool __must_check stream_in_list(struct stream_list *stream_list, struct acm_stream *stream) {
    struct acm_stream *stream_item;
    bool ret = false;

    TRACE3_ENTER();
    if (!stream_list)
        return ret;  // nothing to do
    ACMLIST_LOCK(stream_list);
    ACMLIST_FOREACH(stream_item, stream_list, entry)
    {
        if (stream_item == stream) {
            /* stream found in queue */
            ret = true;
            break;
        }
    }
    ACMLIST_UNLOCK(stream_list);
    TRACE3_EXIT();

    return ret;
}

int __must_check stream_num_gather_ops(struct acm_stream *stream) {
    int num_egress_ops;
    struct operation *operation;
    struct operation_list *oplist;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0;   // as there is no stream number of egress operations if stream is NULL
    }
    num_egress_ops = 0;
    oplist = &stream->operations;
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if ( (operation->opcode == INSERT) || (operation->opcode == INSERT_CONSTANT)
                || (operation->opcode == PAD) || (operation->opcode == FORWARD)
                || (operation->opcode == FORWARD_ALL)) {
            num_egress_ops++;
        }
    }
    ACMLIST_UNLOCK(oplist);
    if (stream->type == REDUNDANT_STREAM_TX) {
        /* in case of a redundant stream an extra R-Tag command must be
         * inserted into gather table - thus increase num_egress_ops by 1*/
        num_egress_ops++;
    }
    TRACE3_MSG("number of egress operations is %d", num_egress_ops);
    TRACE3_EXIT();
    return num_egress_ops;
}

int __must_check stream_num_scatter_ops(struct acm_stream *stream) {
    int num_ingress_ops;
    struct operation *operation;
    struct operation_list *oplist;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0;   // as there is no stream number of ingress operations of stream is 0
    }
    num_ingress_ops = 0;
    oplist = &stream->operations;
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if (operation->opcode == READ) {
            num_ingress_ops++;
        }
    }
    ACMLIST_UNLOCK(oplist);
    TRACE3_MSG("number of ingress operations %d", num_ingress_ops);
    TRACE3_EXIT();
    return num_ingress_ops;
}

int __must_check stream_num_prefetch_ops(struct acm_stream *stream) {
    int num_prefetch_commands;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0;   // as there is no stream number of ingress operations of stream is 0
    }

    num_prefetch_commands = stream_num_operations_x(stream, INSERT);
    /* If the stream has an insert operation, then, the prefetch
     * lock vector must be written (has 4 prefetch commands).
     * Otherwise only one prefetch NOP command is written. */
    if (num_prefetch_commands > 0) {
        num_prefetch_commands = num_prefetch_commands +
        NUM_PREFETCH_LOCK_COMMANDS;
    } else {
        num_prefetch_commands++;
    }

    TRACE3_MSG("number of prefetch operations %d", num_prefetch_commands);
    TRACE3_EXIT();
    return num_prefetch_commands;
}

int __must_check stream_set_indiv_recov(struct acm_stream *stream, uint32_t timeout_ms) {

    TRACE2_ENTER();
    if (!stream) {
        LOGERR("Stream: stream is null");
        return -EINVAL;
    }
    /* check if stream was already applied to HW */
    if (stream_config_applied(stream)) {
        LOGERR("Stream: configuration of stream already applied to HW");
        return -EPERM;
    }

    if ( (stream->type != INGRESS_TRIGGERED_STREAM) && (stream->type != REDUNDANT_STREAM_RX)) {
        LOGERR("Stream: only allowed for Ingress Triggered streams. is %d", stream->type);
        return -EPERM;
    }

    stream->indiv_recov_timeout_ms = timeout_ms;

    TRACE2_EXIT();
    return 0;
}

