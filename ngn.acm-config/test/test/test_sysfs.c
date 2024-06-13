#define _GNU_SOURCE /* for mkostemp */
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "sysfs.h"
#include "list.h"

/* directly added modules */
#include "tracing.h"
#include "setup_helper.h"

/* mock modules needed */
#include "mock_memory.h"
#include "mock_logging.h"
#include "mock_stream.h"
#include "mock_status.h"
#include "mock_stub_pthread.h"

/* helper macros */
#define STREAM_INITIALIZER_SCATTER_DMA_IDX(_stream, _type, _scatter_dma_index) \
    STREAM_INITIALIZER_ALL(_stream, _type, NULL, NULL, NULL, NULL, 0, 0, _scatter_dma_index, 0, 0)

#define STREAM_INITIALIZER_GATHER_DMA_IDX(_stream, _type, _gather_dma_index) \
    STREAM_INITIALIZER_ALL(_stream, _type, NULL, NULL, NULL, NULL, 0, _gather_dma_index, 0, 0, 0)

#define STREAM_INITIALIZER_REFERENCE(_stream, _type, _reference) \
    STREAM_INITIALIZER_ALL(_stream, _type, NULL, _reference, NULL, NULL, 0, 0, 0, 0, 0)

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
    setup_sysfs();
}

void tearDown(void)
{
    teardown_sysfs();
}

void test_write_file_sysfs(void) {

    int ret;
    char tmpfile[] = "/tmp/test_write_file_sysfs_XXXXXX";
    int tmpfd;
    char tmpbuffer[128] = { 0 };
    char testbuffer[] = { 0xAF, 0xFE, 0xDE, 0xAD };
    char readbuffer[sizeof(testbuffer)];

    /* create 128 bytes temp file for test input */
    tmpfd = mkostemp(tmpfile, O_WRONLY);
    ret = write(tmpfd, tmpbuffer, sizeof(tmpbuffer));
    close(tmpfd);

    if (ret != sizeof(tmpbuffer))
        TEST_FAIL_MESSAGE("Cannot setup temp file");

    /* test write_file_sysfs */
    ret = write_file_sysfs(tmpfile, testbuffer, sizeof(testbuffer), 42);
    TEST_ASSERT_EQUAL(0, ret); /* must succeed */

    /* read back file */
    tmpfd = open(tmpfile, O_RDONLY);
    if (tmpfd == -1)
        TEST_FAIL_MESSAGE("Cannot open temp file for reading");
    ret = pread(tmpfd, readbuffer, sizeof(readbuffer), 42);
    close(tmpfd);
    if (sizeof(readbuffer) != ret)
        TEST_FAIL_MESSAGE("Cannot read back temp file");

    /* compare content */
    TEST_ASSERT_EQUAL_MEMORY(testbuffer, readbuffer,sizeof(testbuffer));

    unlink(tmpfile);
}

void test_write_file_sysfs_neg_open_file(void) {
    int result;
    char pathname[] = "config_bin/individual_recovery";
    uint32_t write_value = 555;

    logging_Expect(0, "Sysfs: open file %s failed");
    result = write_file_sysfs(pathname, (void*) &write_value, sizeof (write_value), 0);
    TEST_ASSERT_EQUAL(-ENOENT, result);
}

void test_write_file_sysfs_neg_write_file(void) {
    int result;
    char pathname[] = ACMDEV_BASE "config_bin/clear_all_fpga";
    uint32_t write_value = 555;

    logging_Expect(0, "Sysfs: problem writing data %s");
    result = write_file_sysfs(pathname, (void*) &write_value, sizeof (write_value), -10);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_write_buffer_sysfs_item(void) {
    char pathname[] = __stringify(ACM_SYSFS_CLEAR_ALL_FPGA);
    int32_t buffer = ACMDRV_CLEAR_ALL_PATTERN, read_value;
    int result, fd;

    TEST_ASSERT_EQUAL_STRING("clear_all_fpga", __stringify(ACM_SYSFS_CLEAR_ALL_FPGA));
    result = write_buffer_config_sysfs_item(pathname, (char*) &buffer, sizeof (buffer), 0);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/clear_all_fpga", O_RDONLY);
    result = read(fd, &read_value, sizeof(uint32_t));
    TEST_ASSERT_EQUAL(sizeof(uint32_t), result);
    TEST_ASSERT_EQUAL(ACMDRV_CLEAR_ALL_PATTERN, read_value);
    close(fd);
}

void test_write_buffer_sysfs_item_neg(void) {
    char pathname[] = "there_are_too_many_characters_for_the_successful_test";
    int32_t buffer = ACMDRV_CLEAR_ALL_PATTERN;
    int result;

    logging_Expect(0, "Sysfs: pathname of sysfs device too long");
    result = write_buffer_config_sysfs_item(pathname, (char*) &buffer, sizeof (buffer), 0);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_read_uint64_sysfs_item(void) {
    int value_read;
    char file[SYSFS_PATH_LENGTH] = ACMDEV_BASE;

    strcat(file, "status/msgbuf_count");
    value_read = read_uint64_sysfs_item(file);
    TEST_ASSERT_EQUAL_INT64(32, value_read);
}

void test_read_uint64_sysfs_item_uint64(void) {
    uint64_t value_read;
    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    uint64_t test_value = 0xAFFEDEADAFFEDEADULL;

    testf = fopen(testfile, "w");
    fprintf(testf, "%" PRIu64, test_value);
    fclose(testf);

    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_UINT64(test_value, value_read);
}

void test_read_uint64_sysfs_item_int64(void) {
    int64_t value_read;
    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    int64_t test_value = -0x7FFEDEADAFFEDEADLL;

    testf = fopen(testfile, "w");
    fprintf(testf, "%" PRId64, test_value);
    fclose(testf);

    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_INT64(test_value, value_read);
}

void test_read_uint64_sysfs_item_uint32(void) {
    uint32_t value_read;
    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    uint64_t test_value = 0xAFFEDEADULL;

    testf = fopen(testfile, "w");
    fprintf(testf, "%" PRIu32, test_value);
    fclose(testf);

    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_UINT32(test_value, value_read);
}

void test_read_uint64_sysfs_item_int32(void) {
    int32_t value_read;
    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    int64_t test_value = -0x7FFEDEADLL;

    testf = fopen(testfile, "w");
    fprintf(testf, "%" PRId32, test_value);
    fclose(testf);

    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_INT32(test_value, value_read);
}

void test_read_uint64_sysfs_item_uint16(void) {
    uint16_t value_read;
    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    uint64_t test_value = 0xAFFEULL;

    testf = fopen(testfile, "w");
    fprintf(testf, "%" PRIu16, test_value);
    fclose(testf);

    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_UINT16(test_value, value_read);
}

void test_read_uint64_sysfs_item_int16(void) {
    int16_t value_read;
    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    int64_t test_value = -0x7FFELL;

    testf = fopen(testfile, "w");
    fprintf(testf, "%" PRId16, test_value);
    fclose(testf);

    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_INT16(test_value, value_read);
}

void test_read_uint64_sysfs_item_open_fail(void) {
    int value_read;
    char file[SYSFS_PATH_LENGTH] = ACMDEV_BASE;

    strcat(file, "status/not_existant");
    logging_Expect(0, "Sysfs: open file %s failed");
    value_read = read_uint64_sysfs_item(file);
    TEST_ASSERT_EQUAL_INT64(-ENODEV, value_read);
}

void test_read_uint64_sysfs_item_no_data(void) {
    int value_read;
    char file[SYSFS_PATH_LENGTH] = ACMDEV_BASE;

    strcat(file, "empty_file");
    logging_Expect(0, "Sysfs: read error or no data available at file %s");
    value_read = read_uint64_sysfs_item(file);
    TEST_ASSERT_EQUAL_INT64(-EACMSYSFSNODATA, value_read);
}

void test_read_uint64_sysfs_item_data_not_integer(void) {
    int value_read;

    const char *testfile = ACMDEV_BASE "read_uint64_sysfs_item_test";
    FILE *testf;
    char test_value[] = "text_file";

    testf = fopen(testfile, "w");
    fprintf(testf, "%s", test_value);
    fclose(testf);

    logging_Expect(0, "Sysfs: problem converting %s to integer");
    value_read = read_uint64_sysfs_item(testfile);
    TEST_ASSERT_EQUAL_INT64(-EINVAL, value_read);
}

void test_add_fsc_to_module_sorted_insert_first(void) {
    struct fsc_command_list fsc_list = COMMANDLIST_INITIALIZER(fsc_list);
    struct fsc_command fsc_schedule = { { 0, 40 }, NULL, NULL, NULL };

    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule);
    TEST_ASSERT_EQUAL_PTR(ACMLIST_FIRST(&fsc_list), &fsc_schedule);
}

void test_add_fsc_to_module_sorted_insert_last(void) {
    struct fsc_command_list fsc_list = COMMANDLIST_INITIALIZER(fsc_list);
    struct fsc_command fsc_schedule1 = { { 0, 40 }, NULL, NULL, NULL };
    struct fsc_command fsc_schedule2 = { { 0, 80 }, NULL, NULL, NULL };

    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule1);
    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule2);
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_FIRST(&fsc_list));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule2, ACMLIST_NEXT(&fsc_schedule1, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule2, ACMLIST_LAST(&fsc_list, fsc_command_list));
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&fsc_list));
}

void test_add_fsc_to_module_sorted_insert_before(void) {
    struct fsc_command_list fsc_list = COMMANDLIST_INITIALIZER(fsc_list);
    struct fsc_command fsc_schedule1 = { { 0, 70 }, NULL, NULL, NULL };
    struct fsc_command fsc_schedule2 = { { 0, 40 }, NULL, NULL, NULL };
    struct fsc_command fsc_schedule3 = { { 0, 50 }, NULL, NULL, NULL };
    struct fsc_command fsc_schedule4 = { { 0, 30 }, NULL, NULL, NULL };
    struct fsc_command fsc_schedule5 = { { 0, 130 }, NULL, NULL, NULL };

    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule1);
    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule2);
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule2, ACMLIST_FIRST(&fsc_list));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_NEXT(&fsc_schedule2, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_LAST(&fsc_list, fsc_command_list));
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&fsc_list));

    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule3);
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule2, ACMLIST_FIRST(&fsc_list));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule3, ACMLIST_NEXT(&fsc_schedule2, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_NEXT(&fsc_schedule3, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_LAST(&fsc_list, fsc_command_list));
    TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&fsc_list));

    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule4);
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule4, ACMLIST_FIRST(&fsc_list));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule2, ACMLIST_NEXT(&fsc_schedule4, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule3, ACMLIST_NEXT(&fsc_schedule2, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_NEXT(&fsc_schedule3, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_LAST(&fsc_list, fsc_command_list));
    TEST_ASSERT_EQUAL(4, ACMLIST_COUNT(&fsc_list));

    pthread_mutex_lock_ExpectAndReturn(&fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&fsc_list.lock, 0);
    add_fsc_to_module_sorted(&fsc_list, &fsc_schedule5);
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule4, ACMLIST_FIRST(&fsc_list));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule2, ACMLIST_NEXT(&fsc_schedule4, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule3, ACMLIST_NEXT(&fsc_schedule2, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule1, ACMLIST_NEXT(&fsc_schedule3, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule5, ACMLIST_NEXT(&fsc_schedule1, entry));
    TEST_ASSERT_EQUAL_PTR(&fsc_schedule5, ACMLIST_LAST(&fsc_list, fsc_command_list));
    TEST_ASSERT_EQUAL(5, ACMLIST_COUNT(&fsc_list));
}

void test_fsc_init_list(void) {
    struct fsc_command_list fsc_list = COMMANDLIST_INITIALIZER(fsc_list);

    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&fsc_list));
    TEST_ASSERT_NULL(ACMLIST_FIRST(&fsc_list));
    TEST_ASSERT_EQUAL_PTR(ACMLIST_LAST(&fsc_list, fsc_command_list), ACMLIST_FIRST(&fsc_list));
}

