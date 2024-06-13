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

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "sysfs.h"

/* directly added modules */
#include "libc_helper.h" // <-- used to link in libc helper functions
#include "tracing.h"

/* mock modules needed */
#include "mock_libc.h" // <-- used to generate mocks for libc functions
#include "mock_logging.h"
#include "mock_memory.h"
#include "mock_stream.h"
#include "mock_status.h"

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

/* program mock for an error log call */
static void logging_Expect_loglevel_err(void) {
    logging_Expect(LOGLEVEL_ERR, NULL);
    logging_IgnoreArg_format();
}

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

/* program mock for a successful call to sysfs_construct_path_name */
static void sysfs_construct_path_name_Expect(char *filename) {
    libc_snprintf_Expect_result_retval(filename, 0);
}

/* program mock for an unsuccessful call to sysfs_construct_path_name */
static void sysfs_construct_path_name_Expect_failure(int err) {
    libc_snprintf_Expect_result_retval(NULL, -err);
    logging_Expect_loglevel_err();
}

void test_write_file_sysfs(void) {
    int ret;
    const char *filename = "JustTesting";
    uint8_t buffer[]  = { 0xAF, 0xFE, 0xDE, 0xAD };
    off_t offset = 42;
    int fd = 27;

    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, buffer, sizeof(buffer), offset, sizeof(buffer));
    libc_close_ExpectAndReturn(fd, 0);

    /* test write_file_sysfs */
    ret = write_file_sysfs(filename, buffer, sizeof(buffer), offset);
    TEST_ASSERT_EQUAL(0, ret); /* must succeed */
}

void test_write_file_sysfs_pwrite_fails(void) {
    int ret;
    const char *filename = "JustTesting";
    uint8_t buffer[]  = { 0xAF, 0xFE, 0xDE, 0xAD };
    off_t offset = 42;
    int fd = 27;
    int my_errno = EBUSY;

    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, buffer, sizeof(buffer), offset, -1);
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    /* test write_file_sysfs */
    ret = write_file_sysfs(filename, buffer, sizeof(buffer), offset);
    TEST_ASSERT_EQUAL(-EBUSY, ret); /* must succeed */
}

void test_write_file_sysfs_pwrite_less_data(void) {
    int ret;
    const char *filename = "JustTesting";
    uint8_t buffer[]  = { 0xAF, 0xFE, 0xDE, 0xAD };
    off_t offset = 42;
    int fd = 27;

    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, buffer, sizeof(buffer), offset, sizeof(buffer) - 1);
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect_loglevel_err();

    /* test write_file_sysfs */
    ret = write_file_sysfs(filename, buffer, sizeof(buffer), offset);
    TEST_ASSERT_EQUAL(-EIO, ret); /* must succeed */
}

void test_write_file_sysfs_open_fails(void) {
    int ret;
    const char *filename = "JustTesting";
    uint8_t buffer[]  = { 0xAF, 0xFE, 0xDE, 0xAD };
    off_t offset = 42;
    int my_errno = ENODEV;

    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    /* test write_file_sysfs */
    ret = write_file_sysfs(filename, buffer, sizeof(buffer), offset);
    TEST_ASSERT_EQUAL(-ENODEV, ret);
}

void test_get_mac_address(void) {
    int ret;
    char *ifname = "eth0";
    uint8_t mac[ETHER_ADDR_LEN];
    int fd = 12;

    libc_socket_ExpectAndReturn(AF_INET, SOCK_DGRAM, 0, fd);
    libc_ioctl_ExpectAndReturn(fd, SIOCGIFHWADDR, 0);
    libc_close_ExpectAndReturn(fd, 0);
    ret = get_mac_address(ifname, mac);

    TEST_ASSERT_EQUAL(0, ret);
}

void test_get_mac_address_neg_fd(void) {
    int ret;
    char *ifname = "eth0";
    uint8_t mac[ETHER_ADDR_LEN];
    int fd = -1;
    int my_errno = EACCES;

    libc_socket_ExpectAndReturn(AF_INET, SOCK_DGRAM, 0, fd);
    libc___errno_location_ExpectAndReturn(&my_errno);
    ret = get_mac_address(ifname, mac);
    TEST_ASSERT_EQUAL(-EACCES, ret);
}

void test_read_uint64_sysfs_item_data_out_of_range(void) {
    int64_t ret;
    const char *filename = "JustTesting";
    int fd = 58;
    char *data = "18446744073709551616"; // ULLONG_MAX + 1
    ssize_t data_len = strlen(data) + 1;

    LIBC_UNMOCK(__errno_location); // <-- do not mock errno here
    libc_open_ExpectAndReturn(filename, O_RDONLY | O_DSYNC, fd);
    libc_read_ExpectAndReturn(fd, NULL, 0, data_len);
    libc_read_IgnoreArg_buf();
    libc_read_IgnoreArg_nbytes();
    libc_read_ReturnMemThruPtr_buf(data, data_len);
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect_loglevel_err();

    ret = read_uint64_sysfs_item(filename);
    TEST_ASSERT_EQUAL(-ERANGE, ret);
}

