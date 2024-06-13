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

#include "lookup.h"
#include "logging.h"
#include "tracing.h"
#include "memory.h"

struct lookup __must_check* lookup_create(uint8_t header[ACM_MAX_LOOKUP_SIZE],
        uint8_t header_mask[ACM_MAX_LOOKUP_SIZE],
        const uint8_t *filter_pattern,
        const uint8_t *filter_mask,
        size_t filter_size) {
    struct lookup *lookup;

    TRACE2_ENTER();
    if (filter_size > ACM_MAX_FILTER_SIZE) {
        LOGERR("Lookup: Filter size too big: %u > %u", filter_size, ACM_MAX_FILTER_SIZE);
        TRACE2_MSG("Fail");
        return NULL;
    }

    if ( (filter_size > 0) && (!filter_mask || !filter_pattern)) {
        LOGERR("Lookup: Filter input error");
        TRACE2_MSG("Fail");
        return NULL;
    }

    lookup = acm_zalloc(sizeof (*lookup));

    if (!lookup) {
        LOGERR("Lookup: Out of memory");
        TRACE2_MSG("Fail");
        return NULL;
    }

    memcpy(lookup->header, header, ACM_MAX_LOOKUP_SIZE);
    memcpy(lookup->header_mask, header_mask, ACM_MAX_LOOKUP_SIZE);

    memcpy(lookup->filter_mask, filter_mask, filter_size);
    memcpy(lookup->filter_pattern, filter_pattern, filter_size);
    lookup->filter_size = filter_size;

    TRACE2_EXIT();
    return lookup;
}

void lookup_destroy(struct lookup *lookup) {
    TRACE2_MSG("Executed");
    if (!lookup) {
        return;
    }
    acm_free(lookup);
}