void test_create_event_sysfs_items(void) {
    int result;
    struct fsc_command fsc_schedule_mem1, fsc_schedule_mem2, fsc_schedule_mem3, fsc_schedule_mem4,
            fsc_schedule_mem5, fsc_schedule_mem6, fsc_schedule_mem7, fsc_schedule_mem8,
            fsc_schedule_mem9, fsc_schedule_mem10;
    memset(&fsc_schedule_mem1, 0, sizeof (fsc_schedule_mem1));
    memset(&fsc_schedule_mem2, 0, sizeof (fsc_schedule_mem2));
    memset(&fsc_schedule_mem3, 0, sizeof (fsc_schedule_mem3));
    memset(&fsc_schedule_mem4, 0, sizeof (fsc_schedule_mem4));
    memset(&fsc_schedule_mem5, 0, sizeof (fsc_schedule_mem5));
    memset(&fsc_schedule_mem6, 0, sizeof (fsc_schedule_mem6));
    memset(&fsc_schedule_mem7, 0, sizeof (fsc_schedule_mem7));
    memset(&fsc_schedule_mem8, 0, sizeof (fsc_schedule_mem8));
    memset(&fsc_schedule_mem9, 0, sizeof (fsc_schedule_mem9));
    memset(&fsc_schedule_mem10, 0, sizeof (fsc_schedule_mem10));
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 5000;
    module.module_delays[SPEED_100MBps].chip_in = 50;
    module.module_delays[SPEED_100MBps].chip_eg = 120;
    module.module_delays[SPEED_100MBps].phy_in = 404;
    module.module_delays[SPEED_100MBps].phy_eg = 444;
    module.module_delays[SPEED_100MBps].ser_bypass = 2844;
    module.module_delays[SPEED_100MBps].ser_switch = 3900;
    // prepare log_schedule
    log_schedule.period_ns = 500;
    log_schedule.send_time_ns = 70;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem2), &fsc_schedule_mem2);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem3), &fsc_schedule_mem3);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem4), &fsc_schedule_mem4);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem5), &fsc_schedule_mem5);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem6), &fsc_schedule_mem6);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem7), &fsc_schedule_mem7);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem8), &fsc_schedule_mem8);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem9), &fsc_schedule_mem9);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem10), &fsc_schedule_mem10);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    result = create_event_sysfs_items(&log_schedule, &module, 80, 20, 5);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(10, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(0, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(6, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(13, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(19, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(25, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(31, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(38, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(44, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(50, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(56, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
}

void test_create_event_sysfs_items_small_gap(void) {
    int result;
    struct fsc_command fsc_schedule_mem1, fsc_schedule_mem2, fsc_schedule_mem3, fsc_schedule_mem4,
            fsc_schedule_mem5, fsc_schedule_mem6, fsc_schedule_mem7, fsc_schedule_mem8,
            fsc_schedule_mem9, fsc_schedule_mem10;
    memset(&fsc_schedule_mem1, 0, sizeof (fsc_schedule_mem1));
    memset(&fsc_schedule_mem2, 0, sizeof (fsc_schedule_mem2));
    memset(&fsc_schedule_mem3, 0, sizeof (fsc_schedule_mem3));
    memset(&fsc_schedule_mem4, 0, sizeof (fsc_schedule_mem4));
    memset(&fsc_schedule_mem5, 0, sizeof (fsc_schedule_mem5));
    memset(&fsc_schedule_mem6, 0, sizeof (fsc_schedule_mem6));
    memset(&fsc_schedule_mem7, 0, sizeof (fsc_schedule_mem7));
    memset(&fsc_schedule_mem8, 0, sizeof (fsc_schedule_mem8));
    memset(&fsc_schedule_mem9, 0, sizeof (fsc_schedule_mem9));
    memset(&fsc_schedule_mem10, 0, sizeof (fsc_schedule_mem10));
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule1 = SCHEDULE_ENTRY_INITIALIZER;
    struct schedule_entry log_schedule2 = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 1000000;
    module.module_delays[SPEED_100MBps].chip_in = 50;
    module.module_delays[SPEED_100MBps].chip_eg = 120;
    module.module_delays[SPEED_100MBps].phy_in = 404;
    module.module_delays[SPEED_100MBps].phy_eg = 444;
    module.module_delays[SPEED_100MBps].ser_bypass = 2844;
    module.module_delays[SPEED_100MBps].ser_switch = 3900;
    // prepare log_schedule
    log_schedule1.period_ns = 1000000;
    log_schedule1.send_time_ns = 100000;
    log_schedule2.period_ns = 1000000;
    log_schedule2.send_time_ns = 100004;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    result = create_event_sysfs_items(&log_schedule1, &module, 80, 20, 5);
    TEST_ASSERT_EQUAL(0, result);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem2), &fsc_schedule_mem2);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    result = create_event_sysfs_items(&log_schedule2, &module, 80, 20, 5);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(1243, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(1243, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(36175892, next_item->hw_schedule_item.cmd);
}

void test_create_event_sysfs_items_null(void) {
    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;

    //init module
    module.cycle_ns = 5000;
    // prepare log_schedule
    log_schedule.period_ns = 500;
    log_schedule.send_time_ns = 70;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), NULL);
    logging_Expect(0, "Sysfs: Out of memory");

    result = create_event_sysfs_items(&log_schedule, &module, 80, 20, 5);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_create_window_sysfs_items(void) {
    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem3 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem4 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem5 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem6 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem7 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem8 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem9 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem10 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_PARALLEL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 10000;
    module.module_delays[SPEED_1GBps].chip_in = 50;
    module.module_delays[SPEED_1GBps].chip_eg = 120;
    module.module_delays[SPEED_1GBps].phy_in = 298;
    module.module_delays[SPEED_1GBps].phy_eg = 199;
    module.module_delays[SPEED_1GBps].ser_bypass = 439;
    module.module_delays[SPEED_1GBps].ser_switch = 940;
    // prepare log_schedule
    log_schedule.period_ns = 2000;
    log_schedule.time_start_ns = 1500;
    log_schedule.time_end_ns = 1900;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem2), &fsc_schedule_mem2);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem3), &fsc_schedule_mem3);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem4), &fsc_schedule_mem4);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem5), &fsc_schedule_mem5);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem6), &fsc_schedule_mem6);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem7), &fsc_schedule_mem7);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem8), &fsc_schedule_mem8);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem9), &fsc_schedule_mem9);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem10), &fsc_schedule_mem10);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    result = create_window_sysfs_items(&log_schedule, &module, 80, 111, 12, false);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(10, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(4, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(134414336, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(23, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(29, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(134414336, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(48, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(54, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(134414336, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(73, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(79, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(134414336, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(98, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(104, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(134414336, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(123, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
}

void test_create_window_sysfs_items_recovery(void) {

    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem3 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem4 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_PARALLEL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 10000;
    module.module_delays[SPEED_1GBps].chip_in = 50;
    module.module_delays[SPEED_1GBps].chip_eg = 120;
    module.module_delays[SPEED_1GBps].phy_in = 298;
    module.module_delays[SPEED_1GBps].phy_eg = 199;
    module.module_delays[SPEED_1GBps].ser_bypass = 439;
    module.module_delays[SPEED_1GBps].ser_switch = 940;
    // prepare log_schedule
    log_schedule.period_ns = 5000;
    log_schedule.time_start_ns = 1200;
    log_schedule.time_end_ns = 3800;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem2), &fsc_schedule_mem2);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem3), &fsc_schedule_mem3);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem4), &fsc_schedule_mem4);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    result = create_window_sysfs_items(&log_schedule, &module, 80, 111, 12, true);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(4, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(19, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(52, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(201523311, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(81, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(115, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(201523311, next_item->hw_schedule_item.cmd);
}

void test_create_window_sysfs_items_start_next_period(void) {

    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 5000;
    module.module_delays[SPEED_1GBps].chip_in = 50;
    module.module_delays[SPEED_1GBps].chip_eg = 120;
    module.module_delays[SPEED_1GBps].phy_in = 298;
    module.module_delays[SPEED_1GBps].phy_eg = 199;
    module.module_delays[SPEED_1GBps].ser_bypass = 439;
    module.module_delays[SPEED_1GBps].ser_switch = 940;
    // prepare log_schedule
    log_schedule.period_ns = 5000;
    log_schedule.time_start_ns = 4700;
    log_schedule.time_end_ns = 4900;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem2), &fsc_schedule_mem2);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    result = create_window_sysfs_items(&log_schedule, &module, 80, 111, 12, true);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(4, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(201523311, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(12, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
}

void test_create_window_sysfs_items_tiny_gap_start_end(void) {

    int result;
    int tick_dauer = 10;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_PARALLEL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 200000;
    module.module_delays[SPEED_1GBps].chip_in = 50;
    module.module_delays[SPEED_1GBps].chip_eg = 120;
    module.module_delays[SPEED_1GBps].phy_in = 562; //642
    module.module_delays[SPEED_1GBps].phy_eg = 199;
    module.module_delays[SPEED_1GBps].ser_bypass = 439;
    module.module_delays[SPEED_1GBps].ser_switch = 940;
    // prepare log_schedule
    log_schedule.period_ns = 200000;
    log_schedule.time_start_ns = 199388;
    log_schedule.time_end_ns = 199308;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem2), &fsc_schedule_mem2);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    result = create_window_sysfs_items(&log_schedule, &module, tick_dauer, 111, 12, true);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(0, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(268632064, next_item->hw_schedule_item.cmd);
    next_item = ACMLIST_NEXT(next_item, entry);
    TEST_ASSERT_EQUAL(19992, next_item->hw_schedule_item.abs_cycle);
    TEST_ASSERT_EQUAL_UINT32(201523311, next_item->hw_schedule_item.cmd);
}

void test_create_window_sysfs_items_neg_conn_mode_default(void) {
    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;

    //init module
    module.cycle_ns = 10000;
    module.mode = 4;	//undefined value
    // prepare log_schedule
    log_schedule.period_ns = 2000;
    log_schedule.time_start_ns = 1500;
    log_schedule.time_end_ns = 1900;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    acm_free_Expect(&fsc_schedule_mem1);
    logging_Expect(0, "Sysfs: connection mode has undefined value: %d");

    result = create_window_sysfs_items(&log_schedule, &module, 80, 111, 12, false);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_create_window_sysfs_items_null_start(void) {
    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;

    //init module
    module.cycle_ns = 10000;
    // prepare log_schedule
    log_schedule.period_ns = 2000;
    log_schedule.time_start_ns = 1500;
    log_schedule.time_end_ns = 1900;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), NULL);
    logging_Expect(0, "Sysfs: Out of memory");

    result = create_window_sysfs_items(&log_schedule, &module, 80, 111, 12, false);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_create_window_sysfs_items_null_end(void) {

    int result;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_PARALLEL,
            SPEED_1GBps,
            MODULE_0,
            NULL);
    struct schedule_entry log_schedule = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command *next_item;

    //init module
    module.cycle_ns = 10000;
    module.module_delays[SPEED_1GBps].chip_in = 50;
    module.module_delays[SPEED_1GBps].chip_eg = 120;
    module.module_delays[SPEED_1GBps].phy_in = 298;
    module.module_delays[SPEED_1GBps].phy_eg = 199;
    module.module_delays[SPEED_1GBps].ser_bypass = 439;
    module.module_delays[SPEED_1GBps].ser_switch = 940;
    // prepare log_schedule
    log_schedule.period_ns = 2000;
    log_schedule.time_start_ns = 1500;
    log_schedule.time_end_ns = 1900;

    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), &fsc_schedule_mem1);
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    acm_zalloc_ExpectAndReturn(sizeof (fsc_schedule_mem1), NULL);
    logging_Expect(0, "Sysfs: Out of memory");

    result = create_window_sysfs_items(&log_schedule, &module, 80, 111, 12, false);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.fsc_list));
    next_item = ACMLIST_FIRST(&module.fsc_list);
    TEST_ASSERT_EQUAL(23, next_item->hw_schedule_item.abs_cycle);
}

void test_sysfs_construct_path_name(void) {
    char path_name[SYSFS_PATH_LENGTH];
    int result;
    char expected_path[] = ACMDEV_BASE "config_bin/sched_tab_row";

    result = sysfs_construct_path_name(path_name,
            SYSFS_PATH_LENGTH,
            __stringify(ACMDRV_SYSFS_CONFIG_GROUP),
            __stringify(ACM_SYSFS_SCHED_TAB));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING(expected_path, path_name);
}

// sysfs_write_delay_file() has been removed
#if 0

void test_sysfs_write_delay_file_module_0(void) {
    int result, fd, read_value;
    char speed_100MB[] = "100";
    char tx_dir[] = "tx";
    char written_value[10];

    result = sysfs_write_delay_file(0, 440, speed_100MB, tx_dir);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(DELAY_BASE "sw0p2/phy/delay100tx", O_RDONLY);
    result = read(fd, written_value, 10);
    TEST_ASSERT_EQUAL(4, result);
    read_value = strtoul(written_value, NULL, 0);
    TEST_ASSERT_EQUAL(440, read_value);
}

void test_sysfs_write_delay_file_module_1(void) {
    int result, fd, read_value;
    char speed_100MB[] = "100";
    char rx_dir[] = "rx";
    char written_value[10];

    result = sysfs_write_delay_file(1, 21, speed_100MB, rx_dir);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(DELAY_BASE "sw0p3/phy/delay100rx", O_RDONLY);
    result = read(fd, written_value, 10);
    TEST_ASSERT_EQUAL(3, result);
    read_value = strtoul(written_value, NULL, 0);
    TEST_ASSERT_EQUAL(21, read_value);
}

void test_sysfs_write_delay_file_neg_module(void) {
    int result;
    char speed_100MB[] = "100";
    char rx_dir[] = "rx";

    result = sysfs_write_delay_file(3, 213, speed_100MB, rx_dir);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

#endif

void test_sysfs_write_configuration_id(void) {
    int result, fd;
    uint32_t written_value;

    result = sysfs_write_configuration_id(130986);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/configuration_id", O_RDONLY);
    result = read(fd, &written_value, sizeof(uint32_t));
    TEST_ASSERT_EQUAL(sizeof(uint32_t), result);
    close(fd);
    TEST_ASSERT_EQUAL(130986, written_value);
}

void test_sysfs_read_configuration_id(void) {
    int fd;
    uint32_t read_value, test_value;

    // prepare testcase
    test_value = 500000;
    fd = open(ACMDEV_BASE "config_bin/configuration_id", O_WRONLY);
    pwrite(fd, (char*) &test_value, sizeof(uint32_t), 0);
    close(fd);

    read_value = sysfs_read_configuration_id();
    TEST_ASSERT_EQUAL(test_value, read_value);
}

void test_check_buff_name_against_sys_devices_msgbuf_not_exists(void) {
    int result;
    char file[ACM_MAX_NAME_SIZE + 1] = "acmbuf_buffer_1";

    result = check_buff_name_against_sys_devices(file);
    TEST_ASSERT_EQUAL(0, result);
}

void test_sysfs_write_msg_buff_to_HW_BUFF_DESC(void) {
    char path[] = ACMDEV_BASE "config_bin/msg_buff_desc";
    int result, fd;
    struct buffer_list bufferlist;
    memset(&bufferlist, 0, sizeof (bufferlist));
    struct sysfs_buffer msg_buf1;
    memset(&msg_buf1, 0, sizeof (msg_buf1));
    struct acmdrv_buff_desc read_desc;
    char buff_name[] = "erster_Name";

    //prepare test
    //init bufferlist
    ACMLIST_INIT(&bufferlist);
    // init msg buffer item
    msg_buf1.msg_buff_index = 0;
    msg_buf1.msg_buff_offset = 0;
    msg_buf1.reset = false;
    msg_buf1.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
    msg_buf1.buff_size = 20;
    msg_buf1.timestamp = true;
    msg_buf1.valid = true;
    msg_buf1.msg_buff_name = buff_name;
    // add item to list
    _ACMLIST_INSERT_TAIL(&bufferlist, &msg_buf1, entry);

    //execute test
    pthread_mutex_lock_ExpectAndReturn(&bufferlist.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&bufferlist.lock, 0);
    result = sysfs_write_msg_buff_to_HW(&bufferlist, BUFF_DESC);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    pread(fd, &read_desc, sizeof (read_desc), 0);
    close(fd);
    TEST_ASSERT_EQUAL(msg_buf1.msg_buff_offset, acmdrv_buff_desc_offset_read(&read_desc));
    TEST_ASSERT_EQUAL(msg_buf1.reset, acmdrv_buff_desc_reset_read(&read_desc));
    TEST_ASSERT_EQUAL(msg_buf1.stream_direction, acmdrv_buff_desc_type_read(&read_desc));
    TEST_ASSERT_EQUAL(msg_buf1.buff_size, acmdrv_buff_desc_sub_buffer_size_read(&read_desc));
    TEST_ASSERT_EQUAL(msg_buf1.timestamp, acmdrv_buff_desc_timestamp_read(&read_desc));
    TEST_ASSERT_EQUAL(msg_buf1.valid, acmdrv_buff_desc_valid_read(&read_desc));
}

void test_sysfs_write_msg_buff_to_HW_BUFF_ALIAS(void) {
    char path[] = ACMDEV_BASE "/config_bin/msg_buff_alias";
    int result, fd;
    struct buffer_list bufferlist;
    memset(&bufferlist, 0, sizeof (bufferlist));
    struct sysfs_buffer msg_buf1;
    memset(&msg_buf1, 0, sizeof (msg_buf1));
    struct acmdrv_buff_alias read_alias;
    char buff_name[] = "erster_Name";

    //prepare test
    //init bufferlist
    ACMLIST_INIT(&bufferlist);
    // init msg buffer item
    msg_buf1.msg_buff_index = 0;
    msg_buf1.msg_buff_offset = 0;
    msg_buf1.reset = false;
    msg_buf1.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
    msg_buf1.buff_size = 20;
    msg_buf1.timestamp = true;
    msg_buf1.valid = true;
    msg_buf1.msg_buff_name = buff_name;
    // add item to list
    _ACMLIST_INSERT_TAIL(&bufferlist, &msg_buf1, entry);

    //execute test
    pthread_mutex_lock_ExpectAndReturn(&bufferlist.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&bufferlist.lock, 0);
    result = sysfs_write_msg_buff_to_HW(&bufferlist, BUFF_ALIAS);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    pread(fd, &read_alias, sizeof (read_alias), 0);
    close(fd);
    TEST_ASSERT_EQUAL(read_alias.idx, msg_buf1.msg_buff_index);
    TEST_ASSERT_EQUAL_STRING(read_alias.alias, msg_buf1.msg_buff_name);
}

void test_sysfs_write_msg_buff_to_HW_BUFF_ALIAS_neg_driver_resp(void) {
    int result;
    struct buffer_list bufferlist;
    memset(&bufferlist, 0, sizeof (bufferlist));
    struct sysfs_buffer msg_buf1;
    memset(&msg_buf1, 0, sizeof (msg_buf1));

    //prepare test
    //init bufferlist
    ACMLIST_INIT(&bufferlist);
    // init msg buffer item
    msg_buf1.msg_buff_index = 0;
    msg_buf1.msg_buff_offset = 0;
    msg_buf1.reset = false;
    msg_buf1.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
    msg_buf1.buff_size = 20;
    msg_buf1.timestamp = true;
    msg_buf1.valid = true;
    // add item to list
    _ACMLIST_INSERT_TAIL(&bufferlist, &msg_buf1, entry);

    //execute test
    pthread_mutex_lock_ExpectAndReturn(&bufferlist.lock, 0);
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem creating msg buffer alias %s ");
    pthread_mutex_unlock_ExpectAndReturn(&bufferlist.lock, 0);
    result = sysfs_write_msg_buff_to_HW(&bufferlist, BUFF_ALIAS);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_sysfs_write_buffer_control_mask(void) {
    char path[] = ACMDEV_BASE "/control_bin/lock_msg_bufs";
    int result, fd;
    uint64_t vector = 2779096485; // 0xA5A5A5A5
    struct acmdrv_msgbuf_lock_ctrl read_vector;

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);
    result = sysfs_write_buffer_control_mask(vector, __stringify(ACM_SYSFS_LOCK_BUFFMASK));
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    pread(fd, &read_vector, sizeof (read_vector), 0);
    close(fd);
    TEST_ASSERT_EQUAL_MEMORY(&vector, &read_vector, sizeof (vector));
}

void test_sysfs_write_buffer_control_mask_neg_read_buffer_num(void) {
    int result;
    uint64_t vector = 0xA5A5A5A5A5ULL;

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), -ENOMEM);
    logging_Expect(0, "Sysfs: invalid number of message buffers: %d");
    result = sysfs_write_buffer_control_mask(vector, __stringify(ACM_SYSFS_LOCK_BUFFMASK));
    TEST_ASSERT_EQUAL(-EACMNUMMESSBUFF, result);
}

void test_sysfs_write_buffer_control_mask_neg_too_much_buffers(void) {
    int result;
    uint64_t vector = 0xA5A5A5A5A5ULL;

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);
    logging_Expect(0, "Sysfs: too many message buffers used. Only %d are available");
    result = sysfs_write_buffer_control_mask(vector, __stringify(ACM_SYSFS_LOCK_BUFFMASK));
    TEST_ASSERT_EQUAL(-EACMNUMMESSBUFF, result);
}

void test_sysfs_write_cntl_speed_wrong_speed(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    module.speed = 2;

    logging_Expect(0, "Sysfs: speed: %d out of range");
    result = sysfs_write_cntl_speed(&module);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_sysfs_write_cntl_speed_100(void) {
    char path[] = ACMDEV_BASE "/config_bin/cntl_speed";
    int result, fd;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    uint32_t speed_read;

    module.speed = SPEED_100MBps;
    module.module_id = 1;

    result = sysfs_write_cntl_speed(&module);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    pread(fd,
            &speed_read,
            sizeof(enum acmdrv_bypass_speed_select),
            sizeof(enum acmdrv_bypass_speed_select));
    close(fd);
    TEST_ASSERT_EQUAL(ACMDRV_BYPASS_SPEED_SELECT_100, acmdrv_bypass_speed_read(speed_read));
}

void test_sysfs_write_cntl_speed_1000(void) {
    char path[] = ACMDEV_BASE "/config_bin/cntl_speed";
    int result, fd;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    uint32_t speed_read;

    module.speed = SPEED_1GBps;
    module.module_id = 0;

    result = sysfs_write_cntl_speed(&module);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    pread(fd, &speed_read, sizeof(enum acmdrv_bypass_speed_select), 0);
    close(fd);
    TEST_ASSERT_EQUAL(ACMDRV_BYPASS_SPEED_SELECT_1000, acmdrv_bypass_speed_read(speed_read));
}

void test_sysfs_get_msg_buff_config_item_praefix(void) {
    int result;
    char praefix[20];

    result = sysfs_get_configfile_item(KEY_PRAEFIX, praefix, sizeof (praefix));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("acmbuf_", praefix);
}

void test_sysfs_get_msg_buff_config_item_neg_value_length(void) {
    int result;
    char praefix[3];

    logging_Expect(LOGLEVEL_ERR,
            "Sysfs: configuration value %s has %d characters, but only %d supported");
    result = sysfs_get_configfile_item(KEY_PRAEFIX, praefix, sizeof (praefix));
    TEST_ASSERT_EQUAL(-EACMCONFIGVAL, result);
}

void test_sysfs_get_msg_buff_config_item_neg_confkey(void) {
    int result;
    char read_value[20];

    logging_Expect(LOGLEVEL_INFO, "Sysfs: configuration item not found %s");
    result = sysfs_get_configfile_item("KEY_NOT_FOUND", read_value, sizeof (read_value));
    TEST_ASSERT_EQUAL(-EACMCONFIG, result);
}

void test_sysfs_get_msg_buff_config_item_delays(void) {
    int result;
    char delay[20];

    result = sysfs_get_configfile_item(KEY_CHIP_IN_100MBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("10", delay);
    result = sysfs_get_configfile_item(KEY_CHIP_EG_100MBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("20", delay);
    result = sysfs_get_configfile_item(KEY_PHY_IN_100MBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("30", delay);
    result = sysfs_get_configfile_item(KEY_PHY_EG_100MBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("40", delay);
    result = sysfs_get_configfile_item(KEY_SER_BYPASS_100MBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("50", delay);
    result = sysfs_get_configfile_item(KEY_SER_SWITCH_100MBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("60", delay);
    result = sysfs_get_configfile_item(KEY_CHIP_IN_1GBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("0100", delay);
    result = sysfs_get_configfile_item(KEY_CHIP_EG_1GBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("200", delay);
    result = sysfs_get_configfile_item(KEY_PHY_IN_1GBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("a300", delay);
    result = sysfs_get_configfile_item(KEY_PHY_EG_1GBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("400", delay);
    result = sysfs_get_configfile_item(KEY_SER_BYPASS_1GBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("500", delay);
    result = sysfs_get_configfile_item(KEY_SER_SWITCH_1GBps, delay, sizeof (delay));
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL_STRING("600", delay);
}

void test_sysfs_write_config_status_to_HW(void) {
    int result;

    result = sysfs_write_config_status_to_HW(ACMDRV_CONFIG_END_STATE);
    TEST_ASSERT_EQUAL(0, result);
}

void test_sysfs_write_connection_mode_to_HW_serial(void) {
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_1,
            NULL);
    int result, fd;
    enum acmdrv_conn_mode read_value;

    result = sysfs_write_connection_mode_to_HW(&module);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/cntl_connection_mode", O_RDONLY);
    result = pread(fd, &read_value, sizeof(uint32_t), 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(ACMDRV_BYPASS_CONN_MODE_SERIES, read_value);
}

void test_sysfs_write_connection_mode_to_HW_parallel(void) {
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_PARALLEL,
            SPEED_100MBps,
            MODULE_0,
            NULL);
    int result, fd;
    enum acmdrv_conn_mode read_value;

    result = sysfs_write_connection_mode_to_HW(&module);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/cntl_connection_mode", O_RDONLY);
    result = pread(fd, &read_value, sizeof(uint32_t), 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(ACMDRV_BYPASS_CONN_MODE_PARALLEL, read_value);
}

void test_sysfs_write_connection_mode_to_HW_wrong_mode(void) {
    struct acm_module module = MODULE_INITIALIZER(module, 4, SPEED_100MBps, MODULE_0, NULL);
    int result;

    logging_Expect(0, "Sysfs: module mode has undefined value");
    result = sysfs_write_connection_mode_to_HW(&module);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_sysfs_write_redund_ctrl_table(void) {
    char path[] = ACMDEV_BASE "/config_bin/redund_cnt_tab";
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result, i, fd;
    uint32_t redund_cnt_tab[4] = { 0x0, 0x0, 0x0, 0x0 };
    uint32_t read_redund_cnt_tab[4];
    uint32_t expect_redund_cnt_tab_row[4] = { 0x00000002, 0x00014100, 0x00020302, 0x00030302 };
    /* table item 0: 0x00000002 - NOP
     * table item 1: 0x00010302 - index 1, UPD_FIN_BOTH(=3), SRC_INTSEQNUM(=2)
     * table item 2: 0x00020302 - index 2, UPD_FIN_BOTH(=3), SRC_INTSEQNUM(=2)
     * table item 3: 0x00030302 - index 3, UPD_FIN_BOTH(=3), SRC_INTSEQNUM(=2)
     */
    struct acm_stream stream[5];
    for (i = 0; i < 5; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }
    stream[0].type = REDUNDANT_STREAM_RX;
    stream[0].redundand_index = REDUNDANCY_START_IDX;
    stream[3].type = REDUNDANT_STREAM_TX;
    stream[3].redundand_index = REDUNDANCY_START_IDX + 1;
    stream[4].type = REDUNDANT_STREAM_TX;
    stream[4].redundand_index = REDUNDANCY_START_IDX + 2;
    stream[1].type = EVENT_STREAM;
    stream[2].type = INGRESS_TRIGGERED_STREAM;

    /* prepare test case */
    /* initialize file redund_cnt_tab: zeilenweises Schreiben von Nullen */
    fd = open(path, O_WRONLY);
    for (i = 0; i < 16; i++) {
        pwrite(fd, &redund_cnt_tab, sizeof (redund_cnt_tab), i * sizeof (redund_cnt_tab));
    }
    close(fd);
    /* initialize module */
    module.module_id = 1;
    ACMLIST_INIT(&module.streams);
    /* initialize streams */
    for (i = 0; i < 5; i++)
        _ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);

    pthread_mutex_lock_ExpectAndReturn(&module.streams.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.streams.lock, 0);

    /* execute test case */
    result = sysfs_write_redund_ctrl_table(&module);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    for (i = 0; i < 16; i++) {
        pread(fd,
                &read_redund_cnt_tab,
                sizeof (read_redund_cnt_tab),
                i * sizeof (read_redund_cnt_tab));
        if ( (i < 8) || (i > 8)) {
            TEST_ASSERT_EQUAL_HEX32_ARRAY(redund_cnt_tab, read_redund_cnt_tab, 4);
        } else {
            TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_redund_cnt_tab_row, read_redund_cnt_tab, 4);
        }
    }
    close(fd);
}

void test_sysfs_write_scatter_dma(void) {
    char path[] = ACMDEV_BASE "/config_bin/scatter_dma";
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1, NULL);
    int result, i, fd;
    uint32_t scatter_tab_row_init[4] = { 0x0, 0x0, 0x0, 0x0 };
    uint32_t read_scatter_tab_row[4];
    uint32_t expect_scatter_tab_row1[4] = { 0x00000005, 0x00050054, 0x04040065, 0x080300c5 };
    uint32_t expect_scatter_tab_row2[4] = { 0x0c080144, 0x10028195, 0x0, 0x0 };
    struct acm_stream stream[5] = {
        STREAM_INITIALIZER_SCATTER_DMA_IDX(stream[0], REDUNDANT_STREAM_TX, 0),
        STREAM_INITIALIZER_SCATTER_DMA_IDX(stream[1], INGRESS_TRIGGERED_STREAM, SCATTER_START_IDX),
        STREAM_INITIALIZER_SCATTER_DMA_IDX(stream[2], EVENT_STREAM, 0),
        STREAM_INITIALIZER_SCATTER_DMA_IDX(stream[3], INGRESS_TRIGGERED_STREAM, SCATTER_START_IDX + 2),
        STREAM_INITIALIZER_SCATTER_DMA_IDX(stream[4], INGRESS_TRIGGERED_STREAM, SCATTER_START_IDX + 3),
    };
    struct sysfs_buffer message_buffer[5] = {
        BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
        BUFFER_INITIALIZER(1, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
        BUFFER_INITIALIZER(2, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
        BUFFER_INITIALIZER(3, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
        BUFFER_INITIALIZER(4, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
    };

    struct operation op_item[7] = {
            READ_OPERATION_INITIALIZER(5, 10, NULL),
            READ_OPERATION_INITIALIZER(6, 8, NULL),
            READ_OPERATION_INITIALIZER(12, 6, NULL),
            READ_OPERATION_INITIALIZER(20, 16, NULL),
            READ_OPERATION_INITIALIZER(25, 5, NULL),
            INSERT_OPERATION_INITIALIZER(0, NULL, NULL),
            FORWARD_OPERATION_INITIALIZER(0, 0)
    };

    /* prepare test case */
    /* initialize file scatter_dma: zeilenweises Schreiben von Nullen */
    fd = open(path, O_WRONLY);
    for (i = 0; i < 128; i++) {
        pwrite(fd,
                &scatter_tab_row_init,
                sizeof (scatter_tab_row_init),
                i * sizeof (scatter_tab_row_init));
    }
    close(fd);

    /* add streams to module */
    for (i = 0; i < 5; i++)
        _ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);

    /* initialize operations */
    for (i = 0; i < 5; ++i)
        op_item[i].msg_buf = &message_buffer[i];

    _ACMLIST_INSERT_TAIL(&stream[1].operations, &op_item[0], entry);
    _ACMLIST_INSERT_TAIL(&stream[1].operations, &op_item[1], entry);
    _ACMLIST_INSERT_TAIL(&stream[3].operations, &op_item[2], entry);
    _ACMLIST_INSERT_TAIL(&stream[4].operations, &op_item[3], entry);
    _ACMLIST_INSERT_TAIL(&stream[4].operations, &op_item[5], entry);
    _ACMLIST_INSERT_TAIL(&stream[4].operations, &op_item[4], entry);
    _ACMLIST_INSERT_TAIL(&stream[1].operations, &op_item[6], entry);

    pthread_mutex_lock_ExpectAndReturn(&module.streams.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[1].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[1].operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[1].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[1].operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[3].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[3].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.streams.lock, 0);

    /* execute test case */
    result = sysfs_write_scatter_dma(&module);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(path, O_RDONLY);
    for (i = 0; i < 128; i++) {
        pread(fd,
                &read_scatter_tab_row,
                sizeof (read_scatter_tab_row),
                i * sizeof (read_scatter_tab_row));
        if (i == 64) {
            TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_scatter_tab_row1, read_scatter_tab_row, 4);
        } else {
            if (i == 65) {
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_scatter_tab_row2, read_scatter_tab_row, 4);
            } else {
                TEST_ASSERT_EQUAL_HEX32_ARRAY(scatter_tab_row_init, read_scatter_tab_row, 4);
            }
        }
    }
    close(fd);
}

void test_sysfs_write_prefecher_gather_dma(void) {
    int result, i, fd;
    char path_gather[] = ACMDEV_BASE "/config_bin/gather_dma";
    char path_prefetch[] = ACMDEV_BASE "/config_bin/prefetch_dma";
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1, NULL);
    uint32_t tab_row_init[4] = { 0x0, 0x0, 0x0, 0x0 };
    uint32_t read_gather_tab_row[4];
    uint32_t read_prefetch_tab_row[4];
    /* expected in prefetch and gather table:
     *
     * prefetch table			gather table
     * NOP						NOP
     * NOP						Forward_all
     * NOP						insert constant - begin REDUNDANT_STREAM_TX
     * 							forward
     * 							forward
     * 							r-Tag
     * 							PAD
     * lock vector 0-15			forward - begin EVENT_STREAM
     * mov_msg_buff				forward
     * mov_msg_buff				forward
     * mov_msg_buff				insert
     * 							insert
     * 							insert
     * lock vector 0-15			forward - begin TIME_TRIGGERED_STREAM
     * mov_msg_buff				forward
     * 							forward
     * 							insert
     * 									begin INGRESS_TRIGGERED_STREAM without gather commands
     */
    uint32_t expect_prefetch_tab_row1[4] = { 0x00000002, 0x00000002, 0x00000002, 0x00 };
    uint32_t expect_prefetch_tab_row2[4] = { 0x00, 0x00, 0x00, 0x000e0006 };
    uint32_t expect_prefetch_tab_row3[4] = { 0x00010404, 0x000200a4, 0x000300a5, 0x00 };
    uint32_t expect_prefetch_tab_row4[4] = { 0x00, 0x00100006, 0x000400a5, 0x00 };
    uint32_t expect_gather_tab_row1[4] = { 0x0000000b, 0x00000002, 0x000000c8, 0x000600ca };
    uint32_t expect_gather_tab_row2[4] = { 0x000c008a, 0x0000000e, 0x004101ed, 0x000000ca };
    uint32_t expect_gather_tab_row3[4] = { 0x000600ca, 0x000c00ca, 0x00000004, 0x00000004 };
    uint32_t expect_gather_tab_row4[4] = { 0x00000005, 0x000000ca, 0x000600ca, 0x000c00ca };
    uint32_t expect_gather_tab_row5[4] = { 0x00000005, 0x0, 0x0, 0x0 };

    struct acm_stream stream[5] = {
            STREAM_INITIALIZER(stream[0], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[1], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[2], EVENT_STREAM),
            STREAM_INITIALIZER(stream[3], TIME_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[4], INGRESS_TRIGGERED_STREAM) };

    struct sysfs_buffer message_buffer =
            BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL);
    char const_data[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char pad_value = 'A';
    struct sysfs_buffer mes_buffer_event[3] = {
            BUFFER_INITIALIZER(1, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
            BUFFER_INITIALIZER(2, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL),
            BUFFER_INITIALIZER(3, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL)
    };
    struct sysfs_buffer mes_buffer_time =
            BUFFER_INITIALIZER(4, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL);

    struct operation op_item[16] = {
            READ_OPERATION_INITIALIZER(5, 10, NULL),
            FORWARD_ALL_OPERATION_INITIALIZER,
            INSERT_CONSTANT_OPERATION_INITIALIZER(const_data, sizeof (const_data)),
            FORWARD_OPERATION_INITIALIZER(6, 6),
            FORWARD_OPERATION_INITIALIZER(12, 4),
            PAD_OPERATION_INITIALIZER(15, NULL),
            INSERT_OPERATION_INITIALIZER(32, "testname", &mes_buffer_event[0]),
            INSERT_OPERATION_INITIALIZER(5, "name2_insert", &mes_buffer_event[1]),
            INSERT_OPERATION_INITIALIZER(5, "name3_event", &mes_buffer_event[2]),
            INSERT_OPERATION_INITIALIZER(5, "name_time_triggered", &mes_buffer_time),
            FORWARD_OPERATION_INITIALIZER(0, 6),
            FORWARD_OPERATION_INITIALIZER(6, 6),
            FORWARD_OPERATION_INITIALIZER(12, 6),
            FORWARD_OPERATION_INITIALIZER(0, 6),
            FORWARD_OPERATION_INITIALIZER(6, 6),
            FORWARD_OPERATION_INITIALIZER(12, 6) };

    /* prepare test case */
    /* initialize file prefetch_dma: zeilenweises Schreiben von Nullen */
    fd = open(path_gather, O_WRONLY);
    for (i = 0; i < 128; i++) {
        pwrite(fd, &tab_row_init, sizeof (tab_row_init), i * sizeof (tab_row_init));
    }
    close(fd);
    /* initialize file gather_dma: zeilenweises Schreiben von Nullen */
    fd = open(path_prefetch, O_WRONLY);
    for (i = 0; i < 128; i++) {
        pwrite(fd, &tab_row_init, sizeof (tab_row_init), i * sizeof (tab_row_init));
    }
    close(fd);

    for (i = 0; i < 5; i++)
        _ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);

    stream[0].gather_dma_index = GATHER_START_IDX;
    stream[1].gather_dma_index = 0;
    stream[2].gather_dma_index = GATHER_START_IDX + 5;
    stream[3].gather_dma_index = GATHER_START_IDX + 5 + 6;
    stream[4].gather_dma_index = GATHER_FORWARD_IDX;

    /* initialize operations */
    // first INGRESS_TRIGGERED_STREAM
    op_item[0].msg_buf = &message_buffer;
    _ACMLIST_INSERT_TAIL(&stream[1].operations, &op_item[0], entry);
    // second INGRESS_TRIGGERED_STREAM
    _ACMLIST_INSERT_TAIL(&stream[4].operations, &op_item[1], entry);
    // REDUNDANT_STREAM_TX
    _ACMLIST_INSERT_TAIL(&stream[0].operations, &op_item[2], entry);
    _ACMLIST_INSERT_TAIL(&stream[0].operations, &op_item[3], entry);
    _ACMLIST_INSERT_TAIL(&stream[0].operations, &op_item[4], entry);
    op_item[5].length = 15;
    op_item[5].data = &pad_value;
    op_item[5].data_size = 1;
    _ACMLIST_INSERT_TAIL(&stream[0].operations, &op_item[5], entry);
    // EVENT_STREAM
    _ACMLIST_INSERT_TAIL(&stream[2].operations, &op_item[10], entry);
    _ACMLIST_INSERT_TAIL(&stream[2].operations, &op_item[11], entry);
    _ACMLIST_INSERT_TAIL(&stream[2].operations, &op_item[12], entry);
    _ACMLIST_INSERT_TAIL(&stream[2].operations, &op_item[6], entry);
    _ACMLIST_INSERT_TAIL(&stream[2].operations, &op_item[7], entry);
    _ACMLIST_INSERT_TAIL(&stream[2].operations, &op_item[8], entry);
    // TIME_TRIGGERED STREAM
    _ACMLIST_INSERT_TAIL(&stream[3].operations, &op_item[13], entry);
    _ACMLIST_INSERT_TAIL(&stream[3].operations, &op_item[14], entry);
    _ACMLIST_INSERT_TAIL(&stream[3].operations, &op_item[15], entry);

    _ACMLIST_INSERT_TAIL(&stream[3].operations, &op_item[9], entry);

    pthread_mutex_lock_ExpectAndReturn(&module.streams.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[1].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[1].operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[4].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[4].operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[0].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[0].operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[0].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[0].operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream[0].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream[0].operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.streams.lock, 0);

    /* execute test case */
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(0, result);
    /* check gather table*/
    fd = open(path_gather, O_RDONLY);
    for (i = 0; i < 128; i++) {
        pread(fd,
                &read_gather_tab_row,
                sizeof (read_gather_tab_row),
                i * sizeof (read_gather_tab_row));
        switch (i) {
            case 64:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_gather_tab_row1, read_gather_tab_row, 4);
                break;
            case 65:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_gather_tab_row2, read_gather_tab_row, 4);
                break;
            case 66:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_gather_tab_row3, read_gather_tab_row, 4);
                break;
            case 67:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_gather_tab_row4, read_gather_tab_row, 4);
                break;
            case 68:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_gather_tab_row5, read_gather_tab_row, 4);
                break;
            default:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(tab_row_init, read_gather_tab_row, 4);
        }
    }
    close(fd);
    /* check prefetch table*/
    fd = open(path_prefetch, O_RDONLY);
    for (i = 0; i < 128; i++) {
        pread(fd,
                &read_prefetch_tab_row,
                sizeof (read_prefetch_tab_row),
                i * sizeof (read_prefetch_tab_row));
        switch (i) {
            case 64:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_prefetch_tab_row1, read_prefetch_tab_row, 4);
                break;
            case 65:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_prefetch_tab_row2, read_prefetch_tab_row, 4);
                break;
            case 66:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_prefetch_tab_row3, read_prefetch_tab_row, 4);
                break;
            case 67:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(expect_prefetch_tab_row4, read_prefetch_tab_row, 4);
                break;
            default:
                TEST_ASSERT_EQUAL_HEX32_ARRAY(tab_row_init, read_prefetch_tab_row, 4);
        }
    }
    close(fd);
}

void test_write_module_schedules_to_HW(void) {
    char path_cycle_time[] = ACMDEV_BASE "/config_bin/sched_cycle_time";
    char path_start_time[] = ACMDEV_BASE "/config_bin/sched_start_table";
    struct acmdrv_sched_cycle_time cycle_time_read, cycle_time_init = { 0, 0 };
    struct acmdrv_timespec64 start_time_read, start_time_init = { 0, 0 };
    struct acmdrv_sched_cycle_time cycle_time_expected = { 0, 0x000002bc };
    struct acmdrv_timespec64 start_time_expected = { 0x00d9038, 0x0000115c };
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result, i, fd;

    /* prepare test case */
    fd = open(path_cycle_time, O_WRONLY);
    for (i = 0; i < 4; i++) {
        pwrite(fd, &cycle_time_init, sizeof (cycle_time_init), i * sizeof (cycle_time_init));
    }
    close(fd);
    /* initialize file gather_dma: zeilenweises Schreiben von Nullen */
    fd = open(path_start_time, O_WRONLY);
    for (i = 0; i < 4; i++) {
        pwrite(fd, &start_time_init, sizeof (start_time_init), i * sizeof (start_time_init));
    }
    close(fd);
    /*initialize module */
    module.module_id = 1;
    module.cycle_ns = 700;
    module.start.tv_nsec = 4444;
    module.start.tv_sec = 888888;

    /* execute test case */
    result = write_module_schedules_to_HW(&module, 0);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    /* check cycle time*/
    fd = open(path_cycle_time, O_RDONLY);
    for (i = 0; i < 4; i++) {
        pread(fd, &cycle_time_read, sizeof (cycle_time_read), i * sizeof (cycle_time_read));
        if (i == 2) {
            TEST_ASSERT_EQUAL_MEMORY(&cycle_time_expected,
                    &cycle_time_read,
                    sizeof (cycle_time_read));
        } else {
            TEST_ASSERT_EQUAL_MEMORY(&cycle_time_init, &cycle_time_read, sizeof (cycle_time_read));
        }
    }
    close(fd);
    /* check start time */
    fd = open(path_start_time, O_RDONLY);
    for (i = 0; i < 4; i++) {
        pread(fd, &start_time_read, sizeof (start_time_read), i * sizeof (start_time_read));
        if (i == 2) {
            TEST_ASSERT_EQUAL_MEMORY(&start_time_expected,
                    &start_time_read,
                    sizeof (start_time_read));
        } else {
            TEST_ASSERT_EQUAL_MEMORY(&start_time_init, &start_time_read, sizeof (start_time_read));
        }
    }
    close(fd);
}

void test_update_fsc_indexes_stand_alone(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct acm_stream stream = STREAM_INITIALIZER_ALL(stream, REDUNDANT_STREAM_TX, NULL, NULL,
    NULL, NULL, 0, 191, 0, 23, 15);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t expected_cmd = 1387790527; // binary: 01010010 10111000 00000000 10111111
    uint32_t input_cmd = 1375731712;     //binary: 01010010 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;
    _ACMLIST_INSERT_TAIL(&stream.windows, &window, entry);

    /* execute test case */
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(expected_cmd, fsc_command.hw_schedule_item.cmd);
}

void test_update_fsc_indexes_neg_schedule_reference(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);

    /* execute test case */
    logging_Expect(0, "Sysfs: fsc_schedule without reference to schedule item");
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_update_fsc_indexes_neg_ref_schedulelist(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;

    /* prepare test case */
    fsc_command.schedule_reference = &window;

    /* execute test case */
    logging_Expect(0, "Sysfs: schedule item without reference to schedule list");
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_update_fsc_indexes_neg_stream_ref(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t input_cmd = 1375731712;     //binary: 01010010 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;

    /* execute test case */
    logging_Expect(0, "Sysfs: schedule item without reference to schedule list");
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_update_fsc_indexes_neg_undefined_fsc_command(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t input_cmd = 1442840576;     //binary: 01010110 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;
    _ACMLIST_INSERT_TAIL(&stream.windows, &window, entry);

    /* execute test case */
    logging_Expect(0, "Sysfs: Undefined trigger command in fsc schedule item");
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_update_fsc_indexes_trigger_stand_alone(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct acm_stream stream = STREAM_INITIALIZER_GATHER_DMA_IDX(stream,
            INGRESS_TRIGGERED_STREAM,
            3);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t input_cmd = 1375731712;     //binary: 01010010 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;
    _ACMLIST_INSERT_TAIL(&stream.windows, &window, entry);

    /* execute test case */
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(0, result);
    //binary: 01010010 00000000 00000000 00000011
    TEST_ASSERT_EQUAL(1375731715, fsc_command.hw_schedule_item.cmd);
}

void test_update_fsc_indexes_trigger_first_stage_no_event(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct acm_stream stream = STREAM_INITIALIZER_GATHER_DMA_IDX(stream,
            INGRESS_TRIGGERED_STREAM,
            3);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t input_cmd = 1409286144;     //binary: 01010100 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;
    _ACMLIST_INSERT_TAIL(&stream.windows, &window, entry);

    /* execute test case */
    logging_Expect(0, "Sysfs: Ingress Triggered Stream misses Event Stream");
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_update_fsc_indexes_trigger_first_stage_no_recovery(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct acm_stream event_stream = STREAM_INITIALIZER(event_stream, EVENT_STREAM);
    struct acm_stream stream = STREAM_INITIALIZER_REFERENCE(stream,
            INGRESS_TRIGGERED_STREAM,
            &event_stream);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t input_cmd = 1409286144;     //binary: 01010100 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;
    _ACMLIST_INSERT_TAIL(&stream.windows, &window, entry);

    /* execute test case */
    logging_Expect(0, "Sysfs: Event Stream misses Recovery Stream");
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_update_fsc_indexes_trigger_first_stage(void) {
    int result;
    struct fsc_command fsc_command = COMMAND_INITIALIZER(0, 0);
    struct acm_stream recov_stream = STREAM_INITIALIZER_GATHER_DMA_IDX(recov_stream,
            RECOVERY_STREAM,
            5);
    struct acm_stream event_stream = STREAM_INITIALIZER_REFERENCE(event_stream,
            EVENT_STREAM,
            &recov_stream);
    struct acm_stream stream = STREAM_INITIALIZER_REFERENCE(stream,
            INGRESS_TRIGGERED_STREAM,
            &event_stream);
    struct schedule_entry window = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t input_cmd = 1409286144;     //binary: 01010100 00000000 00000000 00000000

    /* prepare test case */
    fsc_command.schedule_reference = &window;
    fsc_command.hw_schedule_item.cmd = input_cmd;
    _ACMLIST_INSERT_TAIL(&stream.windows, &window, entry);

    /* execute test case */
    result = update_fsc_indexes(&fsc_command);
    TEST_ASSERT_EQUAL(0, result);
    //binary: 01010100 00000000 00000000 00000111
    TEST_ASSERT_EQUAL(1409286149, fsc_command.hw_schedule_item.cmd);
}

void test_sysfs_write_base_recovery_no_module1(void) {
    int result;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            &configuration);

    /* prepare test case */
    configuration.bypass[0] = &module;

    /* execute test case */
    result = sysfs_write_base_recovery(&configuration);
    TEST_ASSERT_EQUAL(0, result);
}

void test_sysfs_write_base_recovery_no_module0(void) {
    int result;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_1,
            &configuration);

    /* prepare test case */
    configuration.bypass[1] = &module;

    /* execute test case */
    result = sysfs_write_base_recovery(&configuration);
    TEST_ASSERT_EQUAL(0, result);
}

void test_sysfs_write_base_recovery(void) {
    int result, fd;
    uint32_t read_data[32];
    uint32_t expected[32] = {
            0, 1500, 0, 0, 0, 0, 0, 1500,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0
    };
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_0, NULL),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_1, NULL)
    };
    struct acm_stream stream[6] = {
            STREAM_INITIALIZER(stream[0], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[2], REDUNDANT_STREAM_RX),
            STREAM_INITIALIZER(stream[3], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[4], REDUNDANT_STREAM_RX),
            STREAM_INITIALIZER(stream[5], TIME_TRIGGERED_STREAM)
    };

    /* prepare test case */
    config.bypass[0] = &module[0];
    config.bypass[1] = &module[1];

    stream[1].reference_redundant = &stream[3];
    stream[3].reference_redundant = &stream[1];
    stream[1].redundand_index = 1;
    stream[3].redundand_index = 1;
    stream[2].reference_redundant = &stream[4];
    stream[4].reference_redundant = &stream[2];
    stream[2].redundand_index = 7;
    stream[4].redundand_index = 7;

    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[0], entry);
    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[1], entry);
    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[2], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[3], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[4], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[5], entry);

    /* execute test case */
    pthread_mutex_lock_ExpectAndReturn(&module[0].streams.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module[0].streams.lock, 0);
    result = sysfs_write_base_recovery(&config);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/base_recovery", O_RDONLY);
    result = pread(fd, &read_data, sizeof (read_data), 0);
    close(fd);
    TEST_ASSERT_EQUAL_INT32_ARRAY(&expected, &read_data, ACMDRV_REDUN_TABLE_ENTRY_COUNT);
}

void test_sysfs_write_individual_recovery(void) {
    int result, fd;
    uint32_t read_data[ACMDRV_BYPASS_NR_RULES][ACMDRV_BYPASS_MODULES_COUNT];
    uint32_t expected[ACMDRV_BYPASS_NR_RULES][ACMDRV_BYPASS_MODULES_COUNT] = {
            0, 2000, 2500, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 3500, 0, 0, 0, 5000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_0, &config),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_1, &config)
    };
    struct acm_stream stream[6] = {
            STREAM_INITIALIZER(stream[0], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], REDUNDANT_STREAM_RX),
            STREAM_INITIALIZER(stream[2], REDUNDANT_STREAM_RX),
            STREAM_INITIALIZER(stream[3], REDUNDANT_STREAM_RX),
            STREAM_INITIALIZER(stream[4], REDUNDANT_STREAM_RX),
            STREAM_INITIALIZER(stream[5], TIME_TRIGGERED_STREAM)
    };

    /* prepare test case */
    stream[1].reference_redundant = &stream[3];
    stream[3].reference_redundant = &stream[1];
    stream[1].redundand_index = 1;
    stream[3].redundand_index = 1;
    stream[2].reference_redundant = &stream[4];
    stream[4].reference_redundant = &stream[2];
    stream[2].redundand_index = 7;
    stream[4].redundand_index = 7;
    stream[1].lookup_index = 1;
    stream[2].lookup_index = 2;
    stream[3].lookup_index = 1;
    stream[4].lookup_index = 5;
    stream[1].indiv_recov_timeout_ms = 2000;
    stream[2].indiv_recov_timeout_ms = 2500;
    stream[3].indiv_recov_timeout_ms = 3500;
    stream[4].indiv_recov_timeout_ms = 5000;

    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[0], entry);
    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[1], entry);
    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[2], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[3], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[4], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[5], entry);

    pthread_mutex_lock_ExpectAndReturn(&module[0].streams.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module[0].streams.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&module[1].streams.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module[1].streams.lock, 0);

    /* execute test case */
    result = sysfs_write_individual_recovery(&module[0]);
    TEST_ASSERT_EQUAL(0, result);
    result = sysfs_write_individual_recovery(&module[1]);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/individual_recovery", O_RDONLY);
    result = pread(fd, &read_data, sizeof (read_data), 0);
    close(fd);
    TEST_ASSERT_EQUAL_INT32_ARRAY(&expected,
            &read_data,
            ACMDRV_BYPASS_NR_RULES * ACMDRV_BYPASS_MODULES_COUNT);
}

void test_read_buffer_sysfs_item_neg_open_file(void) {
    int result;
    char pathname[] = "config_bin/individual_recovery";
    uint32_t read_value;

    logging_Expect(0, "Sysfs: open file %s failed");
    result = read_buffer_sysfs_item(pathname, (void*) &read_value, sizeof (read_value), 0);
    TEST_ASSERT_EQUAL(-ENOENT, result);
}

void test_read_buffer_sysfs_item_neg_read_less(void) {
    int result;
    char pathname[] = ACMDEV_BASE "config_bin/clear_all_fpga";
    uint32_t read_value;

    logging_Expect(2, "Sysfs: less data read than expected from file %s. expected %d, read %d");
    result = read_buffer_sysfs_item(pathname, (void*) &read_value, sizeof (read_value), 50);
    TEST_ASSERT_EQUAL(0, result);
}

void test_read_buffer_sysfs_item_neg_read_file(void) {
    int result;
    char pathname[] = ACMDEV_BASE "config_bin/clear_all_fpga";
    uint32_t read_value;

    logging_Expect(0, "Sysfs: problem reading data %s");
    result = read_buffer_sysfs_item(pathname, (void*) &read_value, sizeof (read_value), -10);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_write_fsc_schedules_to_HW(void) {
    int result, i, j, fd, index_table = 0;
    struct acmdrv_sched_tbl_row read_schedule;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_PARALLEL, SPEED_1GBps, MODULE_1, NULL);
    struct fsc_command fsc_schedule_mem[4] = {
            COMMAND_INITIALIZER(0, 0),
            COMMAND_INITIALIZER(0, 0),
            COMMAND_INITIALIZER(0, 0),
            COMMAND_INITIALIZER(0, 0)
    };
    struct schedule_entry schedule_window = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    /* prepare test case */
    //init module and fsc_schedule items
    module.cycle_ns = 5000;
    fsc_schedule_mem[0].hw_schedule_item.abs_cycle = 65600;
    fsc_schedule_mem[0].hw_schedule_item.cmd = 134414336;
    // 134414336 bin: 00001000 00000011 00000000 00000000
    fsc_schedule_mem[0].schedule_reference = &schedule_window;
    fsc_schedule_mem[1].hw_schedule_item.abs_cycle = 191300;
    fsc_schedule_mem[1].hw_schedule_item.cmd = 268632064;
    // 268632064 bin: 00010000 00000011 00000000 00000000
    fsc_schedule_mem[1].schedule_reference = &schedule_window;
    fsc_schedule_mem[2].hw_schedule_item.abs_cycle = 191600;
    fsc_schedule_mem[2].hw_schedule_item.cmd = 268632064;
    // 268632064 bin: 00010000 00000011 00000000 00000000
    fsc_schedule_mem[2].schedule_reference = &schedule_window;

    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    add_fsc_to_module_sorted(&module.fsc_list, &fsc_schedule_mem[0]);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.fsc_list));
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    add_fsc_to_module_sorted(&module.fsc_list, &fsc_schedule_mem[1]);
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&module.fsc_list));
    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);
    add_fsc_to_module_sorted(&module.fsc_list, &fsc_schedule_mem[2]);
    TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&module.fsc_list));
    //init schedule and stream
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    //schedule list of stream
    _ACMLIST_INSERT_TAIL(&stream.windows, &schedule_window, entry);
    schedule_window.period_ns = 5000;
    schedule_window.time_start_ns = 4700;
    schedule_window.time_end_ns = 4900;

    pthread_mutex_lock_ExpectAndReturn(&module.fsc_list.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.fsc_list.lock, 0);

    /* execute test case */
    result = write_fsc_schedules_to_HW(&module, index_table);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/sched_tab_row", O_RDONLY);
    /* module 0 table index 0 and 1: 1024 elements equal 0 each table */
    for (j = 0; j < 2; j++) {
        for (i = 0; i < ACM_MAX_SCHEDULE_EVENTS; i++) {
            result = pread(fd,
                    &read_schedule,
                    sizeof (read_schedule),
                    sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * j
                            + i * sizeof (read_schedule));
            TEST_ASSERT_GREATER_OR_EQUAL(0, result);
            TEST_ASSERT_EQUAL(0, read_schedule.cmd);
            TEST_ASSERT_EQUAL(0, read_schedule.delta_cycle);
            TEST_ASSERT_EQUAL(0, read_schedule.padding);
        }
    }
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2); // first schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, read_schedule.cmd);
    TEST_ASSERT_EQUAL(60000, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + sizeof (read_schedule)); // second schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, read_schedule.cmd);
    TEST_ASSERT_EQUAL(5600, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + 2 * sizeof (read_schedule)); // 3. schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0x08000000, read_schedule.cmd);
    TEST_ASSERT_EQUAL(60000, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + 3 * sizeof (read_schedule)); // 4. schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, read_schedule.cmd);
    TEST_ASSERT_EQUAL(60000, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + 4 * sizeof (read_schedule)); // 5. schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, read_schedule.cmd);
    TEST_ASSERT_EQUAL(5700, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + 5 * sizeof (read_schedule)); // 6. schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0x10000000, read_schedule.cmd);
    TEST_ASSERT_EQUAL(300, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    result = pread(fd,
            &read_schedule,
            sizeof (read_schedule),
            sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + 6 * sizeof (read_schedule)); // 7. schedule of module 1, tab 0
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0x10000000, read_schedule.cmd);
    TEST_ASSERT_EQUAL(8, read_schedule.delta_cycle);
    TEST_ASSERT_EQUAL(0, read_schedule.padding);
    for (i = 7; i < ACM_MAX_SCHEDULE_EVENTS; i++) {
        result = pread(fd,
                &read_schedule,
                sizeof (read_schedule),
                sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 2 + i * sizeof (read_schedule));
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
        if (result > 0) {
            // only check if there was read something
            TEST_ASSERT_EQUAL(0, read_schedule.cmd);
            TEST_ASSERT_EQUAL(0, read_schedule.delta_cycle);
            TEST_ASSERT_EQUAL(0, read_schedule.padding);
        }
    }

    /* check for table index 1 1024 Elemente equal 0 */
    for (i = 0; i < ACM_MAX_SCHEDULE_EVENTS; i++) {
        result = pread(fd,
                &read_schedule,
                sizeof (read_schedule),
                sizeof (read_schedule) * ACM_MAX_SCHEDULE_EVENTS * 3 + i * sizeof (read_schedule));
        TEST_ASSERT_GREATER_OR_EQUAL(0, result);
        if (result > 0) {
            // only check if there was read something
            TEST_ASSERT_EQUAL(0, read_schedule.cmd);
            TEST_ASSERT_EQUAL(0, read_schedule.delta_cycle);
            TEST_ASSERT_EQUAL(0, read_schedule.padding);
        }
    }
    close(fd);
}

