#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "status.h"

/* directly added modules */
#include "setup_helper.h"

/* mock modules needed */
#include "mock_memory.h"
#include "mock_sysfs.h"
#include "mock_logging.h"

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

void test_status_read_item(void) {
    int64_t status_value;
    char path[] = ACMDEV_BASE "/status/disable_overrun_prev_M0";

    read_uint64_sysfs_item_ExpectAndReturn((const char* )&path, 25);
    read_uint64_sysfs_item_IgnoreArg_path_name();
    status_value = status_read_item(0, STATUS_DISABLE_OVERRUN_PREV_CYCLE);
    TEST_ASSERT_EQUAL_INT64(25, status_value);
}

void test_status_read_item_invalid_module(void) {
    int64_t status_value;

    logging_Expect(0, "Status: module_id out of range: %d");
    status_value = status_read_item(3, 5);
    TEST_ASSERT_EQUAL_INT64(-EINVAL, status_value);
}

void test_status_read_item_invalid_item(void) {
    int64_t status_value;

    logging_Expect(0, "Status: acm_status_item out of range: %d");
    status_value = status_read_item(1, 16);
    TEST_ASSERT_EQUAL_INT64(-EINVAL, status_value);
}

/* Tests for function: struct acm_diagnostic* __must_check status_read_diagnostics
 * (enum acm_module_id module_id);*/

void test_status_read_diagnostics_problem_read(void) {
    struct acm_diagnostic *values_read;
    char path[] = ACMDEV_BASE "diag/diagnostics_M0";
    struct acmdrv_diagnostics packed;
    memset(&packed, 0, sizeof(packed));

    read_buffer_sysfs_item_ExpectAndReturn((const char* )&path,
            (char* )&packed,
            sizeof (packed),
            0,
            -EACCES);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    logging_Expect(0, "Status: problem reading data from file %s");

    values_read = status_read_diagnostics(MODULE_0);
    TEST_ASSERT_EQUAL_PTR(NULL, values_read);
}

