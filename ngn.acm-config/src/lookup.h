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
#ifndef LOOKUP_H_
#define LOOKUP_H_

#include "libacmconfig_def.h"

/**
 * @brief structure to hold lookup data within acm library
 */
struct lookup {
    uint8_t header[ACM_MAX_LOOKUP_SIZE];    /**< header-pattern of the ingress frame (16 bytes) */
    uint8_t header_mask[ACM_MAX_LOOKUP_SIZE]; /**< header-mask to define which parts shall be filtered from the ingress frame */
    uint8_t filter_mask[ACM_MAX_FILTER_SIZE]; /**< mask to define which parts of the filter will be used */
    uint8_t filter_pattern[ACM_MAX_FILTER_SIZE]; /**< pattern of the additional filter (up to 112 bytes) */
    size_t filter_size; /**< relevant length of the additional filter */
};

/**
 * @ingroup acmstream
 * @brief create a lookup item
 *
 * The function allocates a new lookup item and initializes all the properties
 * of it.
 * In case of an error NULL is returned instead of a handle.
 * Errors may be: parameter limitations are not met or input parameter values
 * are inconsistent.
 *
 * @param header header-pattern of the ingress frame (16 bytes)
 * @param header_mask header-mask to define which parts shall be filtered from
 * 			the ingress frame
 * @param filter_pattern pattern of the additional filter (up to 112 bytes)
 * @param filter_mask mask to define which parts of the filter will be used
 * @param filter_size size of the additional filter
 *
 * @return handle to the lookup item
 */
struct lookup __must_check* lookup_create(uint8_t header[ACM_MAX_LOOKUP_SIZE],
        uint8_t header_mask[ACM_MAX_LOOKUP_SIZE],
        const uint8_t *filter_pattern,
        const uint8_t *filter_mask,
        size_t filter_size);

/**
 * @ingroup acmstream
 * @brief delete a lookup item
 *
 * The function destroys the lookup item
 *
 * @param lookup handle to the lookup item
 */
void lookup_destroy(struct lookup *lookup);

#endif /* LOOKUP_H_ */