static void testhelper_read_uint64_sysfs_item_read_data(char *data) {
    int64_t ret;
    const char *filename = "JustTesting";
    int fd = 58;
    char *end;
    unsigned long long value = FUNC(LIBC_STRTOULL)(data, &end, 0);
    ssize_t data_len = strlen(data) + 1;

    LIBC_UNMOCK(__errno_location); // <-- do not mock errno here
    libc_open_ExpectAndReturn(filename, O_RDONLY | O_DSYNC, fd);
    libc_read_ExpectAndReturn(fd, NULL, 0, data_len);
    libc_read_IgnoreArg_buf();
    libc_read_IgnoreArg_nbytes();
    libc_read_ReturnMemThruPtr_buf(data, data_len);
    libc_close_ExpectAndReturn(fd, 0);

    ret = read_uint64_sysfs_item(filename);
    TEST_ASSERT_EQUAL_UINT64(value, ret);
}

void test_read_uint64_sysfs_item_read_data_0(void) {
    testhelper_read_uint64_sysfs_item_read_data("0");
}

void test_read_uint64_sysfs_item_read_data_17(void) {
    testhelper_read_uint64_sysfs_item_read_data("17");
}

void test_read_uint64_sysfs_item_read_data_0x1315(void) {
    testhelper_read_uint64_sysfs_item_read_data("0x1315");
}

void test_read_uint64_sysfs_item_read_data_0xaffedead(void) {
    testhelper_read_uint64_sysfs_item_read_data("0xaffedead");
}

void test_read_uint64_sysfs_item_read_data_0x0badcafeaffedead(void) {
    testhelper_read_uint64_sysfs_item_read_data("0x0badcafeaffedead");
}

void test_read_uint64_sysfs_item_invalid_data(void) {
    int64_t ret;
    const char *filename = "JustTesting";
    int fd = 58;
    char *data = "NoNumber";
    ssize_t data_len = strlen(data) + 1;

    LIBC_UNMOCK(__errno_location); // <-- do not mock errno here
    libc_open_ExpectAndReturn(filename, O_RDONLY | O_DSYNC, fd);
    libc_read_ExpectAndReturn(fd, NULL, 0, data_len);
    libc_read_IgnoreArg_buf();
    libc_read_IgnoreArg_nbytes();
    libc_read_ReturnMemThruPtr_buf(data, data_len);
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect_loglevel_err();

    ret = read_uint64_sysfs_item(filename);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_write_module_schedules_to_HW_construct_path_name_sched_cycle_time_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);

    sysfs_construct_path_name_Expect_failure(EINVAL);

    ret = write_module_schedules_to_HW(&module, 0);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_write_module_schedules_to_HW_open_sched_cycle_time_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    char *filename = ACMDEV_BASE "config_bin/sched_cycle_time";
    int my_errno = EIO;

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    ret = write_module_schedules_to_HW(&module, 0);
    TEST_ASSERT_EQUAL(-EIO, ret);
}

void test_write_module_schedules_to_HW_pwrite_sched_cycle_time_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    int table_index = 5;
    char *filename = ACMDEV_BASE "config_bin/sched_cycle_time";
    int my_errno = EIO;
    int fd = 42;

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_cycle_time),
            (module.module_id * ACMDRV_SCHED_TBL_COUNT + table_index) *
                sizeof(struct acmdrv_sched_cycle_time),
            -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);
    libc_close_ExpectAndReturn(fd, 0);

    ret = write_module_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, ret);
}

void test_write_module_schedules_to_HW_construct_path_name_sched_start_table_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    int table_index = 5;
    char *sched_cycle_time = ACMDEV_BASE "config_bin/sched_cycle_time";
    int fd = 42;

    sysfs_construct_path_name_Expect(sched_cycle_time);
    libc_open_ExpectAndReturn(sched_cycle_time, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_cycle_time),
            (module.module_id * ACMDRV_SCHED_TBL_COUNT + table_index) *
                sizeof(struct acmdrv_sched_cycle_time),
            sizeof(struct acmdrv_sched_cycle_time));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(EINVAL);

    ret = write_module_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_write_module_schedules_to_HW_open_sched_start_table_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    int table_index = 5;
    char *sched_cycle_time = ACMDEV_BASE "config_bin/sched_cycle_time";
    char *sched_start_table = ACMDEV_BASE "config_bin/sched_start_table";
    int fd = 42;
    int my_errno = EIO;

    sysfs_construct_path_name_Expect(sched_cycle_time);
    libc_open_ExpectAndReturn(sched_cycle_time, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_cycle_time),
            (module.module_id * ACMDRV_SCHED_TBL_COUNT + table_index) *
                sizeof(struct acmdrv_sched_cycle_time),
            sizeof(struct acmdrv_sched_cycle_time));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(sched_start_table);
    libc_open_ExpectAndReturn(sched_start_table, O_WRONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    ret = write_module_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, ret);
}

