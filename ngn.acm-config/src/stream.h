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
#ifndef STREAM_H_
#define STREAM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <net/ethernet.h> /* for ETHER_ADDR_LEN */

#include "lookup.h"
#include "schedule.h"
#include "operation.h"
#include "list.h"

struct lookup;

/**
 * @brief types of streams
 *
 * enum stream_type is used to have an indication to which group a stream belongs to so that it
 * is possible to derive valid properties of the stream from this info.
 */
enum stream_type {
    INGRESS_TRIGGERED_STREAM, /**< ingress stream */
    TIME_TRIGGERED_STREAM, /**< egress stream */
    EVENT_STREAM, /**< egress stream which can have a child reference to a TIME_TRIGGERED_STREAM,
    and a parent reference to a RECOVERY_STREAM */
    RECOVERY_STREAM, /**< egress stream which can have a child reference to an EVENT_STREAM */
    REDUNDANT_STREAM_TX, /**< TIME_TRIGGERED_STREAM with redundancy link to another TIME_TRIGGERED_STREAM */
    REDUNDANT_STREAM_RX, /**< INGRESS_TRIGGERED_STREAM with redundancy link to another INGRESS_TRIGGERED_STREAM */
    MAX_STREAM_TYPE,
};

/**
 * @brief structure for handling of queue properties: access lock, number of queue items, list of items
 */
struct acm_stream;
ACMLIST_HEAD(stream_list, acm_stream);

/**
 * @brief helper for initializing a streamlist - especially for module test
 */
#define STREAMLIST_INITIALIZER(streamlist) ACMLIST_HEAD_INITIALIZER(streamlist)

/**
 * @brief structure which holds all the relevant stream data
 */
struct acm_stream {
    enum stream_type type; /**< specifies which type the stream has */
    struct lookup *lookup; /**< only relevant for INGRESS_TRIGGERED_STREAM and REDUNDANT_STREAM_RX */
    struct operation_list operations; /**< list of operations defined for the stream */
    struct schedule_list windows; /**< list of schedules defined for the stream */
    struct acm_stream *reference; /**< only relevant for INGRESS_TRIGGERED_STREAM and EVENT_STREAM */
    struct acm_stream *reference_parent; /**< only relevant for EVENT_STREAM and RECOVERY_STREAM */
    struct acm_stream *reference_redundant; /**< only relevant for REDUNDANT_STREAM_TX and REDUNDANT_STREAM_RX */
    uint32_t indiv_recov_timeout_ms; /**< only relevant for INGRESS_TRIGGERED_STREAM;
    redundancy tag is to be expected. */
    ACMLIST_ENTRY(stream_list, acm_stream) entry; /**< links to next and previous stream and streamlist header of the module */
    uint16_t gather_dma_index; /**< index used for writing gather operations (all except READ)
    to hardware table; not relevant for REDUNDANT_STREAM_RX, which can have only READ operation*/
    uint16_t scatter_dma_index; /**< index used for writing READ operations to hardware table
    only relevant for INGRESS_TRIGGERED_STREAM and REDUNDANT_STREAM_RX */
    uint8_t redundand_index; /**< index used for writing redundant data to HW;
    only relevant for REDUNDANT_STREAM_TX and REDUNDANT_STREAM_RX*/
    uint8_t lookup_index; /**< index used for writing lookup data to HW;
    only relevant for INGRESS_TRIGGERED_STREAM and REDUNDANT_STREAM_RX */
};

/**
 * @brief calculation of stream address for operation list
 */
/* each operation list is contained in a stream */
static inline struct acm_stream *operationlist_to_stream(const struct operation_list *list) {
    return (void *)((char *)(list) - offsetof(struct acm_stream, operations));
}

/**
 * @brief calculation of stream address for schedule list
 */
/* each schedule list is contained in a stream */
static inline struct acm_stream *schedulelist_to_stream(const struct schedule_list *list) {
    return (void *)((char *)(list) - offsetof(struct acm_stream, windows));
}

/**
 * @brief helper for initializing a stream - only stream type with non default value
 *
 * especially for module test
 */
