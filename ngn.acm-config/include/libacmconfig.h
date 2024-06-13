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
/**
 * @file libacmconfig.h
 *
 * ACM configuration library
 *
 */

#ifndef LIBACMCONFIG_H_
#define LIBACMCONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif	/* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include <net/ethernet.h> /* for ETHER_ADDR_LEN */

#include "libacmconfig_def.h"

struct acm_diagnostic;
struct acm_stream;
struct acm_module;
struct acm_config;

/**
 ***********************************************************************
 * @defgroup acmconfiguration ACM configuration library
 * @brief The ACM configuration interface
 *
 * The ACM configuration library provides access to following parts:
 * * @ref acmstatusarea "The device status information"
 * * @ref acmdiagarea "The device diagnostic information"
 * * @ref acmcapability "The device capability information"
 * * @ref acmcontrol "The device control interface"
 * * @ref acmconfigarea "The configuration interface of the device"
 * @{
*/

/**
 * @defgroup acmstatusarea Status
 * @brief ACM status area
 *
 * The status items can be used to debug the device. It is recommended to
 * verify the correct operation of the device and compare the values to
 * expected values according to the provided configuration. In addition
 * the counters can be used to detect misbehaviors of the device.
*/

/**
 *  @defgroup acmdiagarea Diagnostics
 * @brief ACM diagnositc information
 *
 * The diagnostic data provides short-term evaluation counters. The
 * counters can be evaluated for each stream and need to be read for a
 * complete module at the same time. It is recommended to read the
 * diagnostic data in a cyclic manner aligned to the schedule cycle of the
 * configuration.
*/

/**
 * @defgroup acmcapability Capabilitys
 * @brief ACM device capabilities
 *
 * The functions can be used to read and verify the device specific
 * parameters. The parameters might be required by a scheduling tool to define
 * an accurate schedule or configuration for the device.
*/

/**
 * @defgroup acmcontrol Control
 * @brief ACM control items
 *
 * The control items allow to change settings during runtime.
*/

/**
 * @defgroup acmconfigarea Configuration
 * @brief ACM configuration area
 *
 * The configuration area provides all required functions to generate a device specific
 * configuration. It is possible to define a configuration based on a stream. Each stream
 * will be associated to a module which defines the transmission direction of a stream.
 *
 * @ref acmconfig "The complete configuration"
 * @ref acmmodule "The module contains a list of streams"
 * @ref acmstream "Create a stream"
 * @ref acmoperation "Create and add operations to a stream"
 * @ref acmschedule "Create and add scheduling entries to a stream"
 * @ref acmvalidate "Validate the configuration or parts of it"
 *
 * @{*/

/**
 * @defgroup acmconfig ACM Configuration
 * @brief ACM Configuration
 *
 * The ACM configuration item contains the complete configuration to be applied to a device.
 * Up to two modules with multiple streams defined in each module may be defined.
 *
 * The configuration will have the ownership of all sub-items. In case of destroying the configuration
 * all members will be destroyed too.
*/

/**
 * @defgroup acmmodule ACM Module
 * @brief ACM Module
 *
 * Each module can contains different streams. The module define the communication direction of a stream.
 * When a stream is added to a module, the module has the ownership of the stream.
 * It is permitted to remove a stream once it had been added module.
 * When a module is added to a configuration the ownership of the module is moved to the configuration.
 * It is permitted to remove a module one it had been added to a configuration.
*/

