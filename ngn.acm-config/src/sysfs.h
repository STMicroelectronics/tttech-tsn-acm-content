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
 * @file sysfs.h
 *
 * HW accesses of ACM library - interface to acm driver
 *
 */

#ifndef SYSFS_H_
#define SYSFS_H_

#include <linux/acm/acmdrv.h>

#include "libacmconfig_def.h"
#include "hwconfig_def.h"
#include "config.h"
#include "module.h"
#include "buffer.h"

/**
 * @brief For test purposes static functions are declared as non static
 * @{
 */
#ifndef TEST
#define STATIC static
#else
#define STATIC

int __must_check write_buffer_config_sysfs_item(const char *file_name, char *buffer,
        int32_t buffer_length, int32_t offset);
int __must_check update_fsc_indexes(struct fsc_command *fsc_command_item);
int __must_check write_fsc_nop_command(int fd, uint32_t delta_cycle, enum acm_module_id module_index,
        int table_index, int item_index);
#endif
/** @} */

/**
 * @brief Definition of filenames of acm system files of configuration group
 * @{
 */
#define ACM_SYSFS_SCHED_TAB	sched_tab_row
#define ACM_SYSFS_CONFIG_ID configuration_id
#define ACM_SYSFS_SCHED_CYCLE sched_cycle_time
#define ACM_SYSFS_SCHED_START sched_start_table
#define ACM_SYSFS_SCHED_STATUS table_status
#define ACM_SYSFS_EMERGENCY emergency_disable
#define ACM_SYSFS_CONN_MODE cntl_connection_mode
#define ACM_SYSFS_CONFIG_STATE config_state
#define ACM_SYSFS_MODULE_ENABLE cntl_ngn_enable
#define ACM_SYSFS_CLEAR_ALL_FPGA clear_all_fpga

// cntl_gather_delay - only used driver internally
#define ACM_SYSFS_INGRESS_CONTROL cntl_ingress_policing_control
#define ACM_SYSFS_INGRESS_ENABLE cntl_ingress_policing_enable
#define ACM_SYSFS_LAYER7_ENABLE cntl_layer7_enable
#define ACM_SYSFS_LAYER7_LENGTH cntl_layer7_length
#define ACM_SYSFS_LOOKUP_ENABLE cntl_lookup_enable
// cntl_output_disable - not used
#define ACM_SYSFS_SPEED cntl_speed
#define ACM_SYSFS_CONST_BUFFER const_buffer
#define ACM_SYSFS_STREAM_TRIGGER stream_trigger
#define ACM_SYSFS_LAYER7_MASK layer7_mask
#define ACM_SYSFS_LAYER7_PATTERN layer7_pattern
#define ACM_SYSFS_LOOKUP_MASK lookup_mask
#define ACM_SYSFS_LOOKUP_PATTERN lookup_pattern
#define ACM_SYSFS_SCATTER scatter_dma
#define ACM_SYSFS_GATHER gather_dma
#define ACM_SYSFS_PREFETCH prefetch_dma
#define ACM_SYSFS_REDUND_CONTR redund_cnt_tab
#define ACM_SYSFS_BASE_RECOV base_recovery
#define ACM_SYSFS_INDIV_RECOV individual_recovery

// message buffers
#define ACM_SYSFS_MSGBUFF_ALIAS msg_buff_alias
#define ACM_SYSFS_MSGBUFF_DESC msg_buff_desc
/** @} */

/**
 * @brief Definition of filenames of acm system files of control group
 * @{
 */
#define ACM_SYSFS_LOCK_BUFFMASK lock_msg_bufs
#define ACM_SYSFS_UNLOCK_BUFFMASK unlock_msg_bufs
/** @} */

/**
 * @brief maximum lookup items at ingress
 */
#define MAX_LOOKUP_TRIGGER_ITEMS (ACM_MAX_LOOKUP_ITEMS + 1)

