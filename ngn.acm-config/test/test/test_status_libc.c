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
#include <fcntl.h>
#include <unistd.h>

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "status.h"

/* directly added modules */
#include "libc_helper.h" // <-- used to link in libc helper functions
#include "tracing.h"

/* mock modules needed */
#include "mock_libc.h" // <-- used to generate mocks for libc functions
#include "mock_logging.h"
#include "mock_memory.h"
#include "mock_sysfs.h"

void setUp(void)
{
    /* register needed libc mocks */
    LIBC_MOCK(__errno_location);
    LIBC_MOCK(open);
    LIBC_MOCK(pread);
    LIBC_MOCK(pwrite);
    LIBC_MOCK(close);
    LIBC_MOCK(socket);
    LIBC_MOCK(ioctl);
    LIBC_MOCK(read);
    LIBC_MOCK(snprintf);
    LIBC_MOCK(asprintf);
}

void tearDown(void)
{
    /* unregister libc mocks */
    LIBC_UNMOCK(__errno_location);
    LIBC_UNMOCK(open);
    LIBC_UNMOCK(pread);
    LIBC_UNMOCK(pwrite);
    LIBC_UNMOCK(close);
    LIBC_UNMOCK(socket);
    LIBC_UNMOCK(ioctl);
    LIBC_UNMOCK(read);
    LIBC_UNMOCK(snprintf);
    LIBC_UNMOCK(asprintf);
}

/*
 * helpers to program mock
 */

// TODO: delete following function?
/* program mock for an error log call */
static void logging_Expect_loglevel_err(void) {
    logging_Expect(LOGLEVEL_ERR, NULL);
    logging_IgnoreArg_format();
}

// TODO: delete following function?
/* program mock for snprintf */
static void libc_snprintf_Expect_result_retval(char *result, int retval) {
    if (result)
        libc_snprintf_ExpectAndReturn(NULL, 0, NULL, strlen(result));
    else
        libc_snprintf_ExpectAndReturn(NULL, 0, NULL, retval);
    libc_snprintf_IgnoreArg_s();
    libc_snprintf_IgnoreArg_maxlen();
    libc_snprintf_IgnoreArg_format();
    if (result)
        libc_snprintf_ReturnMemThruPtr_s(result, strlen(result) + 1);
}

// TODO: delete following function?
/* program mock for a successful call to sysfs_construct_path_name */
static void sysfs_construct_path_name_Expect(char *filename) {
    libc_snprintf_Expect_result_retval(filename, 0);
}

// TODO: delete following function?
/* program mock for an unsuccessful call to sysfs_construct_path_name */
static void sysfs_construct_path_name_Expect_failure(int err) {
    libc_snprintf_Expect_result_retval(NULL, -err);
    logging_Expect_loglevel_err();
}

void test_status_read_item_neg_asprintf(void) {
    int result;
    enum acm_module_id module_id = 1;
    enum acm_status_item id = STATUS_DISABLE_OVERRUN_PREV_CYCLE;

    libc_asprintf_ExpectAndReturn(NULL, ACMDEV_BASE "%s/%s_M%d", -1);
    libc_asprintf_IgnoreArg_ptr();
    logging_Expect(LOGLEVEL_ERR, "Status: out of memory");
    result = status_read_item(module_id, id);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_read_diagnostics_neg_asprintf(void) {
    struct acm_diagnostic *result;
    enum acm_module_id module_id = 1;

    libc_asprintf_ExpectAndReturn(NULL, ACMDEV_BASE "%s/%s_M%d", -1);
    libc_asprintf_IgnoreArg_ptr();
    logging_Expect(LOGLEVEL_ERR, "Status: out of memory");
    result = status_read_diagnostics(module_id);
    TEST_ASSERT_NULL(result);
}

void test_status_set_diagnostics_poll_time_neg_asprintf1(void) {
    int result;
    enum acm_module_id module_id = 1;
    uint16_t interval_ms = 100;

    libc_asprintf_ExpectAndReturn(NULL, ACMDEV_BASE "%s/%s_M%d", -1);
    libc_asprintf_IgnoreArg_ptr();
    logging_Expect(LOGLEVEL_ERR, "Status: out of memory");
    result = status_set_diagnostics_poll_time(module_id, interval_ms);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_set_diagnostics_poll_time_neg_asprintf2(void) {
    int result;
    enum acm_module_id module_id = 1;
    uint16_t interval_ms = 100;
    char *filename;
    int filenamelength = strlen(ACMDEV_BASE) + strlen(__stringify(ACMDRV_SYSFS_DIAG_GROUP)) +
            strlen(__stringify(ACM_SYSFS_DIAG_POLL_TIME)) + 2;

    filename = malloc(filenamelength);

    libc_asprintf_ExpectAndReturn(NULL, ACMDEV_BASE "%s/%s_M%d", 0);
    libc_asprintf_IgnoreArg_ptr();
    libc_asprintf_ReturnMemThruPtr_ptr(&filename, filenamelength +1);
    libc_asprintf_ExpectAndReturn(NULL, "%d", -1);
    libc_asprintf_IgnoreArg_ptr();
    logging_Expect(LOGLEVEL_ERR, "Status: out of memory");
    result = status_set_diagnostics_poll_time(module_id, interval_ms);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_get_ip_version(void) {
    char *result;

    sysfs_construct_path_name_ExpectAndReturn(NULL, SYSFS_PATH_LENGTH,
                "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_buffer_sysfs_item_ExpectAndReturn(NULL, NULL, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_path_name();
    read_buffer_sysfs_item_IgnoreArg_buffer();
    sysfs_construct_path_name_ExpectAndReturn(NULL, SYSFS_PATH_LENGTH,
                "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_buffer_sysfs_item_ExpectAndReturn(NULL, NULL, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_path_name();
    read_buffer_sysfs_item_IgnoreArg_buffer();
    sysfs_construct_path_name_ExpectAndReturn(NULL, SYSFS_PATH_LENGTH,
                "status", "revision_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_buffer_sysfs_item_ExpectAndReturn(NULL, NULL, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_path_name();
    read_buffer_sysfs_item_IgnoreArg_buffer();

    libc_asprintf_ExpectAndReturn(NULL, "%s-%s-%s", -1);
    libc_asprintf_IgnoreArg_ptr();
    logging_Expect(LOGLEVEL_ERR, "Status: out of memory");
    result = status_get_ip_version();
    TEST_ASSERT_NULL(result);
}