void test_write_module_schedules_to_HW_pwrite_sched_start_table_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    int table_index = 5;
    char *sched_cycle_time = ACMDEV_BASE "config_bin/sched_cycle_time";
    char *sched_start_table = ACMDEV_BASE "config_bin/sched_start_table";
    int fd = 42;
    int my_errno = EIO;

    sysfs_construct_path_name_Expect(sched_cycle_time);
    libc_open_ExpectAndReturn(sched_cycle_time, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_cycle_time),
            (module.module_id * ACMDRV_SCHED_TBL_COUNT + table_index) *
                sizeof(struct acmdrv_sched_cycle_time),
            sizeof(struct acmdrv_sched_cycle_time));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(sched_start_table);
    libc_open_ExpectAndReturn(sched_start_table, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_timespec64),
            (module.module_id * ACMDRV_SCHED_TBL_COUNT + table_index) *
                sizeof(struct acmdrv_timespec64),
            -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);
    libc_close_ExpectAndReturn(fd, 0);

    ret = write_module_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, ret);
}

void test_sysfs_read_schedule_status_construct_path_name_table_status_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    int free_table;

    sysfs_construct_path_name_Expect_failure(EINVAL);

    ret = sysfs_read_schedule_status(&module, &free_table);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_sysfs_read_schedule_status_open_table_status_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    char *filename = ACMDEV_BASE "config_bin/table_status";
    int free_table;
    int my_errno = EIO;

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_RDONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    ret = sysfs_read_schedule_status(&module, &free_table);
    TEST_ASSERT_EQUAL(-EIO, ret);
}

void test_sysfs_read_schedule_status_pread_table_status_fails(void) {
    int ret;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            NULL);
    char *filename = ACMDEV_BASE "config_bin/table_status";
    int free_table;
    int fd = 42;
    int my_errno = EIO;

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_RDONLY | O_DSYNC, fd);
    libc_pread_ExpectAndReturn(fd,
            NULL,
            ACMDRV_SCHED_TBL_COUNT * sizeof(struct acmdrv_sched_tbl_status),
            module.module_id * sizeof(struct acmdrv_sched_tbl_status),
            -1);
    libc_pread_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    ret = sysfs_read_schedule_status(&module, &free_table);
    TEST_ASSERT_EQUAL(-EIO, ret);
}

// sysfs_write_delay_file() has been removed
#if 0