void test_write_fsc_nop_command_neg_pwrite(void) {
    int result, tab_index = 0, item_index = 0, fd = 33;
    uint32_t delta_cycle = 60000;

    logging_Expect(0, "Sysfs: problem writing NOP schedule items");
    result = write_fsc_nop_command(fd, delta_cycle, MODULE_1, tab_index, item_index);
    TEST_ASSERT_EQUAL(-EBADF, result);
}

void test_sysfs_read_schedule_status(void) {
    int free_table_index, result, fd, i;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acmdrv_sched_tbl_status tbl_stat[4];
    for (i = 0; i < 4; i++) {
        memset(&tbl_stat[i], 0, sizeof (tbl_stat[i]));
    }

    /* prepare testcase: second table free for use module 0 */
    tbl_stat[0] = acmdrv_sched_tbl_status_create(true, true, true);
    tbl_stat[1] = acmdrv_sched_tbl_status_create(false, false, false);
    tbl_stat[2] = acmdrv_sched_tbl_status_create(true, true, true);
    tbl_stat[3] = acmdrv_sched_tbl_status_create(false, false, false);
    fd = open(ACMDEV_BASE "config_bin/table_status", O_WRONLY);
    pwrite(fd, (char*) &tbl_stat[0], sizeof (tbl_stat[0]), 0);
    pwrite(fd, (char*) &tbl_stat[1], sizeof (tbl_stat[1]), sizeof (tbl_stat[0]));
    close(fd);

    /* execute test case */
    result = sysfs_read_schedule_status(&module, &free_table_index);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, free_table_index);
}

