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

#include <string.h>

#include "logging.h"
#include "tracing.h"
#include "validate.h"
#include "hwconfig_def.h"
#include "sysfs.h"
#include "status.h"

int __must_check validate_stream(struct acm_stream *stream, bool final_validate) {
    int ret;
    struct acm_config *config_stream, *config_reference;
    struct acm_module *module_stream, *module_reference;

    TRACE2_ENTER();
    TRACE2_MSG("final_validate=%d", final_validate);
    if (!stream) {
        LOGERR("Validate: no stream as input");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }
    if (final_validate) {
        /* each egress streams of the config needs a frame size of
         at least 64 bytes */
        if ( (stream->type == TIME_TRIGGERED_STREAM) || (stream->type == EVENT_STREAM)
                || (stream->type == RECOVERY_STREAM) || (stream->type == REDUNDANT_STREAM_TX)) {
            int egress_framesize;

            /* check the egress_framesize of the stream*/
            egress_framesize = calc_stream_egress_framesize(stream);
            if (egress_framesize < ACM_MIN_FRAME_SIZE) {
                LOGERR("Validate: frame size of egress operations < %d", ACM_MIN_FRAME_SIZE);
                TRACE2_MSG("Fail");
                return -EACMEGRESSFRAMESIZE;
            }
        }
    }

    // tests for a stream which have to be done for final and non final validation
    config_stream = NULL;
    module_stream = NULL;
    config_reference = NULL;
    module_reference = NULL;
    if (ACMLIST_REF(stream, entry)) {
        module_stream = streamlist_to_module(ACMLIST_REF(stream, entry));
        config_stream = module_stream->config_reference;
    }

    /* check if stream is a redundant stream and if referenced stream was added
     to same configuration */
    if ( (stream->type == REDUNDANT_STREAM_TX) || (stream->type == REDUNDANT_STREAM_RX)) {
        if (stream->reference_redundant) {
            if (ACMLIST_REF(stream->reference_redundant, entry)) {
                module_reference = streamlist_to_module(ACMLIST_REF(stream->reference_redundant,
                        entry));
                config_reference = module_reference->config_reference;
            }
        }
        if ( (config_stream != NULL) && (config_reference != NULL)) {
            if (config_stream != config_reference) {
                // two redundant streams are added to different configurations
                LOGERR("Validate: two redundant streams are added to different configurations");
                TRACE2_MSG("Fail");
                return -EACMDIFFCONFIG;
            }
        } else {
            /* at final validation both redundant streams must be added to a config */
            if ( (final_validate) && ( (!config_stream) || (!config_reference))) {
                LOGERR("Validate: stream not added to configuration");
                TRACE2_MSG("Fail");
                return -EACMSTREAMCONFIG;
            }
        }
        /* check if both redundant streams are added to different modules*/
        if ( (module_stream != NULL) && (module_reference != NULL)) {
            if (module_stream == module_reference) {
                // two redundant streams are added to same module
                LOGERR("Validate: two redundant streams are added to same module: %d",
                        module_stream->module_id);
                TRACE2_MSG("Fail");
                return -EACMREDSAMEMOD;
            }

        }
    }

    /* check payload size and truncation of forward operations*/
    ret = check_stream_payload(stream);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* verify that max number of insert operations isn't exceeded */
    if (stream_num_operations_x(stream, INSERT) > ACM_MAX_INSERT_OPERATIONS) {
        TRACE2_MSG("Fail");
        return -EACMNUMINSERT;
    }

    if (!final_validate) {
        if (ACMLIST_REF(stream, entry)) {
            ret = validate_stream_list(ACMLIST_REF(stream, entry), final_validate);
            TRACE2_EXIT();
            return ret;
        }
    }

    TRACE2_EXIT();
    return 0;
}