/**
 * @brief in lookup table items of configuration can be written from the first item of the table
 */
#define LOOKUP_START_IDX 0

/**
 * @brief in redundancy table items of configuration can be written from the second item of the table
 */
#define REDUNDANCY_START_IDX 1

/**
 * @brief in scatter table items of configuration can be written from the second item of the table
 * @{
 */
#define SCATTER_NOP_IDX 0
#define SCATTER_START_IDX 1
/** @} */

/**
 * @brief in scatter table items of configuration can be written from the third item of the table
 * @{
 */
#define GATHER_NOP_IDX 0
#define GATHER_FORWARD_IDX 1
#define GATHER_START_IDX 2
/** @} */

/**
 * @brief number of commands necessary for prefetch lock vector
 */
#define NUM_PREFETCH_LOCK_COMMANDS 4U

/**
 * @brief structure which holds all the relevant data of an fsc_command for writing to HW
 */
struct sched_tbl_row {
    uint32_t cmd; /**< row command prepared for writing to hardware*/
    uint32_t abs_cycle; /**< absolute value of time in cycle */
};

/**
 * @brief structure which holds all the list of fsc_commands of a module
 */
struct fsc_command {
    struct sched_tbl_row hw_schedule_item;/**< schedule table item */
    struct schedule_entry *schedule_reference; /**< reference to original schedule entry in stream*/
    ACMLIST_ENTRY(fsc_command_list, fsc_command) entry;  /**< links to next and previous fsc_command and fsc list header in the module */
};

/**
 * @brief helper for initializing a fsc-command - especially for module test
 */
#define COMMAND_INITIALIZER(_cmd, _abs_cycle)            \
{                                      \
    .hw_schedule_item = { _cmd, _abs_cycle },       \
    .schedule_reference = NULL,                     \
    .entry = ACMLIST_ENTRY_INITIALIZER              \
}

/** @brief Macros for divide and round
 *
 * The Macros are defined to avoid including the math library which
 * contains the "round" function.
 * @{
 */
#define DIV_ROUND_CLOSEST(x, divisor)(          \
{                                               \
        (((x) > 0) == ((divisor) > 0)) ?        \
        (((x) + ((divisor) / 2)) / (divisor)) : \
        (((x) - ((divisor) / 2)) / (divisor));  \
}                                               \
)
#define DIV_ROUND_UP(x, divisor) (((x) + (divisor) - 1) / (divisor))
/** @} */

/** @brief Read char data from acm filesystem
 *
 * The function reads data (char) from the file specified in
 * parameter path_name and writes it to the address where parameter buffer
 * points to.
 *
 * @param path_name path and filename where to read data from.
 * @param buffer contains the value read from filesystem if function was
 * 					successful
 * @param buffer_length number of characters to be read
 * @param offset offset where to start to read
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check read_buffer_sysfs_item(const char *path_name,
        void *buffer,
        size_t buffer_length,
        off_t offset);

/** @brief Write data to acm filesystem
 *
 * The function writes data (char) from buffer to the file specified in
 * parameter path_name. As start position for writing value of parameter offset is taken. The
 * function writes buffer_length bytes.
 *
 * @param path_name path and filename where to write data to.
 * @param buffer contains the data to be written
 * @param buffer_length number of characters to be written
 * @param offset offset where to start to write
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check write_file_sysfs(const char *path_name,
        void *buffer,
        size_t buffer_length,
        off_t offset);

/** @brief Delete content of a file in  acm filesystem
 *
 * The function deletes the content of the file specified in parameter path_name. Afterwards the
 * file still exists but is empty.
 *
 * @param path_name path and filename of file to empty.
 */
void sysfs_delete_file_content(const char *path_name);