/**
 * @defgroup acmstream ACM Stream
 * @brief ACM Stream entries
 *
 * A stream is either time-triggered (transmitted from the device) or ingress-triggered
 * (received by the device). Each stream can contain different operations and a schedule.
 *
 * A stream is created with a defined type. The stream-type defines the capabilities of the stream.
 * Each stream type can only support specific operations or schedule entries.
 * Defining a stream reference or adding the stream to a module will also move the ownership to the
 * parent object. In case of destroying a configuration it is sufficient to destroy the parent object.
 * Destroying a stream which has a defined ownership is permitted and will have no effect on the configuration.
 *
 *     *~~~~~~~~*
 *     | module |
 *     *~~~~~~~~*
 *      |
 *      |
 *      | *~~~~~~~~~~~~~~~~~~~~~~~*
 *      |–| time-triggered-stream |
 *      | *~~~~~~~~~~~~~~~~~~~~~~~*
 *      |     |– operations (Insert, Insertconstant, Pad)
 *      |     |– schedule-events
 *      |
 *      | *~~~~~~~~~~~~~~~~~~~~~~~~~~*-------*~~~~~~~~~~~~~~*------------*~~~~~~~~~~~~~~~~~*
 *      |–| ingress-triggered-stream |       | event-stream |            | recovery-stream |
 *        *~~~~~~~~~~~~~~~~~~~~~~~~~~*-------*~~~~~~~~~~~~~~*------------*~~~~~~~~~~~~~~~~~*
 *            |– operations (Read, Forwardall)    |– operations (Insert     |– operations (Insert
 *            |– schedule-windows                      , Insertconstant            , Insertconstant, Pad)
 *                                                     , Pad, Forward)
 *
 */

/**
 * @defgroup acmoperation ACM Operation
 * @brief ACM Operations
 *
 * Operations allow to define the data of the egress frame or which data shall be
 * read from the ingress frame. In an egress stream the operations must be added in
 * the correct order. The first operation will be effective after the 16th
 * byte of the frame. The first 16 bytes are defined by the stream header information.
 * This includes the destination-MAC, source-MAC and VLAN-tag.
*/

/**
 * @defgroup acmschedule ACM Schedule
 * @brief ACM Schedule entries
 *
 * The schedule can be defined for a module (setting a cycle time and a start-time) and
 * for a stream. A time-triggered stream contains transmission events defining when a
 * frame shall be sent. The ingress-triggered-stream contains scheduling windows. In each window
 * the frame will be allowed to be received. If no scheduling entry is defined for a stream
 * the stream will not be active in the configuration.
 * All schedule entries can be removed again from a stream. It is allowed to
 * manipulate the schedule after the configuration had been applied. The schedule
 * might be changed during execution by applying a new schedule. This will not effect
 * the configuration.
*/

/**
 * @defgroup acmvalidate ACM Validation
 * @brief ACM Validation
 *
 * Validation interface to verify if all checks pass and the configuration can be applied to the
 * device
*/

/**@}*/

/**@}*/


/**
 * @ingroup acmstatusarea
 * @brief Read a specific status item of the defined module
 *
 * @param module_id identifier of the module to read the status from.
 * @param status_id status value to read
 *
 * @return value of status item. Negative values represent an error.
 */
int64_t __must_check acm_read_status_item(enum acm_module_id module_id,
        enum acm_status_item status_id);

/**
 * @ingroup acmstatusarea
 * @brief Read ACM configuration id
 *
 * @return value of configuration identifier. Negative values represent an error.
 */
int64_t __must_check acm_read_config_identifier();

/**
 * @ingroup acmdiagarea
 * @brief Read a diagnostic data of a specific module
 *
 * @param module_id identifier of the module to read the diagnostic data from.
 *
 * @return diagnostic counter values. The values are provided as acm_diagnostics struct
 * containing the complete diagnostic dataset of the module. NULL will be returned in
 * case of an error.
 */
struct acm_diagnostic* __must_check acm_read_diagnostics(enum acm_module_id module_id);

/**
 * @ingroup acmdiagarea
 * @brief Set the diagnostic polling interval
 *
 * The diagnostic counters need to be updated internally with a defined interval. If the
 * interval is too long, the counters might overrun. Per default a interval of 50ms is defined
 * which allows minimal schedule cycles of 200us.
 *
 * @param module_id identifier of the module to set the diagnostic interval to.
 * @param interval_ms interval in milliseconds. A value of 0 will disable the mechanism.
 *
 * @return 0 will be returned in case of success. Negative values represent an error.
 */
int __must_check acm_set_diagnostics_poll_time(enum acm_module_id module_id, uint16_t interval_ms);