int __must_check validate_stream_list(struct stream_list *stream_list,
bool final_validate) {
    int ret;

    TRACE2_ENTER();
    TRACE2_MSG("final_validate=%d", final_validate);
    if (final_validate) {
        struct acm_stream *stream;

        ret = 0;
        ACMLIST_LOCK(stream_list);
        ACMLIST_FOREACH(stream, stream_list, entry)
        {
            ret = validate_stream(stream, final_validate);
            if (ret != 0) {
                break;
            }
        }
        ACMLIST_UNLOCK(stream_list);
        if (ret != 0) {
            TRACE2_MSG("Fail");
            return ret;
        }
    } else {

        /*
         * there are no tests for stream list which have to be done for final and
         * non final validation
         */
        ret = validate_module(streamlist_to_module(stream_list), final_validate);
        TRACE2_EXIT();
        return ret;
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check validate_module(struct acm_module *module, bool final_validate) {
    struct stream_list *streamlist;
    struct acm_stream *stream;
    int ret, sum_const_buffer;
    int num_redundant_stream;
    int num_lookup_entries;
    int num_ingress_ops, num_egress_ops;

    TRACE2_ENTER();
    TRACE2_MSG("final_validate=%d", final_validate);
    if (!module) {
        LOGERR("Validate: no module as input");
        TRACE2_MSG("Fail");
        return -EINVAL;
    }
    if (final_validate) {
        // final validation is top down validation
        ret = validate_stream_list(&module->streams, final_validate);
        if (ret != 0) {
            TRACE2_MSG("Fail");
            return ret;
        }
    }

    // following checks have to be done for final and non final validation
    streamlist = &module->streams;

    // Total constant message buffer size per module <= 4096
    sum_const_buffer = 0;
    ACMLIST_LOCK(streamlist);
    ACMLIST_FOREACH(stream, streamlist, entry)
        sum_const_buffer = sum_const_buffer + stream_sum_const_buffer(stream);
    ACMLIST_UNLOCK(streamlist);
    if (sum_const_buffer > ACM_MAX_CONST_BUFFER_SIZE) {
        LOGERR("Validate: constant message buffer %d too long", sum_const_buffer);
        TRACE2_MSG("Fail");
        return -EACMCONSTBUFFER;
    }

    /*  Maximal number of redundant streams per module <= 31, the 32. redundant
     * 	stream will be used as target for scheduler events without redundancy */
    num_redundant_stream = REDUNDANCY_START_IDX;
    ACMLIST_LOCK(streamlist);
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        if ( (stream->type == REDUNDANT_STREAM_TX) || (stream->type == REDUNDANT_STREAM_RX)) {
            num_redundant_stream++;
        }
    }
    ACMLIST_UNLOCK(streamlist);
    if (num_redundant_stream > (ACM_MAX_REDUNDANT_STREAMS)) {
        LOGERR("Validate: too many redundant streams: %d", num_redundant_stream);
        TRACE2_MSG("Fail");
        return -EACMREDUNDANDSTREAMS;
    }

    // max number of schedule events (HW version!) <= 1024 within a module
    struct fsc_command *first_item;
    uint add_items;
    add_items = calc_nop_schedules_for_long_cycles(&module->fsc_list);
    first_item = ACMLIST_FIRST(&module->fsc_list);
    if ( ( ( (ACMLIST_COUNT(&module->fsc_list) + add_items) > ACM_MAX_SCHEDULE_EVENTS)
            && (first_item->hw_schedule_item.abs_cycle == 0))
            || ( ( (ACMLIST_COUNT(&module->fsc_list) + add_items) > (ACM_MAX_SCHEDULE_EVENTS - 1))
                    && (first_item->hw_schedule_item.abs_cycle != 0))) {
        LOGERR("Validate: too many schedule events: %d", ACMLIST_COUNT(&module->fsc_list));
        TRACE2_MSG("Fail");
        return -EACMSCHEDULEEVENTS;
    }

    /* check if all configured stream periods are compatible with the scheduling
     * 		cycle of the associated module */
    if (module->cycle_ns == 0) {
        /* module period not set since initialization */
        LOGERR("Validate: module period equal zero");
        TRACE2_MSG("Fail");
        return -EACMMODCYCLE;
    }
    ret = 0;
    ACMLIST_LOCK(streamlist);
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        ret = stream_check_periods(stream, module->cycle_ns, final_validate);
        if (ret != 0) {
            // there was a problem with a stream schedule cycle
            break;
        }
    }
    ACMLIST_UNLOCK(streamlist);
    if (ret != 0) {
        TRACE2_MSG("Fail");
        return ret;
    }

    /* if scheduling gap < 8ticks between 2 schedule events within one module
     * 		-> error! */
    ret = check_module_scheduling_gaps(module);
    if (ret != 0) {
        LOGERR("Validate: scheduling gap too short");
        TRACE2_MSG("Fail");
        return ret;
    }

    // Maximum number of egress operations (Gather limit) <= 256
    /* counter initialized with 2 for general NOP operation
     * inserted at index 0 and forward_all operation for lookup rule 16(17)
     * inserted at index 1 */
    num_egress_ops = GATHER_START_IDX;
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        int prefetch_ops, gather_ops;
        prefetch_ops = stream_num_prefetch_ops(stream);
        gather_ops = stream_num_gather_ops(stream);
        if (gather_ops >= prefetch_ops) {
            num_egress_ops = num_egress_ops + gather_ops;
        } else {
            num_egress_ops = num_egress_ops + prefetch_ops;
        }
    }
    ACMLIST_UNLOCK(&module->streams);
    if (num_egress_ops > ACM_MAX_EGRESS_OPERATIONS) {
        LOGERR("Validate: too many egress operations: %d", num_egress_ops);
        TRACE2_MSG("Fail");
        return -EACMEGRESSOPERATIONS;
    }

    // Maximum number of ingress operations (Scatter limit) <= 256
    /* initialization counter initialized with 1 for general NOP operation
     * inserted at index 0 */
    num_ingress_ops = SCATTER_START_IDX;
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        num_ingress_ops = num_ingress_ops + stream_num_scatter_ops(stream);
    }
    ACMLIST_UNLOCK(&module->streams);
    if (num_ingress_ops > ACM_MAX_INGRESS_OPERATIONS) {
        LOGERR("Validate: too many ingress operations: %d", num_ingress_ops);
        TRACE2_MSG("Fail");
        return -EACMINGRESSOPERATIONS;
    }

    // Maximal number of lookup entries used <= 16
    num_lookup_entries = LOOKUP_START_IDX;
    ACMLIST_LOCK(&module->streams);
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        if ( (stream->type == INGRESS_TRIGGERED_STREAM) || (stream->type == REDUNDANT_STREAM_RX)) {
            /* as an Ingress Triggered Stream always has a lookup entry, it is
             * sufficient to count Ingress Triggered Streams. But don't forget
             * streams of type REDUNDANT_STREAM_RX, because they are Ingress
             * Triggered streams with a reference to another Ingress Triggered
             * stream */
            num_lookup_entries++;
        }
    }
    ACMLIST_UNLOCK(&module->streams);
    if (num_lookup_entries > ACM_MAX_LOOKUP_ITEMS) {
        LOGERR("Validate: too many lookup entries: %d", num_lookup_entries);
        TRACE2_MSG("Fail");
        return -EACMLOOKUPENTRIES;
    }

    if (!final_validate) {
        // non final validation is bottom up validation
        if (module->config_reference) {
            ret = validate_config(module->config_reference, final_validate);
            TRACE3_EXIT();
            return ret;
        }
    }
    TRACE2_EXIT();
    return 0;
}