void test_sysfs_read_schedule_status_neg_free_tab(void) {
    int free_table_index, result, fd, i;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acmdrv_sched_tbl_status tbl_stat[4];
    for (i = 0; i < 4; i++) {
        memset(&tbl_stat[i], 0, sizeof (tbl_stat[i]));
    }

    /* prepare testcase: no  table free for use */
    // module 0
    tbl_stat[0] = acmdrv_sched_tbl_status_create(true, true, true);
    tbl_stat[1] = acmdrv_sched_tbl_status_create(true, true, true);
    // module 1
    tbl_stat[2] = acmdrv_sched_tbl_status_create(true, true, true);
    tbl_stat[3] = acmdrv_sched_tbl_status_create(true, true, true);
    fd = open(ACMDEV_BASE "config_bin/table_status", O_WRONLY);
    pwrite(fd, (char*) &tbl_stat[0], sizeof (tbl_stat[0]), 0);
    pwrite(fd, (char*) &tbl_stat[1], sizeof (tbl_stat[1]), sizeof (tbl_stat[0]));
    close(fd);

    /* execute test case */
    logging_Expect(0, "Sysfs: no free schedule table found to apply schedule");
    result = sysfs_read_schedule_status(&module, &free_table_index);
    TEST_ASSERT_EQUAL(-EACMNOFREESCHEDTAB, result);
}

