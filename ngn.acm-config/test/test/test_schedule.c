#include "unity.h"

#include "schedule.h"
#include "tracing.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "mock_memory.h"
#include "mock_logging.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_schedule_init_list(void) {
    int result;
    struct schedule_list schedlist;
    memset(&schedlist, 0xAA, sizeof (schedlist));

    result = schedule_list_init(&schedlist);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&schedlist));
    TEST_ASSERT(ACMLIST_EMPTY(&schedlist));
    TEST_ASSERT_EQUAL(0, ACMLIST_LOCK(&schedlist));

    ACMLIST_UNLOCK(&schedlist);
}

void test_schedule_init_list_null(void) {
    int result;

    result = schedule_list_init(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_schedule_list_add_schedule(void) {
    int result;
    struct schedule_list schedlist = SCHEDULE_LIST_INITIALIZER(schedlist);
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;

    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&schedlist));
    result = schedule_list_add_schedule(&schedlist, &schedule);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&schedlist));
    TEST_ASSERT_EQUAL(&schedule, ACMLIST_FIRST(&schedlist));
    TEST_ASSERT_EQUAL(&schedule, ACMLIST_LAST(&schedlist,schedule_list));
}

void test_schedule_list_add_schedule_list_null(void) {
    int result;
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;

    logging_Expect(LOGLEVEL_ERR, "%s: parameters must be non-null");
    result = schedule_list_add_schedule(NULL, &schedule);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_schedule_list_add_schedule_schedule_null(void) {
    int result;
    struct schedule_list schedlist = SCHEDULE_LIST_INITIALIZER(schedlist);

    logging_Expect(LOGLEVEL_ERR, "%s: parameters must be non-null");
    result = schedule_list_add_schedule(&schedlist, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_schedule_list_add_schedule_all_null(void) {
    int result;

    logging_Expect(LOGLEVEL_ERR, "%s: parameters must be non-null");
    result = schedule_list_add_schedule(NULL, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_schedule_create(void) {
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct schedule_entry *result;

    acm_zalloc_ExpectAndReturn(sizeof(schedule), &schedule);
    result = schedule_create(10, 30, 20, 300);
    TEST_ASSERT_EQUAL_PTR(&schedule, result);
    TEST_ASSERT_EQUAL(10, result->time_start_ns);
    TEST_ASSERT_EQUAL(30, result->time_end_ns);
    TEST_ASSERT_EQUAL(20, result->send_time_ns);
    TEST_ASSERT_EQUAL(300, result->period_ns);
}

void test_schedule_create_period_fail(void) {
    struct schedule_entry *result;

    logging_Expect(LOGLEVEL_ERR, "Schedulewindow: Nonzero period needed");

    result = schedule_create(10, 30, 20, 0);
    TEST_ASSERT_NULL(result);
}

void test_schedule_create_neg_mem_alloc(void) {
    struct schedule_entry *result;

    acm_zalloc_ExpectAndReturn(sizeof(struct schedule_entry), NULL);
    logging_Expect(LOGLEVEL_ERR, "%s: Out of memory");

    result = schedule_create(10, 30, 20, 500);
    TEST_ASSERT_NULL(result);
}

void test_schedule_destroy(void) {
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;

    acm_free_Expect(&schedule);
    schedule_destroy(&schedule);
}

void test_schedule_list_flush(void) {
    int i;
    struct schedule_entry schedule[10] = {
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER
    };
    struct schedule_list list = SCHEDULE_LIST_INITIALIZER(list);

    for (i = 0; i < 10; ++i) {
        _ACMLIST_INSERT_TAIL(&list, &schedule[i], entry);
    }
    TEST_ASSERT_EQUAL(10, ACMLIST_COUNT(&list));

    for (i = 0; i < 10; ++i)
        acm_free_Expect(&schedule[i]);

    schedule_list_flush(&list);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&list));
}

void test_schedule_empty_list_null(void) {
    schedule_list_flush(NULL);
}

void test_schedule_list_remove_schedule(void) {
    int i;
    struct schedule_entry schedule[10] = {
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER,
        SCHEDULE_ENTRY_INITIALIZER
    };
    /* prepare test case*/
    struct schedule_list list = SCHEDULE_LIST_INITIALIZER(list);

    for (i = 0; i < 10; ++i) {
        _ACMLIST_INSERT_TAIL(&list, &schedule[i], entry);
    }
    TEST_ASSERT_EQUAL(10, ACMLIST_COUNT(&list));
    TEST_ASSERT_EQUAL_PTR(&schedule[4], schedule[3].entry.tqe.tqe_next);
    TEST_ASSERT_EQUAL_PTR(&schedule[9], schedule[8].entry.tqe.tqe_next);

    /* execute test case*/
    acm_free_Expect(&schedule[4]);
    schedule_list_remove_schedule(&list, &schedule[4]);
    TEST_ASSERT_EQUAL(9, ACMLIST_COUNT(&list));
    TEST_ASSERT_EQUAL_PTR(&schedule[5], schedule[3].entry.tqe.tqe_next);

    acm_free_Expect(&schedule[9]);
    schedule_list_remove_schedule(&list, &schedule[9]);
    TEST_ASSERT_EQUAL(8, ACMLIST_COUNT(&list));
    TEST_ASSERT_EQUAL_PTR(NULL, schedule[8].entry.tqe.tqe_next);
}
