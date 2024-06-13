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

#include "parse_functions.h"

void parse_mac_addr(sr_val_t node, uint8_t *mac, char *name)
{
    unsigned int tmp[6];

    if(true != sr_xpath_node_name_eq(node.xpath, name))
        return;

    if (ETHER_ADDR_LEN == sscanf(node.data.string_val, "%x-%x-%x-%x-%x-%x%*c",
                                 &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5])) {
        int i;

        for (i = 0; i < ETHER_ADDR_LEN; i++)
            mac[i] = (uint8_t) tmp[i];
    }
}

void parse_stream_vid(sr_val_t node, uint16_t *vid)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_VID_STR))
        return;

    *vid = node.data.uint16_val;
}

void parse_stream_priority(sr_val_t node, uint8_t *prio)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_PRIORITY_STR))
        return;

    *prio = node.data.uint8_val;
}

void parse_stream_const_data_size(sr_val_t node, uint16_t *size)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_ADD_CONST_DATA_SIZE_STR))
        return;

    *size = node.data.uint16_val;
}

void parse_stream_num_of_operations(sr_val_t node, uint16_t *num_stream_ops)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_OPER_LIST_LENGTH_STR))
        return;

    *num_stream_ops = node.data.uint16_val;
}

void parse_hex_string(sr_val_t node, uint8_t *data, char *name, int expected_len)
{
    char *pos;
    int i;
    int len;

    if(true != sr_xpath_node_name_eq(node.xpath, name))
        return;

    if (0 == strcmp(node.data.string_val, "")) {
        SRP_LOG_INF("%s node is empty.", sr_xpath_node_name(node.xpath));
        return;
    }

    pos = node.data.string_val;
    len = strlen(node.data.string_val)/2;
    if (len != expected_len) {
        SRP_LOG_INF("%s node has incorrect size.", sr_xpath_node_name(node.xpath));
        return;
    }
    for (i = 0; i < len; i++) {
        sscanf(pos, "%2hhx", &data[i]);
        pos+=2;
    }
}

void parse_hex_string_add_const_data(sr_val_t node, char *data, char *name, int expected_len)
{
    char *pos;
    int i;
    int len;

    if(true != sr_xpath_node_name_eq(node.xpath, name))
        return;

    if (0 == strcmp(node.data.string_val, "")) {
        SRP_LOG_INF("%s node is empty.", sr_xpath_node_name(node.xpath));
        return;
    }

    pos = node.data.string_val;
    len = strlen(node.data.string_val)/2;
    if (len != expected_len) {
        SRP_LOG_INF("%s node has incorrect size.", sr_xpath_node_name(node.xpath));
        return;
    }
    for (i = 0; i < len; i++) {
        sscanf(pos, "%2hhx", &data[i]);
        pos+=2;
    }
}

void parse_stream_ops_opcode(sr_val_t node, STREAM_OPCODE *opcode, STREAM_OPCODE def_op)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_OPER_NAME_STR))
        return;

    if (0 == strcmp("insert", node.data.enum_val)) {
        *opcode = OPC_INSERT;
        return;
    }

    if (0 == strcmp("forward", node.data.enum_val)) {
        *opcode = OPC_FORWARD;
        return;
    }

    if (0 == strcmp("pad", node.data.enum_val)) {
        *opcode = OPC_PADDING;
        return;
    }

    if (0 == strcmp("insert-constant", node.data.enum_val)) {
        *opcode = OPC_CONST_INSERT;
        return;
    }

    /* set default opcode (???) */
    *opcode = def_op;
}

void parse_stream_ops_offset(sr_val_t node, uint16_t *offset)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_OFFSET_STR))
        return;

    *offset = node.data.uint16_val;
}

void parse_stream_ops_length(sr_val_t node, uint16_t *length)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_LENGTH_STR))
        return;

    *length = node.data.uint16_val;
}

void parse_stream_ops_bufname(sr_val_t node, char *bufname)
{
    if(true != sr_xpath_node_name_eq(node.xpath, ACM_BUFFER_NAME_STR))
        return;

    strncpy(bufname, node.data.string_val, ACM_MAX_NAME_SIZE-1);
}