/** @brief Write data to acm filesystem
 *
 * The function compounds the pathname of a file in acm filesystem out of the base directory name
 * the group name and the filename.
 *
 * @param path_name complete path of file after application of function
 * @param length number of characters available in memory where path_name points to
 * @param group group directory
 * @param file filename
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check sysfs_construct_path_name(char *path_name,
        size_t length,
        const char *group,
        const char *file);


/** @brief Read 64 bit integer value from acm filesystem
 *
 * The function reads data (ASCII characters) from the file specified in
 * parameter path_name and converts it to an integer value.
 *
 * @param path_name path and filename where to read data from.
 *
 * @return Integer value read from file. Negative values represent an error.
 */
int64_t __must_check read_uint64_sysfs_item(const char *path_name);

/**
 * @brief Creates fsc schedule commands from event schedules
 *
 * The function creates all the fsc schedule commands for a schedule item for the
 * module cycle and inserts them into the sorted fsc command list.
 *
 * @param schedule_item address of the acm schedule item
 * @param module address of the module to which the schedule item was added
 * @param tick_duration number of nanoseconds a tick lasts
 * @param gather_dma_index index of the first operation of the stream the
 * 			schedule item belongs to in the gather DMA
 * @param redundand_index index in the redundancy table, if the stream the
 * 			schedule belongs to is a redundant stream. Index is 0, if stream
 * 			is not a redundant stream.
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check create_event_sysfs_items(struct schedule_entry *schedule_item,
        struct acm_module *module,
        int tick_duration,
        uint16_t gather_dma_index,
        uint8_t redundand_index);

/**
 * @brief Creates fsc schedule commands from schedule window
 *
 * The function creates all the fsc schedule commands (start and end) for a
 * schedule item for the module cycle and inserts them into the sorted fsc
 * command list.
 *
 * @param schedule_item address of the acm schedule item
 * @param module address of the module to which the schedule item was added
 * @param tick_duration number of nanoseconds a tick lasts
 * @param gather_dma_index index of the first operation of the stream the
 * 			schedule item belongs to in the gather DMA (only if recovery is true)
 * @param lookup_index index in the lookup table. (This value is inserted into
 * 			window start and window end fsc commands
 * @param recovery if true (for existing recovery stream) gather_dma index is
 * 			set in window end fsc command
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check create_window_sysfs_items(struct schedule_entry *schedule_item,
        struct acm_module *module,
        int tick_duration,
        uint16_t gather_dma_index,
        uint8_t lookup_index,
        bool recovery);

/**
 * @brief Adds a fsc schedule command to the list of fsc schedule commands
 *
 * The function inserts a fsc schedule item to the sorted list of fsc commands.
 * The items are sorted according value of delta_cycle of fsc command.
 *
 * @param fsc_list address of the fsc command list
 * @param fsc_schedule address of the fsc command
 *
 */
void add_fsc_to_module_sorted(struct fsc_command_list *fsc_list, struct fsc_command *fsc_schedule);

/**
 * @brief Writes fsc schedule commands to hardware
 *
 * The function iterates through all fsc commands of the module and writes them to the specified
 * schedule table. If necessary the function inserts nop commands at the beginning of the table
 * or between two fsc command. The function also adds a waiting time of ANZ_MIN_TICKS to the last
 * fsc command.
 *
 * @param module address of the module which schedule items shall be written
 * @param table_index index of the HW schedule table where data shall be written to
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check write_fsc_schedules_to_HW(struct acm_module *module, int table_index);

/**
 * @brief Write module schedule data to hardware
 *
 * The function writes the schedule period of the module and the schedule start time to the
 * schedule table with index table_index.
 *
 * @param module address of the module which schedule items shall be written
 * @param table_index index of the HW schedule table where data shall be written to
 *
 * @return The function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check write_module_schedules_to_HW(struct acm_module *module, int table_index);

/**
 * @brief Find free schedule table
 *
 * The function reads the schedule status from hardware. Then it iterates through the status items
 * and checks if the schedule table is free. When it finds a free table, it writes the index of
 * the table to parameter free_table and returns 0.
 *
 * @param module address of the module which schedule items shall be written
 * @param free_table in case of success index of the HW schedule table which is free and can be
 *          used for a new schedule.
 *
 * @return the function will return 0 in case of success. Negative values represent
 * an error.
 */