void test_sysfs_write_delay_file_asprintf_filename_failure(void) {
    int ret;
    unsigned int delay = 457;
    char *speed = "1000";
    char *dir = "tx";

    libc_asprintf_ExpectAndReturn(NULL, DELAY_BASE "%s/phy/delay%s%s", -1);
    libc_asprintf_IgnoreArg_ptr();

    ret = sysfs_write_delay_file(MODULE_0, delay, speed, dir);
    TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_sysfs_write_delay_file_asprintf_value_failure(void) {
    int ret;
    unsigned int delay = 457;
    char *speed = "1000";
    char *dir = "tx";
    char *filename;
    int count;

    count = FUNC(LIBC_ASPRINTF)(&filename, DELAY_BASE "sw0p2/phy/delay%s%s", speed, dir);

    libc_asprintf_ExpectAndReturn(NULL, DELAY_BASE "%s/phy/delay%s%s", count);
    libc_asprintf_IgnoreArg_ptr();
    libc_asprintf_ReturnThruPtr_ptr(&filename);
    libc_asprintf_ExpectAndReturn(NULL, "%d", -1);
    libc_asprintf_IgnoreArg_ptr();

    ret = sysfs_write_delay_file(MODULE_0, delay, speed, dir);
    TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

#endif

void test_sysfs_get_recovery_timeout(void) {
    int result, fd = 7;
    char buffer[] = "RECOVERY_TIMEOUT_MS   4294968295             ";

    libc_open_ExpectAndReturn(CONFIG_FILE, O_RDONLY | O_DSYNC, fd);

    libc_read_ExpectAndReturn(fd, NULL, 100, 20);
    libc_read_IgnoreArg_buf();
    libc_read_ReturnMemThruPtr_buf(buffer, strlen(buffer)+1);
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect(LOGLEVEL_ERR, "Module: unable to convert value %s of configuration item KEY_RECOVERY_TIMEOUT");

    result = sysfs_get_recovery_timeout();
    TEST_ASSERT_EQUAL(DEFAULT_REC_TIMEOUT_MS, result);
}

void test_sysfs_write_buffer_control_mask_neg_construct_path(void) {
    int result;
    uint64_t vector = 2779096485; // 0xA5A5A5A5

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);
    sysfs_construct_path_name_Expect_failure(EINVAL);

    result = sysfs_write_buffer_control_mask(vector, __stringify(ACM_SYSFS_LOCK_BUFFMASK));
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_sysfs_write_msg_buff_to_HW_neg_BUFF_DESC_path(void){
    int result;
    struct buffer_list bufferlist;

    //prepare test
    //init bufferlist
    ACMLIST_INIT(&bufferlist);

    //execute test
    sysfs_construct_path_name_Expect_failure(EINVAL);
    result = sysfs_write_msg_buff_to_HW(&bufferlist, BUFF_DESC);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_sysfs_write_msg_buff_to_HW_neg_BUFF_DESC_open(void){
    int result;
    struct buffer_list bufferlist;
    char *filename = ACMDEV_BASE "config_bin/msg_buff_desc";
    int my_errno = EIO;

    //prepare test
    //init bufferlist
    ACMLIST_INIT(&bufferlist);

    //execute test
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);

    result = sysfs_write_msg_buff_to_HW(&bufferlist, BUFF_DESC);
    TEST_ASSERT_EQUAL(-EIO, result);
}

void test_sysfs_write_msg_buff_to_HW_neg_BUFF_DESC_write(void){
    int result, fd = 7;
    struct buffer_list bufferlist;
    struct sysfs_buffer msg_buf1;
    memset(&msg_buf1, 0, sizeof(msg_buf1));
    char buff_name[] = "erster_Name";
    char *filename = ACMDEV_BASE "config_bin/msg_buff_desc";
    int my_errno = EBUSY;
    uint32_t *buffer;

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
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, buffer, sizeof(uint32_t), 0, -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);
    libc_close_ExpectAndReturn(fd, 0);

    result = sysfs_write_msg_buff_to_HW(&bufferlist, BUFF_DESC);
    TEST_ASSERT_EQUAL(-EBUSY, result);
}

void test_check_buff_name_against_sys_devices(void) {
    int result, fd = 7;
    char buffername[] = "name1";

    libc_open_ExpectAndReturn(CONFIG_FILE, O_RDONLY | O_DSYNC, fd);

    libc_read_ExpectAndReturn(fd, NULL, 100, 0);
    libc_read_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_INFO, "Sysfs: configuration item not found %s");
    libc_close_ExpectAndReturn(fd, 0);
    logging_Expect(LOGLEVEL_ERR, "Sysfs: message buffer name %s doesn't start with configured/default praefix %s");
    result = check_buff_name_against_sys_devices(buffername);
    TEST_ASSERT_EQUAL(-EPERM, result);
}