/**
 * @ingroup acmcapability
 * @brief Read capabilities of the device
 *
 * @param item_id identifier of the capability item to be read
 *
 * @return capability item value. Negative values represent an error.
 */
int32_t __must_check acm_read_capability_item(enum acm_capability_item item_id);

/**
 * @ingroup acmcapability
 * @brief Read the current library version
 *
 * @return version of the library. The version will be seperated in
 * major.minor.patch version.
 *     - major: representing interface changes
 *     - minor: representing function changes of the library
 *     - patch: representing bug-fixes
 */
const char* __must_check acm_read_lib_version();

/**
 * @ingroup acmcapability
 * @brief Read the current IP version
 *
 * @return version of the library. The version will be seperated in
 * <device-id>-<major.minor.patch>-<revision-id>.
 * 	   - device-id: representing device ID in hex format
 *     - major: representing interface changes
 *     - minor: representing function changes of the library
 *     - patch: representing bug-fixes
 *     - revision-id: representing revision ID in hex format
 */
char* __must_check acm_read_ip_version();

/**
 * @ingroup acmcontrol
 * @brief Get buffer-id by buffer-name
 *
 * @param buffer buffer name of an already applied configuration
 *
 * @return message-buffer id. Negative values represent an error.
 */
int __must_check acm_get_buffer_id(const char* buffer);

/**
 * @ingroup acmcontrol
 * @brief Read the status of the message buffer locking vector
 *
 * The vector is a bitmask. Each set bit represents a locked buffer.
 */
int64_t __must_check acm_read_buffer_locking_vector();

/**
 * @ingroup acmcontrol
 * @brief Lock multiple message buffers
 *
 * Multiple buffers can be locked at the same time. As long as a buffer is locked
 * data will not be updated.
 *
 * @param locking_vector Each bit represents a message buffer. If the bit is set, the
 * related buffer will be locked.
 */
int __must_check acm_set_buffer_locking_mask(uint64_t locking_vector);

/**
 * @ingroup acmcontrol
 * @brief Unlock multiple message buffers
 *
 * Multiple buffers can be unlocked at the same time. As long as a buffer is locked
 * data will not be updated.
 *
 * @param unlocking_vector Each bit represents a message buffer. If the bit is set, the
 * related buffer will be unlocked.
 */
int __must_check acm_set_buffer_unlocking_mask(uint64_t unlocking_vector);