void test_status_read_diagnostics_problem_alloc_mem(void) {
    struct acm_diagnostic *values_read;
    char path[] = ACMDEV_BASE "diag/diagnostics_M0";
    struct acmdrv_diagnostics packed;
    memset(&packed, 0, sizeof(packed));

    read_buffer_sysfs_item_ExpectAndReturn((const char* )&path,
            (char* )&packed,
            sizeof (packed),
            0,
            0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer((char* )&packed, sizeof (packed));
    acm_zalloc_ExpectAndReturn(sizeof (*values_read), NULL);
    logging_Expect(0, "Status: out of memory");
    values_read = status_read_diagnostics(MODULE_0);
    TEST_ASSERT_EQUAL_PTR(NULL, values_read);
}

void test_status_read_diagnostics(void){
    struct acm_diagnostic *values_read;
    struct acm_diagnostic diagnostic_mem;
    memset(&diagnostic_mem, 0, sizeof(diagnostic_mem));
    uint32_t expected[16] = { 0 };
    //struct acm_diagnostic unpacked;
    struct acmdrv_diagnostics packed = { 0x99005d9600000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000, 0x00000000, 0x00000000,
            0x00000000, 0x00000000};
    char path[] = ACMDEV_BASE "diag/diagnostics_M0";

    read_buffer_sysfs_item_ExpectAndReturn((const char* )&path,
            (char* )&packed,
            sizeof (packed),
            0,
            0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer((char* )&packed, sizeof (packed));
    acm_zalloc_ExpectAndReturn(sizeof (*values_read), &diagnostic_mem);
    values_read = status_read_diagnostics(MODULE_0);
    TEST_ASSERT_EQUAL_UINT64(-7421829287080099840, values_read->timestamp.tv_sec);
    TEST_ASSERT_EQUAL_INT32(0, values_read->timestamp.tv_nsec);
    TEST_ASSERT_EQUAL_INT32(0, values_read->scheduleCycleCounter);
    TEST_ASSERT_EQUAL_INT32(0, values_read->txFramesCounter);
    TEST_ASSERT_EQUAL_INT32(0, values_read->rxFramesCounter);
    TEST_ASSERT_EQUAL_INT32(0, values_read->ingressWindowClosedFlags);
    TEST_ASSERT_EQUAL_INT32_ARRAY(expected, values_read->ingressWindowClosedCounter, 16);
    TEST_ASSERT_EQUAL_INT32(0, values_read->noFrameReceivedFlags);
    TEST_ASSERT_EQUAL_INT32_ARRAY(expected, values_read->noFrameReceivedCounter, 16);
    TEST_ASSERT_EQUAL_INT32(0, values_read->recoveryFlags);
    TEST_ASSERT_EQUAL_INT32_ARRAY(expected, values_read->recoveryCounter, 16);
    TEST_ASSERT_EQUAL_INT32(0, values_read->additionalFilterMismatchFlags);
    TEST_ASSERT_EQUAL_INT32_ARRAY(expected, values_read->additionalFilterMismatchCounter, 16);
}

/* Tests for function: void convert_diag2unpacked(struct acmdrv_diagnostics *destination,
 *  struct acm_diagnostic *target);*/
void test_status_read_time_freq(void) {
    int64_t time_freq;
    char path[] = ACMDEV_BASE "/status/time_freq";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "time_freq", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_uint64_sysfs_item_ExpectAndReturn((const char* )&path, 100000000);
    read_uint64_sysfs_item_IgnoreArg_path_name();
    time_freq = status_read_time_freq();
    TEST_ASSERT_EQUAL_INT64(100000000, time_freq);
}

void test_status_read_time_freq_neg_resp_construct_path(void) {
    int64_t time_freq;
//	char path[] = ACMDEV_BASE "/status/time_freq" ;

    sysfs_construct_path_name_ExpectAndReturn("/anypath",
            SYSFS_PATH_LENGTH,
            "status",
            "time_freq",
            -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    time_freq = status_read_time_freq();
    TEST_ASSERT_EQUAL_INT64(-ENOMEM, time_freq);
}

void test_calc_tick_duration(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/time_freq";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "time_freq", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_uint64_sysfs_item_ExpectAndReturn((const char* )&path, 100000000);
    read_uint64_sysfs_item_IgnoreArg_path_name();
    result = calc_tick_duration();
    TEST_ASSERT_EQUAL(10, result);
}

void test_calc_tick_duration_alternat_value(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/time_freq";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "time_freq", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_uint64_sysfs_item_ExpectAndReturn((const char* )&path, 125000000);
    read_uint64_sysfs_item_IgnoreArg_path_name();
    result = calc_tick_duration();
    TEST_ASSERT_EQUAL(8, result);
}

void test_calc_tick_duration_null(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/time_freq";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "time_freq", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    read_uint64_sysfs_item_ExpectAndReturn((const char* )&path, -ENOMEM);
    read_uint64_sysfs_item_IgnoreArg_path_name();
    result = calc_tick_duration();
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_read_capability_item_schedule_tick(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/time_freq";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "time_freq", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 100000000);
    result = status_read_capability_item(CAP_MIN_SCHEDULE_TICK);
    TEST_ASSERT_EQUAL(10, result);
}

void test_status_read_capability_item_msg_buff_size(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/msgbuf_memsize";

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "status",
            "msgbuf_memsize",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 16318);
    result = status_read_capability_item(CAP_MAX_MESSAGE_BUFFER_SIZE);
    TEST_ASSERT_EQUAL(16318, result);
}

void test_status_read_capability_item_read_back(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/cfg_read_back";

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "status",
            "cfg_read_back",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 1);
    result = status_read_capability_item(CAP_CONFIG_READBACK);
    TEST_ASSERT_EQUAL(1, result);
}

void test_status_read_capability_item_debug(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/debug_enable";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "debug_enable", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 1);
    result = status_read_capability_item(CAP_DEBUG);
    TEST_ASSERT_EQUAL(1, result);
}

void test_status_read_capability_item_msgbuf_count(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/msgbuf_count";

    sysfs_construct_path_name_ExpectAndReturn(path, SYSFS_PATH_LENGTH, "status", "msgbuf_count", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 32);
    result = status_read_capability_item(CAP_MAX_ANZ_MESSAGE_BUFFER);
    TEST_ASSERT_EQUAL(32, result);
}

void test_status_read_capability_item_msgbuf_datawidth(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/msgbuf_datawidth";

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "status",
            "msgbuf_datawidth",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 4);
    result = status_read_capability_item(CAP_MESSAGE_BUFFER_BLOCK_SIZE);
    TEST_ASSERT_EQUAL(4, result);
}

void test_status_read_capability_item_rx_redundancy(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/rx_redundancy";

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "status",
            "rx_redundancy",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 1);
    result = status_read_capability_item(CAP_REDUNDANCY_RX);
    TEST_ASSERT_EQUAL(1, result);
}