int __must_check validate_config(struct acm_config *config, bool final_validate) {
    int ret;

    TRACE2_ENTER();
    TRACE2_MSG("final_validate=%d", final_validate);
    if (final_validate) {
        int i;

        // final validation is top down validation
        for (i = 0; i < ACM_MODULES_COUNT; i++) {
            if (config->bypass[i] != NULL) {
                ret = validate_module(config->bypass[i], true);
                if (ret != 0) {
                    TRACE2_MSG("Fail. valide_module=%d", ret);
                    return ret;
                }
            }
        }
        /* Each stream in the ACM configuration shall have at least one
         * operation defined. It is irrelevant if the operation was created
         * automatically by the library at stream_create() or was added to the
         * stream later. In case of an Ingress Triggered Stream with no operation
         * but with a reference to an Event Stream, no error shall be returned
         * if the Event Stream has an operation.
         */
        for (i = 0; (i < ACM_MODULES_COUNT); i++) {
            bool operation_exists;

            operation_exists = check_module_op_exists(config->bypass[i]);
            if (!operation_exists) {
                LOGERR("Validate: stream without operation");
                TRACE2_MSG("Fail. opcode missing");
                return -EACMOPMISSING;
            }
        }
    }

    // following checks have to be done for final and non final validation
    ret = clean_and_recalculate_hw_msg_buffs(config);
    if (ret != 0) {
        TRACE2_MSG("Fail. clean and recalculate hw msg.buffs ret=%d", ret);
        return ret;
    }
    // read defined number of message Buffers
    if (ACMLIST_COUNT(&config->msg_buffs) > get_int32_status_value(__stringify(ACM_SYSFS_MSGBUF_COUNT))) {
        LOGERR("Validate: too many message buffers: %d", ACMLIST_COUNT(&config->msg_buffs));
        TRACE2_MSG("Fail. Max msg-buffers");
        return -EACMNUMMESSBUFF;
    }

    TRACE2_EXIT();
    return 0;
}