#define STREAM_INITIALIZER(_stream, _type)                          \
{                                                                   \
    .type = (_type),                                                \
    .lookup = NULL,                                                 \
    .operations = OPERATION_LIST_INITIALIZER((_stream).operations), \
    .windows = SCHEDULE_LIST_INITIALIZER((_stream).windows),        \
    .reference = NULL,                                              \
    .reference_parent = NULL,                                       \
    .reference_redundant = NULL,                                    \
    .indiv_recov_timeout_ms = 0,                                    \
    .entry = ACMLIST_ENTRY_INITIALIZER,                             \
    .gather_dma_index = 0,                                          \
    .scatter_dma_index = 0,                                         \
    .redundand_index = 0,                                           \
    .lookup_index = 0                                               \
}

/**
 * @brief helper for initializing a stream - non default value for all stream items possible
 *
 * especially for module test
 */
#define STREAM_INITIALIZER_ALL(_stream, _type, _lookup, _reference,   \
    _reference_parent, _reference_redundant, _indiv_recov_timeout_ms, \
    _gather_dma_index, _scatter_dma_index, _redundand_index,          \
    _lookup_index)                                                    \
{                                                                     \
    .type = (_type),                                                  \
    .lookup = (_lookup),                                              \
    .operations = OPERATION_LIST_INITIALIZER((_stream).operations),   \
    .windows = SCHEDULE_LIST_INITIALIZER((_stream).windows),          \
    .reference = (_reference),                                        \
    .reference_parent = (_reference_parent),                          \
    .reference_redundant = (_reference_redundant),                    \
    .indiv_recov_timeout_ms = (_indiv_recov_timeout_ms),              \
    .entry = ACMLIST_ENTRY_INITIALIZER,                               \
    .gather_dma_index = (_gather_dma_index),                          \
    .scatter_dma_index = (_scatter_dma_index),                        \
    .redundand_index = (_redundand_index),                            \
    .lookup_index = (_lookup_index)                                   \
}

/**
 * @brief offset of destination mac address for forward operation
 */
#define OFFSET_DEST_MAC_IN_FRAME 0
/**
 * @brief offset of source mac address for forward operation
 */
#define OFFSET_SOURCE_MAC_IN_FRAME 6
/**
 * @brief offset of vlan tag for forward operation
 */
#define OFFSET_VLAN_TAG_IN_FRAME 12

/**
 * @brief placeholder for mac address of an operation
 *
 * Used as placeholder, when mac address of the associated port shall be used, but can only be
 * found out after stream is added to a module.
 */
#define LOCAL_SMAC_CONST "sw0p23"

/**
 * @ingroup acmstream
 * @brief create a stream item
 *
 * The function allocates a new stream item and initializes stream type and
 * references to other streams. Operations list and schedules list are also
 * initialized.
 * In case of an error NULL is returned instead of a handle.
 *
 * @param type type of the stream to be created
 *
 * @return handle to the stream
 */
struct acm_stream __must_check *stream_create(enum stream_type type);

/**
 * @ingroup acmstream
 * @brief delete a stream
 *
 * The function delets a stream, but only if it isn't added to a module and
 * if it isn't referenced by another stream having stream type Event or Recovery.
 * Before the stream is deleted all subitems (operations, schedules, lookup) are
 * deleted.
 *
 * @param stream stream to be destroyed
 */
