/*
 * TTTech acm-yang
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
 * Contact: https://tttech.com
 * support@tttech.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */

#ifndef PARSE_FUNCTIONS_H_
#define PARSE_FUNCTIONS_H_

#include <stdlib.h>
#include <string.h>
/* sysrepo includes */
#include <sysrepo.h>
#include <sysrepo/values.h>
#include <sysrepo/xpath.h>
/* libacm includes */
#include <libacmconfig/libacmconfig.h>
/* module specific */
#include "acm_defines.h"

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a MAC address.
 *
 * The function parses through sr_val_t structure in search of a MAC address object of a given name
 * (this is typically a source or a destination MAC). If the object is found, the function retrieves
 * the MAC address value.
 *
 * @param[in]   node         current sr_val_t node under which to search
 * @param[out]  mac          retrieved MAC address
 * @param[in]   name         name of the object
 */
void parse_mac_addr(sr_val_t node, uint8_t *mac, char *name);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a VLAN ID.
 *
 * The function parses through sr_val_t structure in search of a VLAN ID.
 *
 * @param[in]   node         current sr_val_t node under which to search
 * @param[out]  vid          VLAN ID
 */
void parse_stream_vid(sr_val_t node, uint16_t *vid);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a VLAN priority.
 *
 * The function parses through sr_val_t structure in search of a VLAN priority.
 *
 * @param[in]   node         current sr_val_t node under which to search
 * @param[out]  prio         VLAN priority
 */
void parse_stream_priority(sr_val_t node, uint8_t *prio);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a VLAN priority.
 *
 * The function parses through sr_val_t structure in search of a VLAN priority.
 *
 * @param[in]   node         current sr_val_t node under which to search
 * @param[out]  prio         VLAN priority
 */
void parse_stream_const_data_size(sr_val_t node, uint16_t *size);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a number of operations.
 *
 * The function parses through sr_val_t structure in search of a number of stream operations.
 *
 * @param[in]   node           current sr_val_t node under which to search
 * @param[out]  num_stream_ops number of stream operations
 */
void parse_stream_num_of_operations(sr_val_t node, uint16_t *num_stream_ops);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a hex-string object.
 *
 * The function parses through sr_val_t structure in search of a hex-string object of a given name. If
 * the object is found, the function retrieves the data value. The data value is stored in sr_val_t as
 * a string representing a hexadecimal value, which is here converted into a numeric value.
 *
 * @param[in]   node         Current node under which to search.
 * @param[out]  data         Object's data value.
 * @param[in]   name         Name of the object.
 * @param[in]   expected_len Expected number of characters of the object value.
 */
void parse_hex_string(sr_val_t node, uint8_t *data, char *name, int expected_len);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a hex-string object.
 *
 * The function parses through sr_val_t structure in search of a hex-string object of a given name. If
 * the object is found, the function retrieves the data value. The data value is stored in sr_val_t as
 * a string representing a hexadecimal value, which is here converted into a numeric value.
 *
 * @param[in]   node         Current node under which to search.
 * @param[out]  data         Object's data value.
 * @param[in]   name         Name of the object.
 * @param[in]   expected_len Expected number of characters of the object value.
 */
void parse_hex_string_add_const_data(sr_val_t node, char *data, char *name, int expected_len);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve a number of operations.
 *
 * The function parses through sr_val_t structure in search of a number of stream operations.
 *
 * @param[in]   node           current sr_val_t node under which to search
 * @param[out]  num_stream_ops number of stream operations
 */
void parse_stream_ops_opcode(sr_val_t node, STREAM_OPCODE *opcode, STREAM_OPCODE def_op);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve offset.
 *
 * The function parses through sr_val_t structure in search of offset.
 *
 * @param[in]   node           current sr_val_t node under which to search
 * @param[out]  num_stream_ops number of stream operations
 */
void parse_stream_ops_offset(sr_val_t node, uint16_t *offset);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve length.
 *
 * The function parses through sr_val_t structure in search of length.
 *
 * @param[in]   node           current sr_val_t node under which to search
 * @param[out]  num_stream_ops number of stream operations
 */
void parse_stream_ops_length(sr_val_t node, uint16_t *length);

/**
 * @ingroup acminternal
 * @brief Parse sr_val_t structure to retrieve buffer-name.
 *
 * The function parses through sr_val_t structure in search of buffer-name.
 *
 * @param[in]   node           current sr_val_t node under which to search
 * @param[out]  num_stream_ops number of stream operations
 */
void parse_stream_ops_bufname(sr_val_t node, char *bufname);

#endif