int __must_check sysfs_read_schedule_status(struct acm_module *module, int *free_table);

/**
 * @brief Read configuration id from hardware
 *
 * The function reads the configuration id of the configuration currently applied to hardware.
 *
 * @return The function will return the configuration id in case of success. Negative values
 * represent an error.
 */
int __must_check sysfs_read_configuration_id(void);

/**
 * @brief Write a configuration id to hardware
 *
 * The function writes the configuration id in parameter identifier to the provided file in acm
 * filesystem.
 * Typically this function is called at the end of the application of a new configuration or a new
 * schedule.
 *
 * @param identifier configuration id to be written to hardware
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_configuration_id(int32_t identifier);

/**
 * @brief Write configuration status to hardware
 *
 * The function writes the configuration status in parameter status to the provided file in acm
 * filesystem.
 * Typically this function is called at start and at end of the application of a new configuration.
 *
 * @param status configuration status value to be written to hardware
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_config_status_to_HW(enum acmdrv_status status);

/**
 * @brief Write emergency disable value to hardware
 *
 * The function writes the emergency disable value in parameter value to the provided file in acm
 * filesystem.
 *
 * @param module address of the module which emergency disable value shall be written
 * @param value emergency disable value to be written to hardware
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_emergency_disable(struct acm_module *module,
        struct acmdrv_sched_emerg_disable *value);

/**
 * @brief Write connection mode to hardware
 *
 * The function writes the connection mode of the module to the provided file in acm
 * filesystem.
 *
 * @param module address of the module which connection mode value shall be written
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_connection_mode_to_HW(struct acm_module *module);

/**
 * @brief Write module enable value to hardware
 *
 * The function writes the module enable value in parameter enable to the provided file in acm
 * filesystem.
 *
 * @param module address of the module which module enable value shall be written
 * @param enable module enable value to be written to hardware
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_module_enable(struct acm_module *module, bool enable);

/**
 * @brief Write constant data of module to hardware
 *
 * The function iterates through all operations of all streams of the module. For each operation
 * with operation code INSERT_CONSTANT it writes the constant data from the operation to the
 * provided file in acm filesystem.
 * The function also determines the offset of the constant operation data in the constant buffer
 * on hardware and stores the offset in the operation data.
 *
 * @param module address of the module which constant data shall be written
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_data_constant_buffer(struct acm_module *module);

/**
 * @brief Write lookup tables of module to hardware
 *
 * The function iterates through all streams of the module. For each INGRESS_TRIGGERED_STREAM and
 * each REDUNDANT_STREAM_RX it writes the lookup data (layer 7 mask, layer 7 pattern, lookup mask,
 * lookup pattern, lookup trigger) to the hardware.
 * Afterwards the function also writes the lookup control block and rule 16 for lookup trigger
 * table.
 *
 * @param module address of the module which lookup tables shall be written
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_lookup_tables(struct acm_module *module);

/**
 * @brief writes the scatter DMA table to hardware
 *
 * The function writes a NOP item to position 0 of the table, and then it
 * iterates through all streams of the module. For each stream of type
 * INGRESS_TRIGGERED_STREAM it writes all READ operation items to the hardware.
 * The function cares to set the "last" bit for the last READ operation of a
 * stream.
 * The message buffer index is taken from the operation data. Therefore it is
 * important that the message buffer data have been set correctly in the
 * operations and message buffers before the function is called.
 *
 * @param module address of the module for which the scatter DMA table is written
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check sysfs_write_scatter_dma(struct acm_module *module);

/**
 * @brief writes the gather DMA table and the prefetch table to hardware
 *
 * The function first writes writes the general commands to both tables:
 * gather DMA table:
 * 		gather NOP command to table row zero
 * 		gather forward all command to table row 1
 * prefetch table:
 * 		prefetch NOP command to table rows zero and 1
 * Then the function iterates through all streams of the module. For each stream
 * it calls a subfunction to write all the gather and prefetch commands for the
 * stream to the hardware. (Different functions for ingress streams and egress
 * streams.)
 *
 * @param module address of the module for which the gather DMA table and
 * 				prefetch table are written
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check sysfs_write_prefetcher_gather_dma(struct acm_module *module);

/**
 * @brief writes the gather DMA commands for ingress triggered streams to hardware
 *
 * The function checks if the ingress triggered stream contains a forward all
 * operation. If this is the case, then a forward all command is written to
 * gather DMA table.
 *
 * @param gather_index  index value within the gather DMA table where the gather
 * 				DMA commands of the stream start; The function uses this value
 * 				as write position for the first gather DMA operation it writes.
 * @param module_id id of the module to which the stream was added
 * @param stream address the Ingress Triggered Stream for which gather DMA
 * 				commands shall be written
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check write_gather_ingress(int gather_index,
        uint32_t module_id,
        struct acm_stream *stream);

/**
 * @brief writes the gather DMA commands and prefetch commands for an egress
 * 		triggered streams to hardware
 *
 * The function iterates through all operations of the stream. For each
 * operation it writes the corresponding gather DMA command to the gather DMA
 * table. For insert operation it also writes the corresponding command to the
 * prefetch table and sets the corresponding bit for the associated message
 * buffer in the prefetch lock vector, which is also written to hardware after
 * all insert commands of the stream were handled. If a stream doesn't contain
 * an insert operation, the function writes a NOP command to the prefetch table.
 * As starting index in the prefetch table for the stream is the same value used
 * as for the gather DMA table. The prefetch lock vector is written first and
 * then the commands for the insert operations are written. The prefetch
 * commands for the insert operations are not written at the same index as the
 * insert command in the gather DMA table.
 *
 * @param gather_index  index value within the gather DMA table where the gather
 * 				DMA commands of the stream start; The function uses this value
 * 				as write position for the first gather DMA operation it writes.
 * @param module_id id of the module to which the stream was added
 * @param stream address the egress stream for which gather DMA commands and
 * 				prefetch commands shall be written
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check write_gather_egress(int gather_index,
        uint32_t module_id,
        struct acm_stream *stream);

/**
 * @brief writes the redundancy table to hardware
 *
 * The function writes a NOP item to position 0 of the table, and then it
 * iterates through all streams of the module. For each stream of type
 * REDUNDANT_STREAM_TX it writes the redundancy table item to the hardware.
 * The redundancy table index is taken from the stream data. Therefore it is
 * important that the table indexes have been set correctly in the streams
 * before the function is called.
 *
 * @param module address of the module for which the redundancy table is written
 *
 * @return the function will return 0 in case of success. Negative values
 * represent an error.
 */