void stream_delete(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief destroy a stream
 *
 * The function destroys a stream, independent of any references the stream has.
 *
 * @param stream stream to be destroyed
 */
void stream_destroy(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief add an operation to a stream
 *
 * The function adds the operation to the operation list of the stream if stream
 * type and operation code are compatible. In this case operation list and stream item are
 * validated.
 * If stream is added to a module, then also stream list and module are validated. And
 * if the module is added to a configuration, then also the configuration is validated.
 * Only if all validations are passed, the function returns 0 for success.
 *
 * @param stream stream to which the operation is added to
 * @param operation operation which shall be added
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check stream_add_operation(struct acm_stream *stream, struct operation *operation);

/**
 * @ingroup acmstream
 * @brief create a reference between two streams
 *
 * The function sets the parent-child or redundant references of the streams in
 * the input parameters, depending on the types of the streams in the input
 * parameters.
 * The function returns an error if any validation fails, when setting the
 * reference is applied. (Validation done depending on link status of streams
 * up to configuration level.)
 * The function returns also an error if an input stream already contains a
 * reference.
 *
 * @param stream parent-stream item in case of Ingress Triggered Stream or Event Stream;
 *        in case of Time Triggered Stream item on same level as item of parameter reference
 * @param reference child-stream item in case of Event Stream or Recovery Stream;
 *        in case of Time Triggered Stream item on same level as item of parameter steam
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check stream_set_reference(struct acm_stream *stream, struct acm_stream *reference);

/**
 * @ingroup acmstream
 * @brief set the egress header of a stream
 *
 * The function creates for egress stream types 3 operations: one for
 * destination MAC address, one for source MAC address and one for vlan_id. The
 * operations are inserted into the operations list of the stream.
 * If anything goes wrong when creating these operations, all items created so
 * far are released, an error is written to the error log and the function is ended
 * unsuccessfully.
 *
 * @param stream reference to the stream which egress header should be defined
 * @param dmac destination MAC address of the stream
 * @param smac source MAC address of the stream
 *	   - If the source MAC is 00-00-00-00-00-00 the MAC of the port will be inserted
 * @param vlan VLAN id to be inserted (range 3-4094)
 * @param prio VLAN priority to be inserted (range 0-7)
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check stream_set_egress_header(struct acm_stream *stream,
        uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan,
        uint8_t prio);

/**
 * @ingroup acmstream
 * @brief inserts a stream into a streamlist
 *
 * The function adds the stream to the end of the streamlist. The function returns an
 * error, if the stream is already added to a streamlist, or the validation of the
 * extended streamlist doesn't succeed. (Validation is done depending on linking status
 * of the stream item up to configuration.)
 *
 * @param streamlist address of the streamlist
 * @param stream address of the stream to be added to streamlist
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check stream_add_list(struct stream_list *streamlist, struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief deletes all elements in a streamlist
 *
 * The function deletes all stream items in streamlist. Stream items are deleted
 * independent of any references to other items.
 *
 * @param streamlist address of the streamlist
 */
void stream_empty_list(struct stream_list *streamlist);

/**
 * @ingroup acmstream
 * @brief removes a stream item from a stream_list
 *
 * The function first checks if the stream item is part of the stream_list. If
 * this is the case, all administrative tasks are done, so that there is no
 * reference between stream and stream_list any more. The stream is still
 * available after removing it.
 * If stream_list doesn't contain the stream, no action is taken.
 *
 * @param stream_list address of the stream_list from which the stream shall be
 * 				removed
 * @param stream	address of the stream, which shall be removed from stream_list
 */
void stream_remove_list(struct stream_list *stream_list, struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief initializes the links to message buffer items of operations in
 * the operation list of the stream
 *
 * The function iterates through all operations in the operation list of the
 * stream and sets the link to sysfs_buffer (message buffer item) of READ and
 * INSERT operations (these are the ones which use message buffers) to NULL.
 *
 * @param stream	address of the stream
 */
void stream_clean_msg_buff_links(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief checks if vlan parameter are within defined ranges
 *
 * The function the vlan parameter values against the minimum and maximum values
 * of the defined range.
 * ACM_VLAN_ID_MIN, ACM_VLAN_ID_MAX, ACM_VLAN_PRIO_MIN, ACM_VLAN_PRIO_MAX
 *
 * @param vlan_id id of the vlan
 * @param vlan_prio priority of the vlan
 *
 * @return the function will return 0 in case the vlan parameters are within
 * the specified ranges. Negative values represent an error - one of the vlan
 * parameter has a value out of range.
 */
int __must_check stream_check_vlan_parameter(uint16_t vlan_id, uint8_t vlan_prio);

/**
 * @ingroup acmstream
 * @brief checks if the stream belongs to a configuration already applied to HW
 *
 * The function tries to navigate from stream structure up to a configuration.
 * If all links/references are set for this navigation, the function checks if
 * the configuration was already applied to hardware.
 * If navigation is interrupted by a missing link, then the stream implicitly
 * doesn't belong to a configuration applied to HW.
 *
 * @param stream address of the stream
 *
 * @return the function will return true if the stream was added to a
 * configuration and the configuration was applied to HW. In all other
 * cases (stream not added to a module, module not added to a configuration,
 * configuration not applied to HW, ...) the function returns false.
 */
bool __must_check stream_config_applied(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief checks if operation with opcode exists in stream
 *
 * The function iterates through the operations list of the stream until it
 * finds the first operation with operation code opcode or the end of the
 * operations list is reached.
 *
 * @param stream address of the stream
 * @param opcode operation code to look for in the stream
 *
 * @return The function will return true if the stream contains an operation
 * with opcode. In all other cases the function returns false.
 */
bool __must_check stream_has_operation_x(struct acm_stream *stream, enum acm_operation_code opcode);

/**
 * @ingroup acmstream
 * @brief returns number of operations of operation code x in stream
 *
 * The function iterates through the operations list of the stream and increments the counter for
 * operation code by one for each operation with operation code x in the stream. The counter stays
 * at 0, if the stream doesn't contain an operation with operation code x.
 *
 * @param stream address of the stream
 * @param opcode operation code to look for in the stream
 *
 * @return The function will return how many operations with operation code x are in the operation
 * list of the stream.
 */
int __must_check stream_num_operations_x(struct acm_stream *stream, enum acm_operation_code opcode);

/**
 * @ingroup acmstream
 * @brief calculates/recalculates indexes for hardware tables
 *
 * The function derives from the type of stream for which hardware tables
 * indexes have to be calculated newly. Calculation/recalculation is done for
 * all streams in stream_list. The new values overwrite the previous values.
 * stream type					affected HW tables
 * INGRESS_TRIGGERED_STREAM		lookup table
 * 								scatter_dma_table
 * 								gather_dma_table
 * REDUNDANT_STREAM_TX			lookup table
 * 								redundancy table
 * 								gather_dma_table
 * TIME_TRIGGERED_STREAM		redundancy table
 * 								gather_dma_table
 * REDUNDANT_STREAM_TX			redundancy table
 * 								gather_dma_table
 * EVENT_STREAM					gather_dma_table
 * RECOVERY_STREAM				gather_dma_table
 *
 * @param stream_list address of the stream_list which was changed
 * @param stream address of the stream, which was added/removed
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check calculate_indizes_for_HW_tables(struct stream_list *stream_list,
        struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief calculates/recalculates indexes for lookup table
 *
 * The function iterates through all streams of the stream_list and sets the
 * lookup index of each stream: For each INGRESS_TRIGGERED_STREAM and each
 * REDUNDANT_STREAM_RX the lookup index is set to the actual counter value,
 * which starts at LOOKUP_START_IDX and is incremented by one for each
 * INGRESS_TRIGGERED_STREAM and each REDUNDANT_STREAM_RX. For all streams
 * which are not of type INGRESS_TRIGGERED_STREAM or REDUNDANT_STREAM_RX,
 * the lookup index is set to zero.
 * Check if index increases beyond maximum value isn't done here, but in
 * validation functions.
 *
 * @param stream_list address of the stream_list for which the calculation
 * 				shall be done
 */
void calculate_lookup_indizes(struct stream_list *stream_list);

/**
 * @ingroup acmstream
 * @brief calculates/recalculates indexes for redundancy table
 *
 * The function iterates through all streams of the stream_list and sets the
 * redundancy index of each stream. A different algorithm is used depending on
 * the module id, the stream_list is connected to:
 * For module id equal MODULE_0:
 * 		For each REDUNDANT_STREAM_TX/RX the redundancy index is set to the actual
 * 		counter value, which starts at REDUNDANCY_START_IDX and is incremented
 * 		by one for each REDUNDANT_STREAM_TX/RX.
 * For module id equal MODULE_1:
 * 		For each REDUNDANT_STREAM_TX/RX the redundancy index is set to the
 * 		redundancy index of the linked REDUNDANT_STREAM_TX/RX.
 * For all streams which are not of type REDUNDANT_STREAM_TX/RX, the redundancy
 * index is set to zero independent of the module id.
 * Note: Check if index increases beyond maximum value isn't done here, but in
 * validation functions.
 *
 * @param stream_list address of the stream_list for which the calculation
 * 				shall be done
 */
void calculate_redundancy_indizes(struct stream_list *stream_list);

/**
 * @ingroup acmstream
 * @brief calculates/recalculates indexes for gather DMA table
 *
 * The function iterates through all streams of the stream_list and sets the
 * gather DMA table index of each stream.
 * For each stream with no gather operations the gather DMA table index is set
 * to zero.
 * For each stream with only one FORWARD_ALL operation and no other gather
 * operation the gather DMA table index is set to 1.
 * For the other streams the gather DMA table index is set to the actual
 * counter value, which starts at GATHER_START_IDX and is incremented by the
 * number of gather operations of the stream. For streams of type
 * REDUNDANT_STREAM_TX the counter value is incremented additionally by one as
 * for these streams an automatically created R-tag command has to be
 * considered.
 * Note: gather operations are: INSERT, INSERT_CONSTANT, PAD, FORWARD, FORWARD_ALL
 *
 * @param stream_list address of the stream_list for which the calculation
 * 				shall be done
 */
void calculate_gather_indizes(struct stream_list *stream_list);

/**
 * @ingroup acmstream
 * @brief calculates/recalculates indexes for scatter DMA table
 *
 * The function iterates through all streams of the stream_list and sets the
 * scatter DMA table index of each stream.
 * For each INGRESS_TRIGGERED_STREAM and for each REDUNDANT_STREAM_RX with no
 * scatter operations and for each stream which is not an
 * INGRESS_TRIGGERED_STREAM or a REDUNDANT_STREAM_RX the scatter DMA table index
 * is set to zero.
 *
 * For other streams of type INGRESS_TRIGGERED_STREAM and  REDUNDANT_STREAM_RX
 * the scatter DMA table index is set to the actual counter value, which starts
 * at SCATTER_START_IDX and is incremented by the number of scatter operations
 * of the stream.
 * Note: scatter operations are: READ
 *
 * @param stream_list address of the stream_list for which the calculation
 * 				shall be done
 */
void calculate_scatter_indizes(struct stream_list *stream_list);

/**
 * @ingroup acmstream
 * @brief checks if stream is part of stream_list
 *
 * The function iterates through the streams of stream_list until it
 * finds the specified stream or the end of stream_list is reached.
 *
 * @param stream_list address of stream_list in which stream shall be looked for
 * @param stream address of the stream
 *
 * @return The function returns true if the stream is part of stream_list.
 * In all other cases the function returns false.
 */
bool __must_check stream_in_list(struct stream_list *stream_list, struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief count the number of egress/gather operations of a stream
 *
 * The function iterates through all operations of the stream. For each egress
 * operation the return value is incremented by one.
 * If the stream is of type REDUNDANT_STREAM_TX, then the number is additionally
 * incremented by one without an operation for this in the operation list,
 * because an extra R-Tag command, which is added automatically must be taken
 * into account.
 * egress operations: INSERT, INSERT_CONSTANT, PAD, FORWARD, FORWARD_ALL
 *
 * @param stream pointer to the stream for which the egress operations
 * 			is calculated
 *
 * @return The function will return 0 in case the stream has no egress operation.
 * 			Otherwise the number of egress operations is returned.
 */
int __must_check stream_num_gather_ops(struct acm_stream *stream);
/**
 * @ingroup acmstream
 * @brief count the number of ingress/scatter operations of a stream
 *
 * The function iterates through all operations of the stream. For each ingress
 * operation the return value is incremented by one.
 * ingress operations: READ
 *
 * @param stream pointer to the stream for which the ingress operations
 * 			are calculated
 *
 * @return The function will return 0 in case the stream has no ingress operation.
 * 			Otherwise the number of ingress operations is returned.
 */
int __must_check stream_num_scatter_ops(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief count the number of prefetch operations of a stream
 *
 * The function takes the number of ingress operations of the stream. (One prefetch command has
 * to be written for each ingress operation.) If the number of ingress operations is greater than
 * zero, than 4 prefetch operations are added for the 4 operations of the prefetch locking vector.
 * If the number of ingress operations is zero than the number of prefetch operations is set to 1
 * for the prefetch NOP operation.
 *
 * @param stream pointer to the stream for which the number of prefetch operations
 *          is calculated
 *
 * @return The function will return 1 in case the stream has no ingress operation.
 *          Otherwise the number of prefetch operations is returned.
 */
int __must_check stream_num_prefetch_ops(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief set individual recovery timeout of a stream
 *
 * The function writes the timeout value into the stream data after a check if the stream is valid.
 *
 * @param stream pointer to the stream for which recovery timeout is to be set
 * @param timeout_ms number of milliseconds for the timeout
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error (for instance: no valid stream, configuration of stream already applied to HW). */
int __must_check stream_set_indiv_recov(struct acm_stream *stream, uint32_t timeout_ms);

#endif /* STREAM_H_ */