void test_write_gather_egress_ingress_triggered_stream_no_operation(void) {
    int ret;
    int start_index;
    uint32_t module_id;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    start_index = 0;
    module_id = 0;

    pthread_mutex_lock_ExpectAndReturn(&stream.operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream.operations.lock, 0);

    ret = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_write_gather_egress_ingress_triggered_stream(void) {
    int ret;
    int start_index;
    uint32_t module_id;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct sysfs_buffer msgbuf = BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_TX,
            16, false, true, "acmbuf_tx0");
    struct operation op_insert = INSERT_OPERATION_INITIALIZER(16, "acmbuf_tx0", &msgbuf);

    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert, entry);

    start_index = 0;
    module_id = 0;

    pthread_mutex_lock_ExpectAndReturn(&stream.operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream.operations.lock, 0);

    ret = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(0, ret);
}


void test_write_gather_egress_redundant_stream_tx(void) {
    int ret;
    int start_index;
    uint32_t module_id;
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_TX);
    struct sysfs_buffer msgbuf = BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_TX,
            16, false, true, "acmbuf_tx0");
    struct operation op_insert = INSERT_OPERATION_INITIALIZER(16, "acmbuf_tx0", &msgbuf);

    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert, entry);

    start_index = 0;
    module_id = 0;

    pthread_mutex_lock_ExpectAndReturn(&stream.operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream.operations.lock, 0);

    ret = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_write_gather_egress_redundant_stream_tx_illegal_operation(void) {
    int ret;
    int start_index;
    uint32_t module_id;
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_TX);
    struct sysfs_buffer msgbuf = BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_TX,
            16, false, true, "acmbuf_tx0");
    struct operation op_insert =
            OPERATION_INITIALIZER(57, 0, 16, "acmbuf_tx0", NULL, 0, &msgbuf, 0);

    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert, entry);

    start_index = 0;
    module_id = 0;

    pthread_mutex_lock_ExpectAndReturn(&stream.operations.lock, 0);
    logging_Expect(LOGLEVEL_ERR, NULL);
    logging_IgnoreArg_format();
    pthread_mutex_unlock_ExpectAndReturn(&stream.operations.lock, 0);
    ret = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, ret);
}