int __must_check calc_stream_egress_framesize(struct acm_stream *stream) {
    int egress_framesize;
    struct operation *operation;
    struct operation_list *oplist;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0;   // not an error, but framesize 0
    }
    egress_framesize = 0;
    oplist = &stream->operations;
    ACMLIST_LOCK(&stream->operations);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if ( (operation->opcode == INSERT) || (operation->opcode == INSERT_CONSTANT)
                || (operation->opcode == PAD) || (operation->opcode == FORWARD)) {
            egress_framesize = egress_framesize + operation->length;
        }
    }
    ACMLIST_UNLOCK(&stream->operations);
    TRACE3_MSG("egress framesize is %d", egress_framesize);
    TRACE3_EXIT();
    return egress_framesize;
}

bool __must_check check_module_op_exists(struct acm_module *module) {
    bool operation_exists;
    struct acm_stream *stream;
    struct stream_list *streamlist;

    TRACE3_ENTER();
    if (!module) {
        TRACE3_MSG("Fail");
        return true;   // there are no streams without an operation
    }
    operation_exists = true;
    streamlist = &module->streams;
    ACMLIST_LOCK(&module->streams);;
    ACMLIST_FOREACH(stream, streamlist, entry)
    {
        struct operation *operation;
        struct operation_list *oplist = &stream->operations;

        operation = ACMLIST_FIRST(oplist);
        if (!operation) {
            operation_exists = false;
            /* in case of an Ingress Triggered Stream with a reference to
             * an Event Stream the Ingress Triggered Stream doesn't need an
             * operation itself. It isn't necessary to check at this point if
             * the Event Stream has an operation, as all streams are iterated
             * anyway. */
            if ( (stream->type == INGRESS_TRIGGERED_STREAM) && (stream->reference)) {
                operation_exists = true;
            }
        }
        if (!operation_exists) {
            break;
        }
    }
    ACMLIST_UNLOCK(&module->streams);;
    TRACE3_EXIT();
    return operation_exists;
}

int __must_check stream_sum_const_buffer(struct acm_stream *stream) {
    int sum_constant_buffer_size;
    struct operation *operation;
    struct operation_list *oplist;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0;   // as there is no stream sum of constant message buffers is 0
    }
    sum_constant_buffer_size = 0;
    oplist = &stream->operations;
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        if (operation->opcode == INSERT_CONSTANT) {
            sum_constant_buffer_size = sum_constant_buffer_size + operation->length;
        }
    }
    ACMLIST_UNLOCK(oplist);
    TRACE3_MSG("calculated buffer-size is %d", sum_constant_buffer_size);
    TRACE3_EXIT();
    return sum_constant_buffer_size;
}