void test_status_read_capability_item_individual_recovery(void) {
    int result;
    char path[] = ACMDEV_BASE "/status/individual_recovery";

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "status",
            "individual_recovery",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path, 1);
    result = status_read_capability_item(CAP_INDIV_RECOVERY);
    TEST_ASSERT_EQUAL(1, result);
}

void test_status_read_capability_invalid_input(void) {
    int result;

    logging_Expect(0, "Status: unknown capability item id: %d");
    result = status_read_capability_item(CAP_INDIV_RECOVERY + 1);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_status_set_diagnostics_poll_time(void) {
    int result;
    char path[] = ACMDEV_BASE "diag/diag_poll_time_M0";

    sysfs_delete_file_content_Expect(path);
    write_file_sysfs_ExpectAndReturn(path, "2000", strlen("2000") + 1, 0, 0);
    result = status_set_diagnostics_poll_time(0, 2000);
    TEST_ASSERT_EQUAL(0, result);
}

void test_status_read_config_identifier(void) {
    int result;

    sysfs_read_configuration_id_ExpectAndReturn(12345);
    result = status_read_config_identifier();
    TEST_ASSERT_EQUAL(12345, result);
}

void test_status_get_buffer_id_from_name_nobuffername(void) {
    int result;

    logging_Expect(0, "Status: no msg buffer name or name length zero");
    result = status_get_buffer_id_from_name(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_status_get_buffer_id_from_name_namelength_zero(void) {
    int result;
    char msg_buff[] = "";

    logging_Expect(0, "Status: no msg buffer name or name length zero");
    result = status_get_buffer_id_from_name(msg_buff);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_status_get_buffer_id_from_name_neg_resp_construct_path(void) {
    int result;
    char msg_buff[] = "buffer 1";

    sysfs_construct_path_name_ExpectAndReturn("/anypath",
            SYSFS_PATH_LENGTH,
            "config_bin",
            "msg_buff_alias",
            -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    result = status_get_buffer_id_from_name(msg_buff);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_get_buffer_id_from_name_buffername_not_found(void) {
    int result, fd, i;
    char msg_buff[] = "buffer not found";
    char path[] = ACMDEV_BASE "/config_bin/msg_buff_alias";
    char path_name[] = ACMDEV_BASE "/status/msgbuf_count";
    struct acmdrv_buff_alias test_data;

    /* prepare testcase */
    #define ACM_MAX_MESSAGE_BUFFERS 64
    fd = open(path, O_WRONLY);
    memset(&test_data, 0, sizeof (test_data));
    for (i = 0; i < ACM_MAX_MESSAGE_BUFFERS; i++) {
        result = pwrite(fd, &test_data, sizeof (test_data), i * sizeof (test_data));
        TEST_ASSERT_EQUAL(sizeof (test_data), result);
    }
    close(fd);

    /* execute test */
    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "config_bin",
            "msg_buff_alias",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path_name,
            SYSFS_PATH_LENGTH,
            "status",
            "msgbuf_count",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path_name, strlen(path_name) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path_name, 32);
    logging_Expect(0, "Status: message buffer name not found, number of mess_buffs: %d");
    result = status_get_buffer_id_from_name(msg_buff);
    TEST_ASSERT_EQUAL(-EACMBUFFNAME, result);
}

void test_status_get_buffer_id_from_name_open_file_problem(void) {
    int result;
    char msg_buff[] = "buffer 1";
    char path[] = ACMDEV_BASE "/config_bin/msg_buff_aliases";
    char path_name[] = ACMDEV_BASE "/status/msgbuf_count";

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "config_bin",
            "msg_buff_alias",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path_name,
            SYSFS_PATH_LENGTH,
            "status",
            "msgbuf_count",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path_name, strlen(path_name) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path_name, 32);
    logging_Expect(0, "Sysfs: open file %s failed");
    result = status_get_buffer_id_from_name(msg_buff);
    TEST_ASSERT_EQUAL(-ENOENT, result);
}

void test_status_get_buffer_id_from_name_read_problem(void) {
    int result;
    FILE *file;
    char mode[] = "w";
    char msg_buff[] = "buffer_found";
    char path[] = ACMDEV_BASE "/config_bin/msg_buff_alias";
    char path_name[] = ACMDEV_BASE "/status/msgbuf_count";

    /* prepare testcase */
    file = fopen(path, mode);
    fclose(file);

    /* execute testcase */
    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "config_bin",
            "msg_buff_alias",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path_name,
            SYSFS_PATH_LENGTH,
            "status",
            "msgbuf_count",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path_name, strlen(path_name) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path_name, 32);
    logging_Expect(0, "Status: message buffer name not found, number of mess_buffs: %d");
    result = status_get_buffer_id_from_name(msg_buff);
    TEST_ASSERT_EQUAL(-EACMBUFFNAME, result);
}

void test_status_get_buffer_id_from_name(void) {
    int result, fd;
    char msg_buff[] = "buffer_found";
    char path[] = ACMDEV_BASE "/config_bin/msg_buff_alias";
    char path_name[] = ACMDEV_BASE "/status/msgbuf_count";
    struct acmdrv_buff_alias test_data;

    /* prepare testcase */
    fd = open(path, O_WRONLY);
    memset(&test_data, 0, sizeof (test_data));
    test_data.id = 0;
    test_data.idx = 0;
    strcpy(test_data.alias, "buffer 1");
    result = pwrite(fd, (char*) &test_data, sizeof (test_data), 0);
    TEST_ASSERT_EQUAL(sizeof (test_data), result);
    memset(&test_data, 0, sizeof (test_data));
    test_data.id = 0;
    test_data.idx = 1;
    strcpy(test_data.alias, "buffer_found");
    result = pwrite(fd, (char*) &test_data, sizeof (test_data), sizeof (test_data));
    TEST_ASSERT_EQUAL(sizeof (test_data), result);
    close(fd);

    /* execute testcase */
    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "config_bin",
            "msg_buff_alias",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path_name,
            SYSFS_PATH_LENGTH,
            "status",
            "msgbuf_count",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path_name, strlen(path_name) + 1);
    read_uint64_sysfs_item_ExpectAndReturn(path_name, 32);
    result = status_get_buffer_id_from_name(msg_buff);
    TEST_ASSERT_EQUAL(1, result);
}

void test_status_read_buffer_locking_vector_neg_resp_construct_path(void) {
    int result;

    sysfs_construct_path_name_ExpectAndReturn("/anypath",
            SYSFS_PATH_LENGTH,
            "control_bin",
            "lock_msg_bufs",
            -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    result = status_read_buffer_locking_vector();
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_read_buffer_locking_vector(void) {
    int result;
    char path[] = ACMDEV_BASE "/control_bin/lock_msg_bufs";
    char *lock_mask;

    sysfs_construct_path_name_ExpectAndReturn(path,
            SYSFS_PATH_LENGTH,
            "control_bin",
            "lock_msg_bufs",
            0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path, strlen(path) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path, lock_mask, sizeof(int64_t), 0, 42300);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    result = status_read_buffer_locking_vector();
    TEST_ASSERT_EQUAL(42300, result);
}

void test_get_int32_status_value_neg_resp_construct_path(void) {
    int32_t result;

    sysfs_construct_path_name_ExpectAndReturn("/anypath",
            SYSFS_PATH_LENGTH,
            "status",
            "debug_enable",
            -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    result = get_int32_status_value(__stringify(ACM_SYSFS_DEBUG));
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_status_get_ip_version(void) {
    char *result;
    char buffer1[] = "0x1301";
    char buffer2[] = "1.1.0";
    char buffer3[] = "1970c9c2";
    char path1[] = ACMDEV_BASE "/status/device_id";
    char path2[] = ACMDEV_BASE "/status/version_id";
    char path3[] = ACMDEV_BASE "/status/revision_id";

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH, "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH, "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path2, strlen(path2) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path2, buffer2, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer2, strlen(buffer2) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path3, SYSFS_PATH_LENGTH, "status", "revision_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path3, strlen(path3) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path3, buffer3, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer3, strlen(buffer3) + 1);
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING("0x1301-1.1.0-1970c9c2", result);
    free(result);
}

void test_status_get_ip_version_newline(void) {
    char *result;
    char buffer1[] = "0x1301\n";
    char buffer2[] = "1.1.0\n";
    char buffer3[] = "1970c9c2\n";
    char path1[] = ACMDEV_BASE "/status/device_id" ;
    char path2[] = ACMDEV_BASE "/status/version_id" ;
    char path3[] = ACMDEV_BASE "/status/revision_id" ;

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH,
            "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1)+1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH,
            "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path2, strlen(path2)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path2, buffer2, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer2, strlen(buffer2)+1);
    sysfs_construct_path_name_ExpectAndReturn(path3, SYSFS_PATH_LENGTH,
            "status", "revision_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path3, strlen(path3)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path3, buffer3, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer3, strlen(buffer3)+1);
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING("0x1301-1.1.0-1970c9c2", result);
    free(result);
}

void test_status_get_ip_version_crlf(void) {
    char *result;
    char buffer1[] = "0x1301\r\n";
    char buffer2[] = "1.1.0\r\n";
    char buffer3[] = "1970c9c2\r\n";
    char path1[] = ACMDEV_BASE "/status/device_id" ;
    char path2[] = ACMDEV_BASE "/status/version_id" ;
    char path3[] = ACMDEV_BASE "/status/revision_id" ;

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH,
            "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1)+1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH,
            "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path2, strlen(path2)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path2, buffer2, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer2, strlen(buffer2)+1);
    sysfs_construct_path_name_ExpectAndReturn(path3, SYSFS_PATH_LENGTH,
            "status", "revision_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path3, strlen(path3)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path3, buffer3, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer3, strlen(buffer3)+1);
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING("0x1301-1.1.0-1970c9c2", result);
    free(result);
}

void test_status_get_ip_version_neg_path1(void) {
    char *result;
    char path1[] = ACMDEV_BASE "/status/device_id" ;

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH,
            "status", "device_id", -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING(NULL, result);
}

void test_status_get_ip_version_neg_read1(void) {
    char *result;
    char buffer1[] = "0x1301";
    char path1[] = ACMDEV_BASE "/status/device_id" ;

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH,
            "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, -ENOENT);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING(NULL, result);
}

void test_status_get_ip_version_neg_path2(void) {
    char *result;
    char buffer1[] = "0x1301";
    char path1[] = ACMDEV_BASE "/status/device_id" ;
    char path2[] = ACMDEV_BASE "/status/version_id" ;

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH,
            "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1)+1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1)+1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH,
            "status", "version_id", -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING(NULL, result);
}

void test_status_get_ip_version_neg_read2(void) {
    char *result;
    char buffer1[] = "0x1301";
    char buffer2[] = "1.1.0";
    char path1[] = ACMDEV_BASE "/status/device_id";
    char path2[] = ACMDEV_BASE "/status/version_id";

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH, "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH, "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path2, strlen(path2) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path2, buffer2, SYSFS_PATH_LENGTH, 0, -ENOENT);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING(NULL, result);
}

void test_status_get_ip_version_neg_path3(void) {
    char *result;
    char buffer1[] = "0x1301";
    char buffer2[] = "1.1.0";
    char path1[] = ACMDEV_BASE "/status/device_id";
    char path2[] = ACMDEV_BASE "/status/version_id";
    char path3[] = ACMDEV_BASE "/status/revision_id";

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH, "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH, "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path2, strlen(path2) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path2, buffer2, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer2, strlen(buffer2) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path3,
            SYSFS_PATH_LENGTH,
            "status",
            "revision_id",
            -ENOMEM);
    sysfs_construct_path_name_IgnoreArg_path_name();
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING(NULL, result);
}

void test_status_get_ip_version_neg_read3(void) {
    char *result;
    char buffer1[] = "0x1301";
    char buffer2[] = "1.1.0";
    char buffer3[] = "1970c9c2";
    char path1[] = ACMDEV_BASE "/status/device_id";
    char path2[] = ACMDEV_BASE "/status/version_id";
    char path3[] = ACMDEV_BASE "/status/revision_id";

    sysfs_construct_path_name_ExpectAndReturn(path1, SYSFS_PATH_LENGTH, "status", "device_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path1, strlen(path1) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path1, buffer1, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer1, strlen(buffer1) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path2, SYSFS_PATH_LENGTH, "status", "version_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path2, strlen(path2) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path2, buffer2, SYSFS_PATH_LENGTH, 0, 0);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    read_buffer_sysfs_item_ReturnMemThruPtr_buffer(buffer2, strlen(buffer2) + 1);
    sysfs_construct_path_name_ExpectAndReturn(path3, SYSFS_PATH_LENGTH, "status", "revision_id", 0);
    sysfs_construct_path_name_IgnoreArg_path_name();
    sysfs_construct_path_name_ReturnMemThruPtr_path_name(path3, strlen(path3) + 1);
    read_buffer_sysfs_item_ExpectAndReturn(path3, buffer3, SYSFS_PATH_LENGTH, 0, -ENOENT);
    read_buffer_sysfs_item_IgnoreArg_buffer();
    result = status_get_ip_version();
    TEST_ASSERT_EQUAL_STRING(NULL, result);
}