void test_sysfs_write_lookup_control_block_neg_write_ingress_policing_control(void) {
    int result;
    uint32_t module_id = 0;
    uint16_t ingress_control = 3;
    uint16_t lookup_enable = 3;
    uint16_t layer7_enable = 3;
    uint8_t layer7_len = 7;

    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_control_block(module_id,
            ingress_control,
            lookup_enable,
            layer7_enable,
            layer7_len);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_sysfs_write_lookup_control_block_neg_write_ingress_policing_enable(void) {
    int result, fd = 9;
    uint32_t module_id = 0;
    uint16_t ingress_control = 3;
    uint16_t lookup_enable = 3;
    uint16_t layer7_enable = 3;
    uint8_t layer7_len = 7;
    char *filename = ACMDEV_BASE "config_bin/cntl_ingress_policing_control";

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_control_block(module_id,
            ingress_control,
            lookup_enable,
            layer7_enable,
            layer7_len);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_sysfs_write_lookup_control_block_neg_write_layer7_enable(void) {
    int result, fd = 9;
    uint32_t module_id = 0;
    uint16_t ingress_control = 3;
    uint16_t lookup_enable = 3;
    uint16_t layer7_enable = 3;
    uint8_t layer7_len = 7;
    char *filename1 = ACMDEV_BASE "config_bin/cntl_ingress_policing_control";
    char *filename2 = ACMDEV_BASE "config_bin/cntl_ingress_policing_enable";

    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);

    result = sysfs_write_lookup_control_block(module_id,
            ingress_control,
            lookup_enable,
            layer7_enable,
            layer7_len);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_sysfs_write_lookup_control_block_neg_write_layer7_length(void) {
    int result, fd = 9;
    uint32_t module_id = 0;
    uint16_t ingress_control = 3;
    uint16_t lookup_enable = 3;
    uint16_t layer7_enable = 3;
    uint8_t layer7_len = 7;
    char *filename1 = ACMDEV_BASE "config_bin/cntl_ingress_policing_control";
    char *filename2 = ACMDEV_BASE "config_bin/cntl_ingress_policing_enable";
    char *filename3 = ACMDEV_BASE "config_bin/cntl_layer7_enable";

    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename3);
    libc_open_ExpectAndReturn(filename3, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);

    result = sysfs_write_lookup_control_block(module_id,
            ingress_control,
            lookup_enable,
            layer7_enable,
            layer7_len);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_sysfs_write_lookup_control_block_neg_write_lookup_enable(void) {
    int result, fd = 9;
    uint32_t module_id = 0;
    uint16_t ingress_control = 3;
    uint16_t lookup_enable = 3;
    uint16_t layer7_enable = 3;
    uint8_t layer7_len = 7;
    char *filename1 = ACMDEV_BASE "config_bin/cntl_ingress_policing_control";
    char *filename2 = ACMDEV_BASE "config_bin/cntl_ingress_policing_enable";
    char *filename3 = ACMDEV_BASE "config_bin/cntl_layer7_enable";
    char *filename4 = ACMDEV_BASE "config_bin/cntl_layer7_length";

    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename3);
    libc_open_ExpectAndReturn(filename3, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename4);
    libc_open_ExpectAndReturn(filename4, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);

    result = sysfs_write_lookup_control_block(module_id,
            ingress_control,
            lookup_enable,
            layer7_enable,
            layer7_len);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_sysfs_write_redund_ctrl_table_neg_write_nop(void) {
    int result;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_redund_ctrl_table(&module);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_sysfs_write_redund_ctrl_table_neg_write_redundand_control(void) {
    int result, fd = 5;
    char *filename = ACMDEV_BASE "config_bin/cntl_ingress_policing_control";
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);

    /* prepare test case */
    stream.redundand_index = REDUNDANCY_START_IDX;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);

    result = sysfs_write_redund_ctrl_table(&module);
    TEST_ASSERT_NOT_EQUAL(0, result);
}

void test_write_gather_egress_neg_write_prefetch(void) {
    int result;
    int start_index = 0;
    uint32_t module_id = 1;
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_TX);
    struct sysfs_buffer msgbuf = BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_TX,
            16, false, true, "acmbuf_tx0");
    struct operation op_insert = INSERT_OPERATION_INITIALIZER(16, "acmbuf_tx0", &msgbuf);

    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_gather_egress_neg_write_gather(void) {
    int result, fd = 8;
    int start_index = 0;
    uint32_t module_id = 0;
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_TX);
    struct sysfs_buffer msgbuf = BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_TX,
            16, false, true, "acmbuf_tx0");
    struct operation op_insert = INSERT_OPERATION_INITIALIZER(16, "acmbuf_tx0", &msgbuf);
    char *filename = ACMDEV_BASE "config_bin/prefetch_dma";

    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write prefetch and gather operation of insert
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);

    result = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}
