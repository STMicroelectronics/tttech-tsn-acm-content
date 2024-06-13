/*
 * TTTech acm-yang-module
 * Copyright(c) 2020 TTTech Industrial Automation AG.
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
 * Contact Information:
 * support@tttech-industrial.com
 * TTTech Industrial Automation AG, Schoenbrunnerstrasse 7, 1040 Vienna, Austria
 */

#ifndef ERROR_CODES_H_
#define ERROR_CODES_H_

#include <libacmconfig/libacmconfig_def.h>
#include <errno.h>

/* These error return codes are complementary to EXIT_SUCCESS (0), EXIT_FAILURE (1) and those
 * defined in libacmconfig_def.h.
 */
#define EACMYMODULECREATE       5001     /* function acm_create_module failed */
#define EACMYSTREAMNUMBER       5002     /* incorrect stream count */
#define EACMYNOSTREAMKEY        5003     /* missing stream key */
#define EACMYSTREAMLISTADD      5004     /* adding to stream list failed */
#define EACMYITSTREAMCREATE     5005     /* function acm_create_ingress_triggered_stream failed */
#define EACMYESTREAMCREATE      5006     /* function acm_create_event_stream failed */
#define EACMYRSTREAMCREATE      5007     /* function acm_create_recovery_stream failed */
#define EACMYTTSTREAMCREATE     5008     /* function acm_create_time_triggered_stream failed */
#define EACMYOPERNUMBER         5009     /* incorrect operation count */
#define EACMYOPERTYPE           5010     /* incorrect operation type */
#define EACMYOPERCONSTLEN       5011     /* length for insert-constant operation is too big */

#endif /* ERROR_CODES_H_ */