void test_sysfs_write_emergency_disable(void) {
    int result, fd;
    struct acmdrv_sched_emerg_disable emerg_disable_value, read_value;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    emerg_disable_value.eme_dis = 2;

    result = sysfs_write_emergency_disable(&module, &emerg_disable_value);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/emergency_disable", O_RDONLY);
    result = pread(fd, &read_value, sizeof (read_value), 0);
    close(fd);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_EQUAL_MEMORY(&emerg_disable_value, &read_value, sizeof(emerg_disable_value));
}

void test_sysfs_write_module_enable(void) {
    int result, fd;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    bool enable = true;
    uint32_t read_value;

    result = sysfs_write_module_enable(&module, enable);
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/cntl_ngn_enable", O_RDONLY);
    result = pread(fd, &read_value, sizeof (read_value), 0);
    close(fd);
    TEST_ASSERT_GREATER_OR_EQUAL(0, result);
    TEST_ASSERT_TRUE(read_value);
}

void test_sysfs_write_data_constant_buffer(void) {
    int result, fd, i;
    uint8_t read_const_buffer_tab_row[16];
    uint8_t tab_row_init[16] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                                0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    uint8_t const_buff_expect1[16] = {0x31, 0x31, 0x31, 0x32, 0x32, 0x20, 0x32, 0x32,
                                      0x33, 0x33, 0x33, 0x20, 0x33, 0x33, 0x33, 0x20};
    uint8_t const_buff_expect2[16] = {0x33, 0x33, 0x33, 0x0, 0x0, 0x0, 0x0, 0x0,
                                       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    char insert_const1[] = "111";
    char insert_const2[] = "22 22";
    char insert_const3[] = "333 333 333";
    char pad_const[] = "F";
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_1, NULL);
    struct acm_stream stream1 = STREAM_INITIALIZER(stream1, TIME_TRIGGERED_STREAM);
    struct acm_stream stream2 = STREAM_INITIALIZER(stream2, TIME_TRIGGERED_STREAM);
    struct operation operation1 = INSERT_CONSTANT_OPERATION_INITIALIZER(insert_const1, 3);
    struct operation operation2 = INSERT_CONSTANT_OPERATION_INITIALIZER(insert_const2, 5);
    struct operation operation3 = INSERT_CONSTANT_OPERATION_INITIALIZER(insert_const3, 11);
    struct operation operation4 = INSERT_OPERATION_INITIALIZER(20, "buffer_1", NULL);
    struct operation operation5 = PAD_OPERATION_INITIALIZER(7, pad_const);

    _ACMLIST_INSERT_HEAD(&module.streams, &stream1, entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &stream2, entry);
    _ACMLIST_INSERT_TAIL(&stream1.operations, &operation1, entry);
    _ACMLIST_INSERT_TAIL(&stream1.operations, &operation2, entry);
    _ACMLIST_INSERT_TAIL(&stream1.operations, &operation4, entry);
    _ACMLIST_INSERT_TAIL(&stream2.operations, &operation3, entry);
    _ACMLIST_INSERT_HEAD(&stream2.operations, &operation5, entry);

    pthread_mutex_lock_ExpectAndReturn(&module.streams.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream1.operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream1.operations.lock, 0);
    pthread_mutex_lock_ExpectAndReturn(&stream2.operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&stream2.operations.lock, 0);
    pthread_mutex_unlock_ExpectAndReturn(&module.streams.lock, 0);
    result = sysfs_write_data_constant_buffer(&module);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, operation1.const_buff_offset);
    TEST_ASSERT_EQUAL(3, operation2.const_buff_offset);
    TEST_ASSERT_EQUAL(8, operation3.const_buff_offset);
    fd = open(ACMDEV_BASE "config_bin/const_buffer", O_RDONLY);
    for (i = 0; i < 512; i++) {
        pread(fd,
                &read_const_buffer_tab_row,
                sizeof (read_const_buffer_tab_row),
                i * sizeof (read_const_buffer_tab_row));
        switch (i) {
            case 256:
                TEST_ASSERT_EQUAL_HEX8_ARRAY(const_buff_expect1, read_const_buffer_tab_row, 4);
                break;
            case 257:
                TEST_ASSERT_EQUAL_HEX8_ARRAY(const_buff_expect2, read_const_buffer_tab_row, 4);
                break;
            default:
                TEST_ASSERT_EQUAL_HEX8_ARRAY(tab_row_init, read_const_buffer_tab_row, 4);
        }
    }
    close(fd);
}