void test_write_gather_egress_neg_write_rtag_command(void) {
    int result, fd = 8;
    int start_index = 0;
    uint32_t module_id = 0;
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_TX);
    struct sysfs_buffer msgbuf = BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_TX,
            16, false, true, "acmbuf_tx0");
    struct operation op_insert1 = INSERT_OPERATION_INITIALIZER(16, "acmbuf_tx0", &msgbuf);
    struct operation op_insert2 = INSERT_OPERATION_INITIALIZER(16, "acmbuf_tx0", &msgbuf);
    struct operation op_forward = FORWARD_OPERATION_INITIALIZER(5, 20);
    char *filename1 = ACMDEV_BASE "config_bin/prefetch_dma";
    char *filename2 = ACMDEV_BASE "config_bin/gather_dma";

    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert1, entry);
    _ACMLIST_INSERT_TAIL(&stream.operations, &op_insert2, entry);
    _ACMLIST_INSERT_TAIL(&stream.operations, &op_forward, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write prefetch and gather operation of first insert
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write prefetch and gather operation of second insert
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 8, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write gather operation of forward
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 8, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write rtag gather operation
    sysfs_construct_path_name_Expect_failure(ENOMEM);

    result = write_gather_egress(start_index, module_id, &stream);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_prefetcher_gather_dma_neg_nop_gather(void){
    int result;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_prefetcher_gather_dma_neg_nop_prefetcher(void){
    int result, fd = 6;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename2 = ACMDEV_BASE "config_bin/gather_dma";

    /* execute test case */
    // write NOP gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_prefetcher_gather_dma_neg_fwdall_gather(void){
    int result, fd = 6;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename1 = ACMDEV_BASE "config_bin/prefetch_dma";
    char *filename2 = ACMDEV_BASE "config_bin/gather_dma";

    /* execute test case */
    // write NOP gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write forwardall gather table
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_prefetcher_gather_dma_neg_prefetch_to_fwdall(void){
    int result, fd = 6;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename1 = ACMDEV_BASE "config_bin/prefetch_dma";
    char *filename2 = ACMDEV_BASE "config_bin/gather_dma";

    /* execute test case */
    // write NOP gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write forwardall gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table in parallel to forward all on gather table
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_prefetcher_gather_dma_neg_gather_ingress(void){
    int result, fd = 6;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename1 = ACMDEV_BASE "config_bin/prefetch_dma";
    char *filename2 = ACMDEV_BASE "config_bin/gather_dma";
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct operation op_forwardall = FORWARD_ALL_OPERATION_INITIALIZER;

    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    _ACMLIST_INSERT_TAIL(&stream.operations, &op_forwardall, entry);

    /* execute test case */
    // write NOP gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write forwardall gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table in parallel to forward all on gather table
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write gather command for ingress triggered stream
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    logging_Expect(LOGLEVEL_ERR, "Failed to write gather ingress");
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_prefetcher_gather_dma_neg_gather_egress(void){
    int result, fd = 6;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename1 = ACMDEV_BASE "config_bin/prefetch_dma";
    char *filename2 = ACMDEV_BASE "config_bin/gather_dma";
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation op_forwardall = FORWARD_ALL_OPERATION_INITIALIZER;

    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    _ACMLIST_INSERT_TAIL(&stream.operations, &op_forwardall, entry);

    /* execute test case */
    // write NOP gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write forwardall gather table
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write NOP prefetcher table in parallel to forward all on gather table
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 4, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    // write gather command for ingress triggered stream
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    logging_Expect(LOGLEVEL_ERR, "Failed to write gather engress");
    result = sysfs_write_prefetcher_gather_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_scatter_dma_neg_write_nop_scatter(void) {
    int result;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    /* execute test case */
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_scatter_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_scatter_dma_neg_write_scatter(void) {
    int result, fd = 6;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename = ACMDEV_BASE "config_bin/scatter_dma";
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);
    char buffername[] = "read_buffer";
    struct sysfs_buffer msg_buf = BUFFER_INITIALIZER( 3, 60, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX,
            10, true, true, buffername);
    struct operation op_read = READ_OPERATION_INITIALIZER(20, 40, buffername);

    op_read.msg_buf = &msg_buf;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    _ACMLIST_INSERT_TAIL(&stream.operations, &op_read, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_scatter_dma(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_layer7_mask(void) {
    int result;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);
    struct lookup lookup = {{0x11, 0x11, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x11, 0x11, 0x11},
            {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 10};

    stream.lookup = &lookup;
    stream.lookup_index = 7;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_layer7_pattern(void) {
    int result, fd = 6;
    char *filename = ACMDEV_BASE "config_bin/layer7_mask";
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);
    struct lookup lookup = {{0x11, 0x11, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x11, 0x11, 0x11},
            {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 10};

    stream.lookup = &lookup;
    stream.lookup_index = 7;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    //write layer7 mask
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write layer7 pattern
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_lookup_header_mask(void) {
    int result, fd = 6;
    char *filename1 = ACMDEV_BASE "config_bin/layer7_mask";
    char *filename2 = ACMDEV_BASE "config_bin/layer7_pattern";
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);
    struct lookup lookup = {{0x11, 0x11, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x11, 0x11, 0x11},
            {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 10};

    stream.lookup = &lookup;
    stream.lookup_index = 7;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    //write layer7 mask
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write layer7 pattern
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write lookup header mask
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_lookup_header_pattern(void) {
    int result, fd = 6;
    char *filename1 = ACMDEV_BASE "config_bin/layer7_mask";
    char *filename2 = ACMDEV_BASE "config_bin/layer7_pattern";
    char *filename3 = ACMDEV_BASE "config_bin/lookup_mask";
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);
    struct lookup lookup = {{0x11, 0x11, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x11, 0x11, 0x11},
            {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 10};

    stream.lookup = &lookup;
    stream.lookup_index = 7;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    //write layer7 mask
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write layer7 pattern
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write lookup header mask
    sysfs_construct_path_name_Expect(filename3);
    libc_open_ExpectAndReturn(filename3, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_lookup),
            sizeof(struct acmdrv_bypass_lookup) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_lookup));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write lookup header pattern
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_stream_trigger(void) {
    int result, fd = 6;
    char *filename1 = ACMDEV_BASE "config_bin/layer7_mask";
    char *filename2 = ACMDEV_BASE "config_bin/layer7_pattern";
    char *filename3 = ACMDEV_BASE "config_bin/lookup_mask";
    char *filename4 = ACMDEV_BASE "config_bin/lookup_pattern";
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);
    struct lookup lookup = {{0x11, 0x11, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x11, 0x11, 0x11},
            {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, {0xAA, 0xAA, 0xAA, 0xAA, 0xAA}, 10};

    stream.lookup = &lookup;
    stream.lookup_index = 7;
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    //write layer7 mask
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write layer7 pattern
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_layer7_check),
            sizeof(struct acmdrv_bypass_layer7_check) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_layer7_check));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write lookup header mask
    sysfs_construct_path_name_Expect(filename3);
    libc_open_ExpectAndReturn(filename3, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_lookup),
            sizeof(struct acmdrv_bypass_lookup) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_lookup));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write lookup header pattern
    sysfs_construct_path_name_Expect(filename4);
    libc_open_ExpectAndReturn(filename4, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_bypass_lookup),
            sizeof(struct acmdrv_bypass_lookup) * stream.lookup_index,
            sizeof(struct acmdrv_bypass_lookup));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    //write stream trigger
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_lookup_control_table(void) {
    int result;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    /* execute test case */
    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_write_lookup_tables_neg_stream_trigger_general(void) {
    int result, fd = 7;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char *filename1 = ACMDEV_BASE "config_bin/cntl_ingress_policing_control";
    char *filename2 = ACMDEV_BASE "config_bin/cntl_ingress_policing_enable";
    char *filename3 = ACMDEV_BASE "config_bin/cntl_layer7_enable";
    char *filename4 = ACMDEV_BASE "config_bin/cntl_layer7_length";
    char *filename5 = ACMDEV_BASE "config_bin/cntl_lookup_enable";

    /* execute test case */
    sysfs_construct_path_name_Expect(filename1);
    libc_open_ExpectAndReturn(filename1, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename2);
    libc_open_ExpectAndReturn(filename2, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename3);
    libc_open_ExpectAndReturn(filename3, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename4);
    libc_open_ExpectAndReturn(filename4, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);
    sysfs_construct_path_name_Expect(filename5);
    libc_open_ExpectAndReturn(filename5, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(uint32_t), 0, sizeof(uint32_t));
    libc_pwrite_IgnoreArg_buf();
    libc_close_ExpectAndReturn(fd, 0);

    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_write_lookup_tables(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_read_configuration_id_neg_path(void) {
    int result;

    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = sysfs_read_configuration_id();
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_sysfs_read_configuration_id_neg_read(void) {
    int result;
    char *filename = ACMDEV_BASE "config_bin/configuration_id";
    int my_errno = ENOENT;

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_RDONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);
    result = sysfs_read_configuration_id();
    TEST_ASSERT_EQUAL(-ENOENT, result);
}

void test_write_fsc_schedules_to_HW_neg_path(void) {
    int result;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;

    sysfs_construct_path_name_Expect_failure(ENOMEM);
    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_fsc_schedules_to_HW_neg_open(void) {
    int result;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    int my_errno = ENOENT;

    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, -1);
    logging_Expect_loglevel_err();
    libc___errno_location_ExpectAndReturn(&my_errno);
    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-ENOENT, result);
}

void test_write_fsc_schedules_to_HW_neg_write_first_nop(void) {
    int result, fd = 7;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    int my_errno = EIO;
    struct fsc_command fsc_schedule_mem = COMMAND_INITIALIZER(0, 0);

    module.cycle_ns = 5000;
    fsc_schedule_mem.hw_schedule_item.abs_cycle = 65600;
    fsc_schedule_mem.hw_schedule_item.cmd = 134414336;
    // 134414336 bin: 00001000 00000011 00000000 00000000
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd, NULL, sizeof(struct acmdrv_sched_tbl_row), 0, -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing NOP schedule items");
    libc___errno_location_ExpectAndReturn(&my_errno);
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing to %s ");
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, result);
}

void test_write_fsc_schedules_to_HW_neg_write_second_nop(void) {
    int result, fd = 7;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    int my_errno = EIO;
    struct fsc_command fsc_schedule_mem = COMMAND_INITIALIZER(0, 0);

    module.cycle_ns = 5000;
    fsc_schedule_mem.hw_schedule_item.abs_cycle = 65600;
    fsc_schedule_mem.hw_schedule_item.cmd = 134414336;
    // 134414336 bin: 00001000 00000011 00000000 00000000
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            0,
            0);
    libc_pwrite_IgnoreArg_buf();
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row),
            -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing NOP schedule items");
    libc___errno_location_ExpectAndReturn(&my_errno);
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing to %s ");
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, result);
}

void test_write_fsc_schedules_to_HW_neg_last_update_indexes(void) {
    int result, fd = 7;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    struct fsc_command fsc_schedule_mem = COMMAND_INITIALIZER(0, 0);

    module.cycle_ns = 5000;
    fsc_schedule_mem.hw_schedule_item.abs_cycle = 65600;
    fsc_schedule_mem.hw_schedule_item.cmd = 134414336;
    // 134414336 bin: 00001000 00000011 00000000 00000000
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            0,
            0);
    libc_pwrite_IgnoreArg_buf();
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row),
            0);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: fsc_schedule without reference to schedule item");
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem updating indexes of fsc schedule item ");
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_write_fsc_schedules_to_HW_neg_last_write_fsc_command(void) {
    int result, fd = 7;
    int my_errno = EIO;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    struct fsc_command fsc_schedule_mem = COMMAND_INITIALIZER(0, 0);
    struct schedule_entry schedule_window = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    module.cycle_ns = 5000;
    fsc_schedule_mem.hw_schedule_item.abs_cycle = 65600;
    fsc_schedule_mem.hw_schedule_item.cmd = 134414336;
    // 134414336 bin: 00001000 00000011 00000000 00000000
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem, entry);
    //init schedule and stream
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    //schedule list of stream
    _ACMLIST_INSERT_TAIL(&stream.windows, &schedule_window, entry);
    schedule_window.period_ns = 5000;
    schedule_window.time_start_ns = 4700;
    schedule_window.time_end_ns = 4900;
    fsc_schedule_mem.schedule_reference = &schedule_window;

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            0,
            0);
    libc_pwrite_IgnoreArg_buf();
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row),
            0);
    libc_pwrite_IgnoreArg_buf();
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row) * 2,
            -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing to %s ");
    libc___errno_location_ExpectAndReturn(&my_errno);
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, result);
}

