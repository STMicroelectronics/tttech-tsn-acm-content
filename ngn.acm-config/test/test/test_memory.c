#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "memory.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_acm_zalloc(void) {
    void *buffer;
    unsigned char expected[128] = { 0 };

    buffer = acm_zalloc(sizeof(expected));
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQUAL_MEMORY(expected, buffer, sizeof(expected));

    free(buffer);
    TEST_PASS();
}

void test_acm_free(void) {
    void *buffer = malloc(128);

    acm_free(buffer);
    TEST_PASS();
}

void test_acm_strdup(void) {
    char *expected = "strdup-test";
    char *result;

    result = acm_strdup(expected);
    TEST_ASSERT_NOT_EQUAL(expected, result);
    TEST_ASSERT_EQUAL_MEMORY(expected, result, strlen(expected) + 1);

    free(result);
    TEST_PASS();
}