/**
 * @ingroup acmoperation
 * @brief Add an "insert"-operation to a stream
 *
 * The operation will insert data from a defined message buffer
 * (created as character device with defined name) into the frame.
 * The operation is allowed for time-triggered, event and recovery frame-types.
 *
 * @param stream stream where to add the operation to
 * @param length number of bytes to insert from the buffer
 * @param buffer name of the character device. The device will be created when applying
 * the configuration.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_operation_insert(struct acm_stream *stream,
        uint16_t length,
        const char *buffer);

/**
 * @ingroup acmoperation
 * @brief Add an "insertconstant"-operation to a stream
 *
 * The operation will insert data from a defined data-stream into the frame.
 * The operation is allowed for time-triggered, event and recovery frame-types.
 * The data can only be defined during the configuration of the device and cannot be
 * changed during operation.
 *
 * @param stream stream where to add the operation to
 * @param content data to be inserted into the frame
 * @param size size of the data
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_operation_insertconstant(struct acm_stream *stream,
        const char *content,
        uint16_t size);

/**
 * @ingroup acmoperation
 * @brief Add an "pad"-operation to a stream
 *
 * The operation will insert padding-bytes with a defined pattern into the frame.
 * The operation is allowed for time-triggered, event and recovery frame-types.
 *
 * @param stream stream where to add the operation to
 * @param length number of bytes to padded in the frame
 * @param value padding value used to insert(0-255)
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_operation_pad(struct acm_stream *stream,
        uint16_t length,
        const char value);

/**
 * @ingroup acmoperation
 * @brief Add an "forward"-operation to a stream
 *
 * The operation will forward data from the ingress frame to the egress frame.
 * The operation is allowed for event-streams only.
 *
 * @param stream stream where to add the operation to
 * @param offset offset from start-of-frame of the ingress frame. Maximal 19 bytes can
 * be skipped when defining the offset, i.e. when the operation shall insert data to the
 * egress frame on byte-position 20 it is possible to forward from an ingress offset of
 * 39. Skipping more bytes is not possible as the data would not have been received by the
 * device yet.
 * @param length number of bytes to be forwarded
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_operation_forward(struct acm_stream *stream,
        uint16_t offset,
        uint16_t length);

/**
 * @ingroup acmoperation
 * @brief Add an "read"-operation to a stream
 *
 * The operation will read data from the ingress frame to a defined character device.
 * The operation is allowed for ingress-triggered-streams only.
 *
 * @param stream stream where to add the operation to
 * @param offset offset from start-of-frame of the ingress frame.
 * @param length number of bytes to be read to the character device
 * @param buffer name of the character device. The device will be created when applying
 * the configuration.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_operation_read(struct acm_stream *stream,
        uint16_t offset,
        uint16_t length,
        const char *buffer);

/**
 * @ingroup acmoperation
 * @brief Add an "forwardall"-operation to a stream
 *
 * The operation will forward the complete ingress frame without manipulation.
 * The operation is allowed for ingress-triggered-streams only. If the operation
 * is defined in the ingress-triggered-stream it is permitted to define an event-stream
 * in addition.
 *
 * @param stream stream where to add the operation to
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_operation_forwardall(struct acm_stream *stream);

/**
 * @ingroup acmoperation
 * @brief Remove all configurations from a stream
 *
 * All operations can be removed again from a stream. This operation is only allowed
 * as long as the configuration is not applied.
 *
 * @param stream stream where the operations shall be removed from.
 */
void acm_clean_operations(struct acm_stream *stream);

/**
 * @ingroup acmschedule
 * @brief Add a scheduling event to a stream
 *
 * A schedule event can only be added to a time-triggered stream. A stream can have
 * multiple events defined. Each entry consists of a period and a send-time.
 *
 * @param stream stream where the schedule-event shall be added to.
 * @param period_ns the period of the event can be defined in nano seconds.
 * The period must be a fraction of the cycle defined for the module.
 * @param send_time_ns the time is defined in nano seconds and represents the time
 * when the frame shall be on the wire.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_schedule_event(struct acm_stream *stream,
        uint32_t period_ns,
        uint32_t send_time_ns);

/**
 * @ingroup acmschedule
 * @brief Add a scheduling window to a stream.
 *
 * A schedule window can only be added to a ingress-triggered stream. A stream can have
 * multiple windows. Each entry consists of a period, a start-time and an end-time.
 *
 * @param stream stream where the schedule-event shall be added to.
 * @param period_ns the period of the event can be defined in nano seconds.
 * The period must be a fraction of the cycle defined for the module.
 * @param time_start_ns Defines the time in nano seconds when the window will be opened.
 * The window represents the time when the frame is entering the device.
 * @param time_end_ns Defines the time in nano seconds when the window will be closed.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_stream_schedule_window(struct acm_stream *stream,
        uint32_t period_ns,
        uint32_t time_start_ns,
        uint32_t time_end_ns);
/**
 * @ingroup acmschedule
 * @brief Remove all schedule entries from a stream
 *
 * @param stream stream where the schedule shall be removed from.
 */