void test_write_fsc_schedules_to_HW_neg_update_indexes_not_last(void) {
    int result, fd = 7;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(134414336, 65600);
    // 134414336 bin: 00001000 00000011 00000000 00000000
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(268632064, 191300);
    // 268632064 bin: 00010000 00000011 00000000 00000000

    module.cycle_ns = 5000;
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem1, entry);
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem2, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            0,
            0);
    libc_pwrite_IgnoreArg_buf();
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row),
            0);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: fsc_schedule without reference to schedule item");
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem updating indexes of fsc schedule item ");
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_write_fsc_schedules_to_HW_neg_write_not_last_fsc_item(void) {
    int result, fd = 7;
    int my_errno = EIO;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(134414336, 35600);
    // 134414336 bin: 00001000 00000011 00000000 00000000
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(268632064, 191300);
    // 268632064 bin: 00010000 00000011 00000000 00000000
    struct schedule_entry schedule_window[2] = { SCHEDULE_ENTRY_INITIALIZER,
            SCHEDULE_ENTRY_INITIALIZER };
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    fsc_schedule_mem1.schedule_reference = &schedule_window[0];
    fsc_schedule_mem2.schedule_reference = &schedule_window[1];
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    _ACMLIST_INSERT_TAIL(&stream.windows, &schedule_window[0], entry);
    _ACMLIST_INSERT_TAIL(&stream.windows, &schedule_window[1], entry);

    module.cycle_ns = 5000;
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem1, entry);
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem2, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            0,
            0);
    libc_pwrite_IgnoreArg_buf();
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row),
            -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing to %s ");
    libc___errno_location_ExpectAndReturn(&my_errno);
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, result);
}