void test_write_clear_all_fpga(void) {
    int result, fd;
    int32_t read_value;

    result = write_clear_all_fpga();
    TEST_ASSERT_EQUAL(0, result);
    fd = open(ACMDEV_BASE "config_bin/clear_all_fpga", O_RDONLY);
    result = pread(fd, &read_value, sizeof(read_value), 0);
    TEST_ASSERT_EQUAL_UINT32(ACMDRV_CLEAR_ALL_PATTERN, read_value);
    close(fd);
}

void test_get_mac_address_neg_ioctl(void) {
    int result;
    uint8_t mac[6];

    result = get_mac_address("WRONGDEV", mac);
    TEST_ASSERT_EQUAL(-ENODEV, result);
}

void test_sysfs_write_lookup_tables(void) {
    int result;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module0 =
            MODULE_INITIALIZER(module0, CONN_MODE_PARALLEL, SPEED_1GBps, MODULE_0, &configuration);
    struct acm_module module1 =
            MODULE_INITIALIZER(module1, CONN_MODE_SERIAL, SPEED_1GBps, MODULE_1, &configuration);
    struct lookup lookup1 = {{0x01, 0x01, 0x01, 0x01, 0x01}, {0x01, 0x01, 0x01, 0x01, 0x01},
            {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 5};
    struct lookup lookup2 = {{0x11, 0x11, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x11, 0x11, 0x11},
             {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 4};
    struct lookup lookup3 = {{0x03, 0x03, 0x03, 0x03, 0x03}, {0x03, 0x03, 0x03, 0x03, 0x03},
             {0x33, 0x33, 0x33}, {0x33, 0x33, 0x33}, 3};
    struct lookup lookup4 = {{0x04, 0x04, 0x04, 0x04, 0x04}, {0x04, 0x04, 0x04, 0x04, 0x04},
             {0x44, 0x44}, {0x44, 0x44}, 2};
    struct acm_stream stream1 = STREAM_INITIALIZER_ALL(stream1, INGRESS_TRIGGERED_STREAM, &lookup1, NULL,
            NULL, NULL, 0, 0, 0, 0, 0);
    struct acm_stream stream2 = STREAM_INITIALIZER_ALL(stream2, INGRESS_TRIGGERED_STREAM, &lookup2, NULL,
            NULL, NULL, 0, 0, 0, 0, 1);
    struct acm_stream stream3 = STREAM_INITIALIZER_ALL(stream3, REDUNDANT_STREAM_RX, &lookup3, NULL,
            NULL, NULL, 0, 0, 0, 2, 2);
    struct acm_stream stream4 = STREAM_INITIALIZER_ALL(stream4, REDUNDANT_STREAM_RX, &lookup4, NULL,
            NULL, &stream3, 0, 0, 0, 2, 7);
    stream3.reference_redundant = &stream4;
    struct acm_stream stream5 = STREAM_INITIALIZER_ALL(stream2, EVENT_STREAM, NULL, NULL,
            NULL, NULL, 0, 0, 7, 0, 0);
    stream2.reference = &stream5;
    stream5.reference_parent = &stream2;
/*    STREAM_INITIALIZER_ALL(_stream, _type, _lookup, _reference,   \
        _reference_parent, _reference_redundant, _indiv_recov_timeout_ms, \
        _gather_dma_index, _scatter_dma_index, _redundand_index,          \
        _lookup_index) */
    _ACMLIST_INSERT_TAIL(&module0.streams, &stream1, entry);
    _ACMLIST_INSERT_TAIL(&module0.streams, &stream2, entry);
    _ACMLIST_INSERT_TAIL(&module0.streams, &stream3, entry);
    _ACMLIST_INSERT_TAIL(&module1.streams, &stream4, entry);
    _ACMLIST_INSERT_TAIL(&module1.streams, &stream5, entry);

    pthread_mutex_lock_ExpectAndReturn(&module0.streams.lock, 0);
    stream_has_operation_x_ExpectAndReturn(&stream1, READ, false);
    stream_has_operation_x_ExpectAndReturn(&stream2, READ, false);
    stream_has_operation_x_ExpectAndReturn(&stream5, INSERT, true);
    stream_has_operation_x_ExpectAndReturn(&stream3, READ, false);
    pthread_mutex_unlock_ExpectAndReturn(&module0.streams.lock, 0);
    result = sysfs_write_lookup_tables(&module0);
    TEST_ASSERT_EQUAL(0, result);

    pthread_mutex_lock_ExpectAndReturn(&module0.streams.lock, 0);
    stream_has_operation_x_ExpectAndReturn(&stream4, READ, false);
    pthread_mutex_unlock_ExpectAndReturn(&module0.streams.lock, 0);
    result = sysfs_write_lookup_tables(&module1);
    TEST_ASSERT_EQUAL(0, result);
}