int __must_check sysfs_write_redund_ctrl_table(struct acm_module *module);

/**
 * @brief Write lookup control block to hardware
 *
 * The function writes the data for lookup control block of the module to the provided file in acm
 * filesystem.
 *
 * @param module_id id of the module which lookup control block shall be written
 * @param ingress_control
 * @param lookup_enable
 * @param layer7_enable
 * @param layer7_len
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_lookup_control_block(uint32_t module_id,
        uint16_t ingress_control,
        uint16_t lookup_enable,
        uint16_t layer7_enable,
        uint8_t layer7_len);

/**
 * @brief Write linkspeed to hardware
 *
 * The function writes the configured linkspeed of the module to the provided file in acm
 * filesystem.
 *
 * @param module address of the module which linkspeed value shall be written
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_cntl_speed(struct acm_module *module);

/**
 * @brief Check message buffer name
 *
 * The function checks if the message buffer name in parameter buffer_name begins with the
 * configured praefix (default praefix if not configured).
 *
 * @param buffer_name address of the buffer name to check
 *
 * @return The function will return 0 in case the buffer name is okay. Negative values indicate
 *      that the buffer name is not okay.
 */
int __must_check check_buff_name_against_sys_devices(const char *buffer_name);

/**
 * @brief Write message buffer data to hardware
 *
 * Depending on the value of parameter buff_table the function writes the data of all message
 * buffers in bufferlist to the hardware.
 *
 * @param bufferlist pointer to the list of message buffers for which data shall be written to HW
 * @param buff_table specifies which of the buffer tables shall be written: buffer descriptors
 *          table or buffer aliases table
 *
 * @return The function will return 0 in case of success. Negative values indicate an error.
 */