void test_write_fsc_schedules_to_HW_neg_write_nop_for_not_first(void) {
    int result, fd = 7;
    int my_errno = EIO;
    struct acm_module module=
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    int table_index = 0;
    char *filename = ACMDEV_BASE "config_bin/sched_tab_row";
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(134414336, 35600);
    // 134414336 bin: 00001000 00000011 00000000 00000000
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(268632064, 191300);
    // 268632064 bin: 00010000 00000011 00000000 00000000
    struct schedule_entry schedule_window[2] = { SCHEDULE_ENTRY_INITIALIZER,
            SCHEDULE_ENTRY_INITIALIZER };
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    fsc_schedule_mem1.schedule_reference = &schedule_window[0];
    fsc_schedule_mem2.schedule_reference = &schedule_window[1];
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    _ACMLIST_INSERT_TAIL(&stream.windows, &schedule_window[0], entry);
    _ACMLIST_INSERT_TAIL(&stream.windows, &schedule_window[1], entry);

    module.cycle_ns = 5000;
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem1, entry);
    _ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem2, entry);

    /* execute test case */
    sysfs_construct_path_name_Expect(filename);
    libc_open_ExpectAndReturn(filename, O_WRONLY | O_DSYNC, fd);
    // write NOP for first fsc_item
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            0,
            0);
    libc_pwrite_IgnoreArg_buf();
    // write first fsc_item
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row),
            0);
    libc_pwrite_IgnoreArg_buf();
    // write NOP for second fsc_item
    libc_pwrite_ExpectAndReturn(fd,
            NULL,
            sizeof(struct acmdrv_sched_tbl_row),
            sizeof(struct acmdrv_sched_tbl_row) * 2,
            -1);
    libc_pwrite_IgnoreArg_buf();
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing NOP schedule items");
    libc___errno_location_ExpectAndReturn(&my_errno);
    logging_Expect(LOGLEVEL_ERR, "Sysfs: problem writing to %s ");
    libc_close_ExpectAndReturn(fd, 0);

    result = write_fsc_schedules_to_HW(&module, table_index);
    TEST_ASSERT_EQUAL(-EIO, result);
}
