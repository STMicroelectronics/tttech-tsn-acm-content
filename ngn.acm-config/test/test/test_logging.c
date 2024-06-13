#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/syslog.h>

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "logging.h"

/* mock modules needed */
#include "mock_stub_stdio.h"
#include "mock_stub_syslog.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_logging_stderr(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_ERR);
    TEST_ASSERT_EQUAL(0, ret);

    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging test", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);

    logging(LOGLEVEL_ERR, "logging test");
}

void test_logging_stderr_loglevel_too_low(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_ERR);
    TEST_ASSERT_EQUAL(0, ret);

    logging(LOGLEVEL_INFO, "logging test");
}

void test_logging_stderr_high_enough(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_DEBUG);
    TEST_ASSERT_EQUAL(0, ret);

    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging test", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);

    logging(LOGLEVEL_INFO, "logging test");
}

void test_setloglevel(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);

    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging warn", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);
    vfprintf_ExpectAndReturn(stderr, "logging info", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);

    ret = set_loglevel(LOGLEVEL_INFO);
    TEST_ASSERT_EQUAL(0, ret);
    logging(LOGLEVEL_WARN, "logging warn");
    logging(LOGLEVEL_INFO, "logging info");
    logging(LOGLEVEL_DEBUG, "logging debug");
}

void test_setloglevel_too_high_ignored(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);

    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging info1", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);
    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging info2", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);

    ret = set_loglevel(LOGLEVEL_INFO);
    TEST_ASSERT_EQUAL(0, ret);
    logging(LOGLEVEL_INFO, "logging info1");
    ret = set_loglevel(255);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
    logging(LOGLEVEL_INFO, "logging info2");
}

void test_setloglevel_too_low_ignored(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);

    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging info1", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);
    fprintf_IgnoreAndReturn(0);
    vfprintf_ExpectAndReturn(stderr, "logging info2", 0, 0);
    vfprintf_IgnoreArg_arg();
    fflush_ExpectAndReturn(stderr, 0);

    ret = set_loglevel(LOGLEVEL_INFO);
    TEST_ASSERT_EQUAL(0, ret);
    logging(LOGLEVEL_INFO, "logging info1");
    ret = set_loglevel(-1);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
    logging(LOGLEVEL_INFO, "logging info2");
}

void test_setlogger(void) {
    int ret;

    ret = set_logger(LOGGER_STDERR);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_logger(LOGGER_SYSLOG);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_logger(0xFFFF);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/*
 * program mock according to implementation
 */
static void setup_logging_syslog_error(int prio, const char *log) {
    char snprintf_buffer[strlen(log) + 32];

    snprintf_ExpectAndReturn(0, strlen(log) + 32, "%s %s", 0);
    snprintf_IgnoreArg_s();
    snprintf_ReturnMemThruPtr_s(snprintf_buffer, strlen(snprintf_buffer) + 1);
    vsyslog_Expect(prio, snprintf_buffer, 0);
    vsyslog_IgnoreArg_ap();
}

void test_logging_syslog_error(void) {
    int ret;

    ret = set_logger(LOGGER_SYSLOG);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_ERR);
    TEST_ASSERT_EQUAL(0, ret);

    setup_logging_syslog_error(LOG_MAKEPRI(LOG_USER, LOG_ERR), "logging test");
    logging(LOGLEVEL_ERR, "logging test");
}

void test_logging_syslog_warn(void) {
    int ret;

    ret = set_logger(LOGGER_SYSLOG);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_WARN);
    TEST_ASSERT_EQUAL(0, ret);

    setup_logging_syslog_error(LOG_MAKEPRI(LOG_USER, LOG_WARNING), "logging test");
    logging(LOGLEVEL_WARN, "logging test");
}

void test_logging_syslog_info(void) {
    int ret;

    ret = set_logger(LOGGER_SYSLOG);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_INFO);
    TEST_ASSERT_EQUAL(0, ret);

    setup_logging_syslog_error(LOG_MAKEPRI(LOG_USER, LOG_INFO), "logging test");
    logging(LOGLEVEL_INFO, "logging test");
}

void test_logging_syslog_debug(void) {
    int ret;

    ret = set_logger(LOGGER_SYSLOG);
    TEST_ASSERT_EQUAL(0, ret);
    ret = set_loglevel(LOGLEVEL_DEBUG);
    TEST_ASSERT_EQUAL(0, ret);

    setup_logging_syslog_error(LOG_MAKEPRI(LOG_USER, LOG_DEBUG), "logging test");
    logging(LOGLEVEL_DEBUG, "logging test");
}