void acm_clean_schedule(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief create a time-triggered stream
 *
 * @param dmac destination MAC address of the stream
 * @param smac source MAC address of the stream
 *	   - If the source MAC is 00-00-00-00-00-00 the MAC of the port will be inserted
 * @param vlan_id VLAN id to be inserted (range 3-4094)
 * @param vlan_priority VLAN priority to be inserted (range 0-7)
 *
 * @return handle the the stream configuration
 */
struct acm_stream* __must_check acm_create_time_triggered_stream(uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan_id,
        uint8_t vlan_priority);

/**
 * @ingroup acmstream
 * @brief create a ingress-triggered stream
 *
 * @param header header-pattern of the ingress frame (16 bytes)
 * @param header_mask header-mask to define which parts shall be filtered from the ingress frame

 * @param additional_filter pattern of the additional filter (up to 112 bytes)
 * @param additional_filter_mask mask to define which parts of the filter will be used
 * @param size size of the additional filter
 *
 * @return handle the the stream configuration
 */
struct acm_stream* __must_check acm_create_ingress_triggered_stream(uint8_t header[ACM_MAX_LOOKUP_SIZE],
        uint8_t header_mask[ACM_MAX_LOOKUP_SIZE],
        uint8_t *additional_filter,
        uint8_t *additional_filter_mask,
        uint16_t size);

/**
 * @ingroup acmstream
 * @brief create a event stream
 *
 * @param dmac destination MAC address of the stream
 * 	  - If the destination MAC is FF-FF-FF-FF-FF-FF the MAC of the ingress-frame will be forwarded
 * @param smac source MAC address of the stream
 *    - If the source MAC is 00-00-00-00-00-00 the MAC of the port will be inserted
 *    - If the source MAC is FF-FF-FF-FF-FF-FF the MAC of the ingress-frame will be forwarded
 * @param vlan_id VLAN id to be inserted (range 3-4094)
 *    - If the vlan-id is 4095 the vlan tag (id + priority) of the ingress frame will be forwarded
 * @param vlan_priority VLAN priority to be inserted (range 0-7)
 *
 * @return handle the the stream configuration
 */
struct acm_stream* __must_check acm_create_event_stream(uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan_id,
        uint8_t vlan_priority);

/**
 * @ingroup acmstream
 * @brief create a recovery stream
 *
 * @param dmac destination MAC address of the stream
 * @param smac source MAC address of the stream
 *	   - If the source MAC is 00-00-00-00-00-00 the MAC of the port will be inserted
 * @param vlan_id VLAN id to be inserted (range 3-4094)
 * @param vlan_priority VLAN priority to be inserted (range 0-7)
 *
 * @return handle the the stream configuration
 */
struct acm_stream* __must_check acm_create_recovery_stream(uint8_t dmac[ETHER_ADDR_LEN],
        uint8_t smac[ETHER_ADDR_LEN],
        uint16_t vlan_id,
        uint8_t vlan_priority);

/**
 * @ingroup acmstream
 * @brief destroy a stream
 *
 * It is not permitted to destroy a stream which is added to a module. It is
 * not permitted to destroy an Event Stream or a Recovery Stream, if the Stream is
 * referenced to another stream.
 * The function call will have no effect in these cases.
 * It is permitted to destroy a Time Triggered Stream, if the Stream has a reference to
 * another Time Triggered Stream (=Redundant Stream). In this case the referenced Time
 * Triggered (Redundant) Stream shall not be deleted but shall be reconverted to a
 * normal Time Triggered Stream.
 *
 * @param stream stream to be destroyed
 */
void acm_destroy_stream(struct acm_stream *stream);

/**
 * @ingroup acmstream
 * @brief create a reference between two streams
 *
 * Different streams can be referenced to define additional behaviors.
 *	   - Ingress-triggered to event stream: Received frame will be manipulated and forwarded
 *	   - event to recovery stream: the recovery stream will be generated in case the ingress stream did not arrive
 *	   - time-triggered to time-triggered stream: Generate both streams as redundant streams.
 *       The two streams must be added to different modules.
 *
 * @param stream parent-stream item
 * @param reference child-stream item
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_set_reference_stream(struct acm_stream *stream,
		struct acm_stream *reference);

/**
 * @ingroup acmstream
 * @brief Set redundancy tag for an ingress triggered stream.
 *
 * This function defines that for the given ingress triggered stream, a redundancy tag is to be
 * expected. Individual recovery will be enabled for this stream.
 * This function will fail if the given stream is not of type ingress triggered.
 *
 * @param stream stream item
 * @param timeout_ns timeout value in nano seconds
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_set_rtag_stream(struct acm_stream *stream,
		uint32_t timeout_ns);

/**
 * @ingroup acmmodule
 * @brief create a module
 *
 * The module contains a list of streams. Each module defines a communication
 * direction. Maximal two modules can be added to an ACM configuration instance.
 *
 * @param mode connection mode of the module (serial or parallel mode)
 * @param speed connection speed the module should operate in.
 * @param module_id The module id defines the communication direction.
 *	   - MODULE_0 will allow streams from port sw0p3 to port sw0p2
 *	   - MODULE_1 will allow streams from port sw0p2 to port sw0p3
 *
 * @return handle to the module configuration
 */
struct acm_module* __must_check acm_create_module(enum acm_connection_mode mode,
		enum acm_linkspeed speed, enum acm_module_id module_id);

/**
 * @ingroup acmmodule
 * @brief destroy a module
 *
 * It is not permitted to destroy a module which is added to a configuration.
 * The function call will have no effect in this case.
 *
 * @param module module configuration to be destroyed
 */
void acm_destroy_module(struct acm_module *module);

/**
 * @ingroup acmschedule
 * @brief Set a schedule for a module
 *
 * @param module module where to set the schedule too
 * @param cycle_ns schedule cycle duration in nano-seconds
 * @param start start-time of the configuration
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_set_module_schedule(struct acm_module *module,
		uint32_t cycle_ns, struct timespec start);

/**
 * @ingroup acmmodule
 * @brief Add a stream to a module
 *
 * @param module module where to add the stream configuration too
 * @param stream stream configuration to be added to a module
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check acm_add_module_stream(struct acm_module *module,
		struct acm_stream *stream);

/**
 * @ingroup acmconfig
 * @brief create a new configuration
 *
 * Create a new ACM configuration object.
 */
struct acm_config* __must_check acm_create();

/**
 * @ingroup acmconfig
 * @brief Destroy a configuration
 *
 * The complete configuration with all linked items will be destroyed
*/
void acm_destroy(struct acm_config *config);

/**
 * @ingroup acmconfig
 * @brief Add a module to a configuration
 *
 * @param config ACM configuration where to add the module config too
 * @param module module configuration to be added to a config item
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_add_module(struct acm_config *config,
		struct acm_module *module);


/**
 * @ingroup acmvalidate
 * @brief Validate a stream
 *
 * The validation will check if the configured stream is valid.
 *
 * @param stream stream to be validated.
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_validate_stream(struct acm_stream *stream);

/**
 * @ingroup acmvalidate
 * @brief Validate a module
 *
 * The validation will check if the configured module configuration is valid.
 *
 * @param module module to be validated.
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_validate_module(struct acm_module *module);

/**
 * @ingroup acmvalidate
 * @brief Validate a configuration
 *
 * The validation will check if the ACM configured is valid.
 *
 * @param config ACM configuration to be validated.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_validate_config(struct acm_config *config);

/**
 * @ingroup acmconfig
 * @brief Apply a complete configuration
 *
 * The old configuration of the device will be replaced by the new configuration
 *
 * @param config ACM configuration to be applied to the device
 * @param identifier configuration id for verification in case of schedule change
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_apply_config(struct acm_config *config,
		uint32_t identifier);

/**
 * @ingroup acmschedule
 * @brief Apply a new schedule
 *
 * The old configuration of the device will no be changed. Only the schedule is updated.
 *
 * @param config ACM configuration to be applied to the device
 * @param identifier configuration id for verification in case of schedule change
 * @param identifier_expected expected configuration id of the device.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_apply_schedule(struct acm_config *config,
		uint32_t identifier, uint32_t identifier_expected);

/**
 * @ingroup acmconfig
 * @brief Disable a configuration
 *
 * The old configuration will be disabled.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
*/
int __must_check acm_disable_config(void);

#ifdef __cplusplus
} /* extern "C" */
#endif	/* __cplusplus */

#endif /* LIBACMCONFIG_H_ */
