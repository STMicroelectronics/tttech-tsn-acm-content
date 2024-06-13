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

/* Unity Test Framework */
#include "unity.h"

/* directly added modules */
#include "libc_helper.h" // <-- used to link in libc helper functions

/* Module under test */
#include "tracing.h"

/* mock modules needed */
#include "mock_libc.h" // <-- used to generate mocks for libc functions

void setUp(void)
{
    /* register needed libc mocks */
    LIBC_MOCK(vfprintf);
    LIBC_MOCK(fflush);
}

void tearDown(void)
{
    /* unregister libc mocks */
    LIBC_UNMOCK(vfprintf);
    LIBC_UNMOCK(fflush);
}

void test_set_trace(void) {
    set_tracer(TRACER_STDOUT);
    TEST_PASS();
}

void test_set_tracelayer(void) {
    set_tracelayer(TRACELAYER_1);
    TEST_PASS();
}

void test_trace(void) {
    set_tracelayer(TRACELAYER_1);
    set_tracer(TRACER_STDOUT);

    libc_vfprintf_ExpectAndReturn(stdout, "Just testing", 0, 0);
    libc_vfprintf_IgnoreArg_arg();
    libc_fflush_ExpectAndReturn(stdout, 0);

    trace(TRACELAYER_1, "Just testing");
}

void test_trace_tracelayer_too_small(void) {
    set_tracelayer(TRACELAYER_1);
    set_tracer(TRACER_STDOUT);

    libc_vfprintf_ExpectAndReturn(stdout, "Just testing", 0, 0);
    libc_vfprintf_IgnoreArg_arg();
    libc_fflush_ExpectAndReturn(stdout, 0);

    set_tracelayer(-2);
    trace(TRACELAYER_1, "Just testing");
}

void test_trace_tracelayer_too_large(void) {
    set_tracelayer(TRACELAYER_1);
    set_tracer(TRACER_STDOUT);

    libc_vfprintf_ExpectAndReturn(stdout, "Just testing", 0, 0);
    libc_vfprintf_IgnoreArg_arg();
    libc_fflush_ExpectAndReturn(stdout, 0);

    set_tracelayer(5);
    trace(TRACELAYER_1, "Just testing");
}

void test_trace_tracer_out_of_range(void) {
    set_tracelayer(TRACELAYER_1);
    set_tracer(TRACER_STDOUT);

    libc_vfprintf_ExpectAndReturn(stdout, "Just testing", 0, 0);
    libc_vfprintf_IgnoreArg_arg();
    libc_fflush_ExpectAndReturn(stdout, 0);

    set_tracer(47);
    trace(TRACELAYER_1, "Just testing");
}

void test_trace_tracer_trace_suppression(void) {
    set_tracelayer(TRACELAYER_1);
    set_tracer(TRACER_STDOUT);

    trace(TRACELAYER_2, "Just testing");
}