int __must_check stream_check_periods(struct acm_stream *stream, uint32_t module_cycle_ns
		, bool final_validate) {
    struct schedule_list *window_list;
    struct schedule_entry *schedule;
    struct schedule_entry *schedule_redundant;
    int ret;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0;   // as there is no stream there are no incompatible periods
    }
    ret = 0;
    window_list = &stream->windows;
    ACMLIST_LOCK(window_list);
    ACMLIST_FOREACH(schedule, window_list, entry)
    {
        if (schedule->period_ns == 0) {
            LOGERR("Validate: Period of window has value 0");
            ret = -EINVAL;
            break;
        }
        if ( (module_cycle_ns % schedule->period_ns) != 0) {
            LOGERR("Validate: stream schedule period not compatible to module period");
            ret = -EACMINCOMPATIBLEPERIOD;
            break;
        }
    }

    if (final_validate) {
		//Additional checks for redundant streams
		if ( stream->type == REDUNDANT_STREAM_TX || stream->type == REDUNDANT_STREAM_RX  ) {
			// Maximal one entry can be defined in the schedule list
			if ( ACMLIST_COUNT(window_list) > 1 ) {
				LOGERR("Validate: redundant stream schedule list can contain maximal one entry (%d>1)"
						, ACMLIST_COUNT(window_list));
				ret = -EINVAL;
			}
			// stream and redundant_reference must have same number of entries (one or zero)
			if ( ACMLIST_COUNT(window_list) != ACMLIST_COUNT(&stream->reference_redundant->windows) ) {
				LOGERR("Validate: redundant stream schedule not aligned between modules (%d != %d)"
						, ACMLIST_COUNT(window_list)
						, ACMLIST_COUNT(&stream->reference_redundant->windows));
				ret = -EINVAL;
			}
			else if ( ACMLIST_COUNT(window_list) == 1 ) {
				schedule = ACMLIST_FIRST(window_list);
				schedule_redundant = ACMLIST_FIRST(&stream->reference_redundant->windows);

				// period and cycle of the stream must be equal
				if ( module_cycle_ns != schedule->period_ns ) {
					LOGERR("Validate: redundant stream schedule period (%dns) not equal to module cycle (%dns)"
							, schedule->period_ns, module_cycle_ns);
					ret = -EACMINCOMPATIBLEPERIOD;
				}
				// stream period must be equal to period on reference stream
				if ( schedule->period_ns != schedule_redundant->period_ns ) {
					LOGERR("Validate: redundant stream cycle not aligned between modules (%d != %d)"
							, schedule->period_ns, schedule_redundant->period_ns);
					ret = -EACMINCOMPATIBLEPERIOD;
				}
			}
		}
    }

    ACMLIST_UNLOCK(window_list);
    TRACE3_EXIT();
    return ret;
}

int __must_check check_module_scheduling_gaps(struct acm_module *module) {
    uint32_t last_time;
    int ret;
    struct fsc_command_list *fsc_list;
    struct fsc_command *fsc_item;

    TRACE3_ENTER();
    if (!module) {
        LOGGING_DEBUG("Validate: no module got in %s", __func__);
        TRACE3_MSG("Fail");
        return -EACMINTERNAL;
    }

    last_time = 0;
    ret = 0;
    fsc_list = &module->fsc_list;
    ACMLIST_LOCK(fsc_list);
    ACMLIST_FOREACH(fsc_item, fsc_list, entry)
    {
        uint32_t diff;

        diff = fsc_item->hw_schedule_item.abs_cycle - last_time;
        if (diff < ANZ_MIN_TICKS) {
            if (!((diff == 0) && (fsc_item == ACMLIST_FIRST(fsc_list)))) {
                /* There is one exception to the restriction that the difference has to be
                 * < ANZ_MIN_TICKS: it is for the first item, if this first item has in
                 * .abs_cycle the value 0. (results in diff 0). This is a valid case, because
                 * for that situation there is no NOP item written to the schedule table at the
                 * beginning of the table. */
                LOGERR("Validate: scheduling gap: %d; cycle times: %d, %d",
                        diff,
                        last_time,
                        fsc_item->hw_schedule_item.abs_cycle);
                ret = -EACMSCHEDTIME;
                break;
            }
        }
        last_time = fsc_item->hw_schedule_item.abs_cycle;
    }
    ACMLIST_UNLOCK(fsc_list);
    TRACE3_EXIT();
    return ret;
}