int __must_check sysfs_write_msg_buff_to_HW(struct buffer_list *bufferlist,
        enum buff_table_type buff_table);

/**
 * @brief Write buffer locking mask to hardware
 *
 * The function writes the locking mask in parameter vector to file with name filename in acm
 * filesystem.
 *
 * @param vector bit array for buffer locking
 * @param filename name of file in acm filesystem where locking vector shall be written to
 *
 * @return The function will return 0 in case of success. Negative values indicate an error.
 */
int __must_check sysfs_write_buffer_control_mask(uint64_t vector, const char *filename);

/**
 * @brief Read MAC address of a port
 *
 * The function reads the MAC address of the port specified in ifname.
 *
 * @param ifname pointer to the name of the port for which MAC address is requested
 * @param mac pointer to memory which contains MAC address if function returns success.
 *
 * @return The function will return 0 in case of success. Negative values indicate an error.
 */
int __must_check get_mac_address(char *ifname, uint8_t *mac);

/**
 * @brief Read configuration value from acm configuration file
 *
 * The function looks for the configuration item in the configuration file. If the configuration
 * file contains it, then the function reads the configuration value associated with the
 * configuration item.
 *
 * @param config_item specifies the name of the configuration value to read
 * @param config_value pointer to memory for configuration value, contains value of configuration
 *          item, when function returns success
 * @param value_length size of memory for configuration value
 *
 * @return The function will return 0 in case of success. Negative values indicate an error.
 */
int __must_check sysfs_get_configfile_item(const char *config_item,
        char *config_value,
        int value_length);

/**
 * @brief Return recovery timeout
 *
 * The function checks if there is a configuration value for recovery timeout in the
 * configuration file. If this is the case, the function returns the configuration value.
 * If no configuration value is found, the function returns the default value for recovery timeout.
 *
 * @return The function will return the recovery timeout in milliseconds.
 */
uint32_t __must_check sysfs_get_recovery_timeout(void);

/**
 * @brief Write base recovery timeout to hardware
 *
 * The function first determines the base recovery timeout (function call to
 * sysfs_get_recovery_timeout).
 * Then it iterates through all pairs of redundant streams (TX and RX) of the configuration and
 * writes for each pair the base recovery timeout to the provided file in acm filesystem.
 *
 * @param config address of the configuration for which base recovery values shall be written
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_base_recovery(struct acm_config *config);

/**
 * @brief Write individual recovery timeout to hardware
 *
 * The function iterates through all streams of the module and writes for each stream of type
 * REDUNDANT_STREAM_RX the individual recovery timeout to the provided file in acm filesystem.
 *
 * @param module address of the module which individual recovery values shall be written
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check sysfs_write_individual_recovery(struct acm_module *module);

/**
 * @brief Clear acm hardware / remove configuration from hardware
 *
 * The function writes the "clear all"-pattern to the provided file in acm filesystem.
 * Typically this is done before a new configuration is applied to hardware or if a configuration
 * is removed from hardware.
 *
 * @return The function will return 0 in case of success. Negative values represent an error.
 */
int __must_check write_clear_all_fpga(void);

#endif /* SYSFS_H_ */