int __must_check check_stream_payload(struct acm_stream *stream) {
    struct operation_list *oplist;
    struct operation *operation;
    unsigned int egress_pos, ingress_pos;
    int ret;

    TRACE3_ENTER();
    if (!stream) {
        TRACE3_EXIT();
        return 0; 	// no problem with payload
    }
    ret = 0;
    egress_pos = 0;
    ingress_pos = 0;
    oplist = &(stream->operations);
    ACMLIST_LOCK(oplist);
    ACMLIST_FOREACH(operation, oplist, entry)
    {
        TRACE3_MSG("opcode=%d", operation->opcode);
        switch (operation->opcode) {
            case INSERT:
            case INSERT_CONSTANT:
            case PAD:
                egress_pos = egress_pos + operation->length;
                ingress_pos = ingress_pos + operation->length;
                break;
            case FORWARD:
                /* it has to be checked if forward offset is higher than ingress_pos
                 * plus MAX_TRUNC_BYTES */
                if (operation->offset > ingress_pos + MAX_TRUNC_BYTES) {
                    LOGERR("Validate: forward operation truncates too many bytes: %d",
                            operation->offset - ingress_pos);
                    ret = - EACMFWDOFFSET;
                    goto exit;
                }
                egress_pos = egress_pos + operation->length;
                /* ingress position depends on forward offset: additional increment */
                if (operation->offset > ingress_pos) {
                    ingress_pos = operation->offset;
                }
                ingress_pos = ingress_pos + operation->length;
                break;
            default:
                /* READ or FORWARD_ALL operation - no action */
                break;
        }
    }

exit:
    ACMLIST_UNLOCK(oplist);
    if (ret < 0) {
        TRACE3_MSG("Fail");
        return ret;
    }
    /* check payload size of stream */
    if (egress_pos > MAX_PAYLOAD_HEADER_SIZE) {
        LOGERR("Validate: size of payload and header %d higher than maximum: %d",
                egress_pos,
                MAX_PAYLOAD_HEADER_SIZE);
        return -EACMPAYLOAD;
    }
    TRACE3_EXIT();
    return 0;
}

int __must_check buffername_check(struct buffer_list *sysfs_buffer_list,
        struct sysfs_buffer *msg_buf,
        bool *reuse,
        struct sysfs_buffer **reuse_msg_buff) {

    struct sysfs_buffer *buffer;
    int ret;

    TRACE3_ENTER();
    /* check if buffername of msg_buf item is equal to any other buffername
     * in sysfs_buffer_list
     * if buffernames are equal and stream directions of buffers are different
     * -> error;
     * if buffernames are equal and stream directions of buffers are also
     * equal, then return that another hw message buffer can be
     * reused for msg_buf */
    ret = 0;
    *reuse = false;
    *reuse_msg_buff = NULL;
    ACMLIST_LOCK(sysfs_buffer_list);
    ACMLIST_FOREACH(buffer, sysfs_buffer_list, entry)
    {
        ret = strcmp(buffer->msg_buff_name, msg_buf->msg_buff_name);
        if (ret == 0) {
            /* strings are equal */
            if (buffer->stream_direction != msg_buf->stream_direction) {
                LOGERR("Validate: buffername %s equal but stream direction different",
                        msg_buf->msg_buff_name);
                ret = -EPERM;
                break;
            }
            /* two buffernames for streams with same direction are equal */
            *reuse = true;
            *reuse_msg_buff = buffer;
            break;
        } else {
            /* assign 0 to ret: result is okay, but ret might have negative value*/
            ret = 0;
        }
    }
    ACMLIST_UNLOCK(sysfs_buffer_list);
    TRACE3_EXIT();
    return ret;
}

