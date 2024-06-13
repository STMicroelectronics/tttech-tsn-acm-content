#include "unity.h"

#include <string.h>
#include <errno.h>
#include <sys/queue.h>
#include <linux/acm/acmdrv.h>

#include "module.h"
#include "tracing.h"

#include "mock_logging.h"
#include "mock_memory.h"
#include "mock_config.h"
#include "mock_stream.h"
#include "mock_sysfs.h"
#include "mock_status.h"
#include "mock_schedule.h"
#include "mock_validate.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_module_create(void) {
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char delay_str[12];

    for (int i = 0; i < 2; i++) {
        struct acm_module *result;

        acm_zalloc_ExpectAndReturn(sizeof (module), &module);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_IN_100MBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("10", strlen("10") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_EG_100MBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("100", strlen("100") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_IN_100MBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("1000", strlen("1000") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_EG_100MBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("210", strlen("210") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_BYPASS_100MBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("1", strlen("1") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_SWITCH_100MBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("99999", strlen("99999") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_IN_1GBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("0010", strlen("0010") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_EG_1GBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("20", strlen("20") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_IN_1GBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("170", strlen("170") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_EG_1GBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("4294967295",
                strlen("4294967295") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_BYPASS_1GBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("2147483647",
                strlen("2147483647") + 1);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_SWITCH_1GBps,
                delay_str,
                sizeof (delay_str),
                0);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ReturnMemThruPtr_config_value("35353535", strlen("35353535") + 1);
        result = module_create(CONN_MODE_PARALLEL, SPEED_1GBps, i);
        TEST_ASSERT_EQUAL_PTR(&module, result);
        TEST_ASSERT_EQUAL_PTR(10, module.module_delays[SPEED_100MBps].chip_in);
        TEST_ASSERT_EQUAL_PTR(100, module.module_delays[SPEED_100MBps].chip_eg);
        TEST_ASSERT_EQUAL_PTR(1000, module.module_delays[SPEED_100MBps].phy_in);
        TEST_ASSERT_EQUAL_PTR(210, module.module_delays[SPEED_100MBps].phy_eg);
        TEST_ASSERT_EQUAL_PTR(1, module.module_delays[SPEED_100MBps].ser_bypass);
        TEST_ASSERT_EQUAL_PTR(99999, module.module_delays[SPEED_100MBps].ser_switch);
        TEST_ASSERT_EQUAL_PTR(10, module.module_delays[SPEED_1GBps].chip_in);
        TEST_ASSERT_EQUAL_PTR(20, module.module_delays[SPEED_1GBps].chip_eg);
        TEST_ASSERT_EQUAL_PTR(170, module.module_delays[SPEED_1GBps].phy_in);
        TEST_ASSERT_EQUAL_PTR(4294967295, module.module_delays[SPEED_1GBps].phy_eg);
        TEST_ASSERT_EQUAL_PTR(2147483647, module.module_delays[SPEED_1GBps].ser_bypass);
        TEST_ASSERT_EQUAL_PTR(35353535, module.module_delays[SPEED_1GBps].ser_switch);
    }
}

void test_module_create_wrong_delays_configfile(void) {
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    char delay_str[12];

    for (int i = 0; i < 2; i++) {
        struct acm_module *result;

        acm_zalloc_ExpectAndReturn(sizeof (module), &module);
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_IN_100MBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_EG_100MBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_IN_100MBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_EG_100MBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_BYPASS_100MBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_SWITCH_100MBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_IN_1GBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_EG_1GBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_IN_1GBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_PHY_EG_1GBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_BYPASS_1GBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        sysfs_get_configfile_item_ExpectAndReturn(KEY_SER_SWITCH_1GBps,
                delay_str,
                sizeof (delay_str),
                -EACMCONFIG);
        sysfs_get_configfile_item_IgnoreArg_config_value();
        result = module_create(CONN_MODE_PARALLEL, SPEED_1GBps, i);
        TEST_ASSERT_EQUAL_PTR(&module, result);
        TEST_ASSERT_EQUAL_PTR(50, module.module_delays[SPEED_100MBps].chip_in);
        TEST_ASSERT_EQUAL_PTR(120, module.module_delays[SPEED_100MBps].chip_eg);
        TEST_ASSERT_EQUAL_PTR(404, module.module_delays[SPEED_100MBps].phy_in);
        TEST_ASSERT_EQUAL_PTR(444, module.module_delays[SPEED_100MBps].phy_eg);
        TEST_ASSERT_EQUAL_PTR(2844, module.module_delays[SPEED_100MBps].ser_bypass);
        TEST_ASSERT_EQUAL_PTR(3900, module.module_delays[SPEED_100MBps].ser_switch);
        TEST_ASSERT_EQUAL_PTR(50, module.module_delays[SPEED_1GBps].chip_in);
        TEST_ASSERT_EQUAL_PTR(120, module.module_delays[SPEED_1GBps].chip_eg);
        TEST_ASSERT_EQUAL_PTR(298, module.module_delays[SPEED_1GBps].phy_in);
        TEST_ASSERT_EQUAL_PTR(199, module.module_delays[SPEED_1GBps].phy_eg);
        TEST_ASSERT_EQUAL_PTR(439, module.module_delays[SPEED_1GBps].ser_bypass);
        TEST_ASSERT_EQUAL_PTR(940, module.module_delays[SPEED_1GBps].ser_switch);
    }
}

void test_module_create_alloc_null(void) {
    struct acm_module *module;

    for (int i = 0; i < 2; i++) {
        acm_zalloc_ExpectAndReturn(sizeof (*module), NULL);
        logging_Expect(0, "Module: Out of memory");
        module = module_create(CONN_MODE_PARALLEL, SPEED_1GBps, i);
        TEST_ASSERT_EQUAL_PTR(NULL, module);
    }
}

void test_module_create_invalid_module_id(void) {
    struct acm_module *module;

    logging_Expect(0, "Module: module_id out of range: %d");
    module = module_create(CONN_MODE_PARALLEL, SPEED_1GBps, ACM_MODULES_COUNT);
    TEST_ASSERT_EQUAL_PTR(NULL, module);
}

void test_module_destroy(void) {
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    stream_empty_list_Expect(&module.streams);
    acm_free_Expect(&module);

    module_destroy(&module);
}

void test_module_destroy_null(void) {
    module_destroy(NULL);
}

void test_module_add_stream(void) {
    int result;
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    stream_add_list_ExpectAndReturn(&module.streams, &stream, 0);
    result = module_add_stream(&module, &stream);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_add_stream_module_null(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    logging_Expect(0, "Module: module or stream id invalid; module: %d, stream: %d");
    result = module_add_stream(NULL, &stream);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_module_add_stream_stream_null(void) {
    int result;
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    logging_Expect(0, "Module: module or stream id invalid; module: %d, stream: %d");
    result = module_add_stream(&module, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_module_add_stream_stream_add_fails(void) {
    int result;
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    stream_add_list_ExpectAndReturn(&module.streams, &stream, -EINVAL);
    result = module_add_stream(&module, &stream);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_module_add_stream_with_schedules_not_okay(void) {
    struct schedule_entry window_mem;
    memset(&window_mem, 0, sizeof (window_mem));
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare fsc list
    ACMLIST_INIT(&module.fsc_list);
    // prepare stream
    ACMLIST_INIT(&stream.windows);
    stream.type = TIME_TRIGGERED_STREAM;
    // prepare window of stream
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);

    stream_add_list_ExpectAndReturn(&module.streams, &stream, 0);
    logging_Expect(0, "Module: streamlist reference not set in stream");
    stream_remove_list_Expect(&module.streams, &stream);
    calculate_indizes_for_HW_tables_ExpectAndReturn(&module.streams, &stream, -EACMINTERNAL);
    logging_Expect(1, "Module: some inconsistency caused by removing stream again");
    result = module_add_stream(&module, &stream);
    TEST_ASSERT_EQUAL(-EFAULT, result);
}

void test_module_add_stream_schedules_not_okay_neg_indizes(void) {
    struct schedule_entry window_mem;
    memset(&window_mem, 0, sizeof (window_mem));
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare fsc list
    ACMLIST_INIT(&module.fsc_list);
    // prepare stream
    ACMLIST_INIT(&stream.windows);
    stream.type = TIME_TRIGGERED_STREAM;
    // prepare window of stream
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);

    stream_add_list_ExpectAndReturn(&module.streams, &stream, 0);
    logging_Expect(0, "Module: streamlist reference not set in stream");
    stream_remove_list_Expect(&module.streams, &stream);
    calculate_indizes_for_HW_tables_ExpectAndReturn(&module.streams, &stream, 0);
    result = module_add_stream(&module, &stream);
    TEST_ASSERT_EQUAL(-EFAULT, result);
}

void test_module_add_stream_with_schedules_okay(void) {
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare window of stream
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    stream_add_list_ExpectAndReturn(&module.streams, &stream, 0);
    calc_tick_duration_ExpectAndReturn(10);
    create_event_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 20, 5, 0);
    create_event_sysfs_items_IgnoreArg_gather_dma_index();
    create_event_sysfs_items_IgnoreArg_redundand_index();
    validate_stream_ExpectAndReturn(&stream, false, 0);
    result = module_add_stream(&module, &stream);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_set_schedule(void) {
    int result;
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct timespec start_time;
    start_time.tv_nsec = 0;
    start_time.tv_sec = 1000;

    result = module_set_schedule(&module, 1000000, start_time);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, module.start.tv_nsec);
    TEST_ASSERT_EQUAL(1000, module.start.tv_sec);
    TEST_ASSERT_EQUAL(1000000, module.cycle_ns);
}

void test_module_set_schedule_module_null(void) {
    int ret;
    struct timespec start_time;
    start_time.tv_nsec = 0;
    start_time.tv_sec = 1000;

    logging_Expect(0, "Module: Invalid module input: %d");

    ret = module_set_schedule(NULL, 1000000, start_time);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_module_set_schedule_cycle_zero(void) {
    int result;
    struct acm_module module =
    MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);
    struct timespec start_time;
    start_time.tv_nsec = 0;
    start_time.tv_sec = 1000;

    logging_Expect(0, "Module: Cycle Time needs value greater 0");
    result = module_set_schedule(&module, 0, start_time);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_module_add_schedules_time_triggered_stream(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare window of stream
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;

    stream.gather_dma_index = 5;
    stream.redundand_index = 7;

    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    create_event_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 5, 7, 0);
    validate_stream_ExpectAndReturn(&stream, false, 0);

    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_add_schedules_ingress_triggered_stream(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare window of stream
    stream.lookup_index = 9;
    window_mem.period_ns = 700;
    window_mem.time_start_ns = 200;
    window_mem.time_end_ns = 400;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    create_window_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 0, 9, false, 0);
    validate_stream_ExpectAndReturn(&stream, false, 0);

    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_add_schedules_ingress_event_stream(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_stream event_stream = STREAM_INITIALIZER(event_stream, EVENT_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare stream
    stream.reference = &event_stream;
    stream.lookup_index = 9;
    event_stream.reference_parent = &stream;
    // prepare window of stream
    window_mem.period_ns = 700;
    window_mem.time_start_ns = 200;
    window_mem.time_end_ns = 400;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    create_window_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 0, 9, false, 0);
    validate_stream_ExpectAndReturn(&stream, false, 0);

    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_add_schedules_ingress_event_recovery_stream(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_stream event_stream = STREAM_INITIALIZER(event_stream, EVENT_STREAM);
    struct acm_stream recovery_stream = STREAM_INITIALIZER(recovery_stream, RECOVERY_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare stream
    stream.reference = &event_stream;
    stream.lookup_index = 9;
    event_stream.reference_parent = &stream;
    event_stream.reference = &recovery_stream;
    event_stream.gather_dma_index = 77;
    recovery_stream.reference_parent = &event_stream;
    recovery_stream.gather_dma_index = 88;
    // prepare window of stream
    window_mem.period_ns = 700;
    window_mem.time_start_ns = 200;
    window_mem.time_end_ns = 400;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    create_window_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 88, 9, true, 0);
    validate_stream_ExpectAndReturn(&stream, false, 0);

    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_add_schedules_wrong_stream_type(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, MAX_STREAM_TYPE);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare window of stream
    stream.gather_dma_index = 5;
    stream.redundand_index = 7;
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    logging_Expect(0, "Module: Invalid stream type: %d");
    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(-EPERM, result);
}

void test_module_add_schedules_streamlist_null(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    logging_Expect(0, "Module: streamlist reference not set in stream");
    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(-EFAULT, result);
}

void test_module_add_schedules_wrong_tick_duration(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(0);
    logging_Expect(0, "Module: Invalid value for tick duration: %d");
    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_module_add_schedules_error_create_fsc_item(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare window of stream
    stream.gather_dma_index = 5;
    stream.redundand_index = 7;
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    create_event_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 5, 7, -ENOMEM);
    logging_Expect(0, "Module: problem at creation of sysfs schedules");
    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_module_add_schedules_error_validate_stream(void) {
    int result;
    struct schedule_entry window_mem = SCHEDULE_ENTRY_INITIALIZER;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    // prepare window of stream
    stream.gather_dma_index = 5;
    stream.redundand_index = 7;
    window_mem.period_ns = 700;
    window_mem.send_time_ns = 600;
    ACMLIST_INSERT_TAIL(&stream.windows, &window_mem, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    calc_tick_duration_ExpectAndReturn(10);
    create_event_sysfs_items_ExpectAndReturn(&window_mem, &module, 10, 5, 7, 0);
    validate_stream_ExpectAndReturn(&stream, false, -10);
    logging_Expect(0, "Module: Validation not successful");

    result = module_add_schedules(&stream, &window_mem);
    TEST_ASSERT_EQUAL(-10, result);
}

void test_remove_schedule_sysfs_items_schedule(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);
    struct schedule_entry schedule_item1 = SCHEDULE_ENTRY_INITIALIZER;
    struct schedule_entry schedule_item2 = SCHEDULE_ENTRY_INITIALIZER;
    struct fsc_command fsc_schedule_mem1 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem2 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem3 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem4 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem5 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem6 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem7 = COMMAND_INITIALIZER(0, 0);
    struct fsc_command fsc_schedule_mem8 = COMMAND_INITIALIZER(0, 0);

    // set test preconditions
    // prepare windows of stream
    schedule_item1.period_ns = 700;
    schedule_item1.send_time_ns = 600;
    schedule_item2.period_ns = 8000;
    schedule_item2.send_time_ns = 2000;
    ACMLIST_INSERT_TAIL(&stream.windows, &schedule_item1, entry);
    ACMLIST_INSERT_TAIL(&stream.windows, &schedule_item2, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    // prepare fsc items and fsc_list
    fsc_schedule_mem1.schedule_reference = &schedule_item1;
    fsc_schedule_mem2.schedule_reference = &schedule_item2;
    fsc_schedule_mem3.schedule_reference = &schedule_item1;
    fsc_schedule_mem4.schedule_reference = &schedule_item1;
    fsc_schedule_mem5.schedule_reference = &schedule_item2;
    fsc_schedule_mem6.schedule_reference = &schedule_item1;
    fsc_schedule_mem7.schedule_reference = &schedule_item2;
    fsc_schedule_mem8.schedule_reference = &schedule_item1;
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem1, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem2, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem3, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem4, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem5, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem6, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem7, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem8, entry);

    // execute test
    acm_free_Expect(&fsc_schedule_mem1);
    acm_free_Expect(&fsc_schedule_mem3);
    acm_free_Expect(&fsc_schedule_mem4);
    acm_free_Expect(&fsc_schedule_mem6);
    acm_free_Expect(&fsc_schedule_mem8);
    remove_schedule_sysfs_items_schedule(&schedule_item1, &module);
    TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&module.fsc_list));
}

void test_remove_schedule_sysfs_items_stream(void) {
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);
    struct acm_stream stream1 = STREAM_INITIALIZER(stream1, TIME_TRIGGERED_STREAM);
    struct acm_stream stream2 = STREAM_INITIALIZER(stream2, TIME_TRIGGERED_STREAM);
    struct schedule_entry schedule_item1,
    schedule_item2, schedule_item3;
    memset(&schedule_item1, 0, sizeof (schedule_item1));
    memset(&schedule_item2, 0, sizeof (schedule_item2));
    memset(&schedule_item3, 0, sizeof (schedule_item3));
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

    ACMLIST_INSERT_TAIL(&module.streams, &stream1, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream2, entry);
    // prepare windows of streams
    schedule_item1.period_ns = 700;
    schedule_item1.send_time_ns = 600;
    schedule_item2.period_ns = 8000;
    schedule_item2.send_time_ns = 2000;
    schedule_item3.period_ns = 2000;
    schedule_item3.send_time_ns = 1100;
    ACMLIST_INSERT_TAIL(&stream1.windows, &schedule_item1, entry);
    ACMLIST_INSERT_TAIL(&stream1.windows, &schedule_item2, entry);
    ACMLIST_INSERT_TAIL(&stream2.windows, &schedule_item3, entry);
    // prepare fsc items and fsc_list
    fsc_schedule_mem1.schedule_reference = &schedule_item3;
    fsc_schedule_mem2.schedule_reference = &schedule_item2;
    fsc_schedule_mem3.schedule_reference = &schedule_item1;
    fsc_schedule_mem4.schedule_reference = &schedule_item3;
    fsc_schedule_mem5.schedule_reference = &schedule_item2;
    fsc_schedule_mem6.schedule_reference = &schedule_item1;
    fsc_schedule_mem7.schedule_reference = &schedule_item2;
    fsc_schedule_mem8.schedule_reference = &schedule_item1;
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem1, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem2, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem3, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem4, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem5, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem6, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem7, entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_schedule_mem8, entry);

    // execute test
    acm_free_Expect(&fsc_schedule_mem3);
    acm_free_Expect(&fsc_schedule_mem6);
    acm_free_Expect(&fsc_schedule_mem8);
    acm_free_Expect(&fsc_schedule_mem2);
    acm_free_Expect(&fsc_schedule_mem5);
    acm_free_Expect(&fsc_schedule_mem7);

    remove_schedule_sysfs_items_stream(&stream1, &module);
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&module.fsc_list));
}

void test_remove_schedule_sysfs_items_stream_stream_null(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    logging_Expect(0, "Module: no stream as input in %s");
    remove_schedule_sysfs_items_stream(NULL, &module);
}

void test_remove_schedule_sysfs_items_stream_module_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    logging_Expect(0, "Module: no module as input in %s");
    remove_schedule_sysfs_items_stream(&stream, NULL);
}

void test_fsc_command_empty_list_no_list(void) {
    fsc_command_empty_list(NULL);
}

void test_fsc_command_empty_list(void) {
    struct fsc_command_list fsc_list = ACMLIST_HEAD_INITIALIZER(fsc_list);
    struct fsc_command fsc_schedule_mem1,
    fsc_schedule_mem2, fsc_schedule_mem3, fsc_schedule_mem4, fsc_schedule_mem5, fsc_schedule_mem6,
            fsc_schedule_mem7, fsc_schedule_mem8, fsc_schedule_mem9, fsc_schedule_mem10;
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

    // set test preconditions
    // prepare fsc list
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem1, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem2, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem3, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem4, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem5, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem6, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem7, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem8, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem9, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_schedule_mem10, entry);

    // execute test
    acm_free_Expect(&fsc_schedule_mem1);
    acm_free_Expect(&fsc_schedule_mem2);
    acm_free_Expect(&fsc_schedule_mem3);
    acm_free_Expect(&fsc_schedule_mem4);
    acm_free_Expect(&fsc_schedule_mem5);
    acm_free_Expect(&fsc_schedule_mem6);
    acm_free_Expect(&fsc_schedule_mem7);
    acm_free_Expect(&fsc_schedule_mem8);
    acm_free_Expect(&fsc_schedule_mem9);
    acm_free_Expect(&fsc_schedule_mem10);

    fsc_command_empty_list(&fsc_list);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&fsc_list));
}

void test_write_module_data_to_HW_neg_const_buff(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_lookup_tables(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_scatter_dma(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_gather_dma(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_connection_mode(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_delays(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare testcase
    module.module_id = 1;
    module.mode = CONN_MODE_PARALLEL;
    module.module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, 0);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-EACCES, result);
}

void test_write_module_data_to_HW_neg_redund_table(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare testcase
    module.module_id = 1;
    module.mode = CONN_MODE_PARALLEL;
    module.module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;
    module.module_delays[SPEED_1GBps].phy_in = schedule_delays[SPEED_1GBps].phy_in;
    module.module_delays[SPEED_100MBps].phy_eg = schedule_delays[SPEED_100MBps].phy_eg;
    module.module_delays[SPEED_100MBps].phy_in = schedule_delays[SPEED_100MBps].phy_in;

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, 0);
    sysfs_write_redund_ctrl_table_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_indiv_recov(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare testcase
    module.module_id = 1;
    module.mode = CONN_MODE_PARALLEL;
    module.module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;
    module.module_delays[SPEED_1GBps].phy_in = schedule_delays[SPEED_1GBps].phy_in;
    module.module_delays[SPEED_100MBps].phy_eg = schedule_delays[SPEED_100MBps].phy_eg;
    module.module_delays[SPEED_100MBps].phy_in = schedule_delays[SPEED_100MBps].phy_in;

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, 0);
    sysfs_write_redund_ctrl_table_ExpectAndReturn(&module, 0);
    sysfs_write_individual_recovery_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_cntl_speed(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare testcase
    module.module_id = 1;
    module.mode = CONN_MODE_PARALLEL;
    module.module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;
    module.module_delays[SPEED_1GBps].phy_in = schedule_delays[SPEED_1GBps].phy_in;
    module.module_delays[SPEED_100MBps].phy_eg = schedule_delays[SPEED_100MBps].phy_eg;
    module.module_delays[SPEED_100MBps].phy_in = schedule_delays[SPEED_100MBps].phy_in;

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, 0);
    sysfs_write_redund_ctrl_table_ExpectAndReturn(&module, 0);
    sysfs_write_individual_recovery_ExpectAndReturn(&module, 0);
    sysfs_write_cntl_speed_ExpectAndReturn(&module, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW_neg_module_enable(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare testcase
    module.module_id = 1;
    module.mode = CONN_MODE_PARALLEL;
    module.module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;
    module.module_delays[SPEED_1GBps].phy_in = schedule_delays[SPEED_1GBps].phy_in;
    module.module_delays[SPEED_100MBps].phy_eg = schedule_delays[SPEED_100MBps].phy_eg;
    module.module_delays[SPEED_100MBps].phy_in = schedule_delays[SPEED_100MBps].phy_in;

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, 0);
    sysfs_write_redund_ctrl_table_ExpectAndReturn(&module, 0);
    sysfs_write_individual_recovery_ExpectAndReturn(&module, 0);
    sysfs_write_cntl_speed_ExpectAndReturn(&module, 0);
    sysfs_write_module_enable_ExpectAndReturn(&module, true, -ENOMEM);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_data_to_HW(void) {
    int result;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // prepare testcase
    module.module_id = 1;
    module.mode = CONN_MODE_PARALLEL;
    module.module_delays[SPEED_1GBps].phy_eg = schedule_delays[SPEED_1GBps].phy_eg;
    module.module_delays[SPEED_1GBps].phy_in = schedule_delays[SPEED_1GBps].phy_in;
    module.module_delays[SPEED_100MBps].phy_eg = schedule_delays[SPEED_100MBps].phy_eg;
    module.module_delays[SPEED_100MBps].phy_in = schedule_delays[SPEED_100MBps].phy_in;

    sysfs_write_data_constant_buffer_ExpectAndReturn(&module, 0);
    sysfs_write_lookup_tables_ExpectAndReturn(&module, 0);
    sysfs_write_scatter_dma_ExpectAndReturn(&module, 0);
    sysfs_write_prefetcher_gather_dma_ExpectAndReturn(&module, 0);
    sysfs_write_connection_mode_to_HW_ExpectAndReturn(&module, 0);
    sysfs_write_redund_ctrl_table_ExpectAndReturn(&module, 0);
    sysfs_write_individual_recovery_ExpectAndReturn(&module, 0);
    sysfs_write_cntl_speed_ExpectAndReturn(&module, 0);
    sysfs_write_module_enable_ExpectAndReturn(&module, true, 0);
    result = write_module_data_to_HW(&module);
    TEST_ASSERT_EQUAL(0, result);
}

void test_write_module_schedule_to_HW_neg_free_table(void) {
    int result, free_table;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_read_schedule_status_ExpectAndReturn(&module, &free_table, -EACMNOFREESCHEDTAB);
    sysfs_read_schedule_status_IgnoreArg_free_table();
    result = write_module_schedule_to_HW(&module);
    TEST_ASSERT_EQUAL(-EACMNOFREESCHEDTAB, result);
}

void test_write_module_schedule_to_HW_neg_fsc_schedules(void) {
    int result, free_table = 4;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_read_schedule_status_ExpectAndReturn(&module, &free_table, 0);
    sysfs_read_schedule_status_IgnoreArg_free_table();
    sysfs_read_schedule_status_ReturnMemThruPtr_free_table(&free_table, sizeof(int));
    write_fsc_schedules_to_HW_ExpectAndReturn(&module, 4, -ENOMEM);
    result = write_module_schedule_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_schedule_to_HW_neg_module_schedules(void) {
    int result, free_table = 4;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    sysfs_read_schedule_status_ExpectAndReturn(&module, &free_table, 0);
    sysfs_read_schedule_status_IgnoreArg_free_table();
    sysfs_read_schedule_status_ReturnMemThruPtr_free_table(&free_table, sizeof(int));
    write_fsc_schedules_to_HW_ExpectAndReturn(&module, 4, 0);
    write_module_schedules_to_HW_ExpectAndReturn(&module, free_table, -ENOMEM);
    result = write_module_schedule_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_schedule_to_HW_neg_emergency_disable(void) {
    int result, free_table = 4;
    struct acmdrv_sched_emerg_disable emerg_disable;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    emerg_disable.eme_dis = 0;

    sysfs_read_schedule_status_ExpectAndReturn(&module, &free_table, 0);
    sysfs_read_schedule_status_IgnoreArg_free_table();
    sysfs_read_schedule_status_ReturnMemThruPtr_free_table(&free_table, sizeof(int));
    write_fsc_schedules_to_HW_ExpectAndReturn(&module, 4, 0);
    write_module_schedules_to_HW_ExpectAndReturn(&module, free_table, 0);
    sysfs_write_emergency_disable_ExpectAndReturn(&module, &emerg_disable, -ENOMEM);
    result = write_module_schedule_to_HW(&module);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_write_module_schedule_to_HW(void) {
    int result, free_table = 4;
    struct acmdrv_sched_emerg_disable emerg_disable;
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    emerg_disable.eme_dis = 0;

    sysfs_read_schedule_status_ExpectAndReturn(&module, &free_table, 0);
    sysfs_read_schedule_status_IgnoreArg_free_table();
    sysfs_read_schedule_status_ReturnMemThruPtr_free_table(&free_table, sizeof(int));
    write_fsc_schedules_to_HW_ExpectAndReturn(&module, 4, 0);
    write_module_schedules_to_HW_ExpectAndReturn(&module, free_table, 0);
    sysfs_write_emergency_disable_ExpectAndReturn(&module, &emerg_disable, 0);
    result = write_module_schedule_to_HW(&module);
    TEST_ASSERT_EQUAL(0, result);
}

void test_module_clean_msg_buff_links_no_module(void) {
    module_clean_msg_buff_links(NULL);
}

void test_module_clean_msg_buff_links(void) {
    struct acm_stream stream1, stream2;
    memset(&stream1, 0, sizeof (stream1));
    memset(&stream2, 0, sizeof (stream2));
    struct acm_module module;
    memset(&module, 0, sizeof (module));

    // set test preconditions
    // prepare fsc list
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream1, entry);
    ACMLIST_INSERT_TAIL(&module.streams, &stream2, entry);

    // execute test
    stream_clean_msg_buff_links_Expect(&stream1);
    stream_clean_msg_buff_links_Expect(&stream2);
    module_clean_msg_buff_links(&module);
}

void test_module_get_delay_value_too_long_value_read(void) {
    int result;
    uint32_t read_value = 99;
    char delay_str[12];
    char delay_item[30] = KEY_CHIP_IN_100MBps;

    sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_IN_100MBps,
            delay_str,
            sizeof (delay_str),
            0);
    sysfs_get_configfile_item_IgnoreArg_config_value();
    sysfs_get_configfile_item_ReturnMemThruPtr_config_value("4294967296", strlen("4294967296") + 1);
    logging_Expect(0, "Module: unable to convert value %s of configuration item %s");
    result = module_get_delay_value(delay_item, &read_value);
    TEST_ASSERT_EQUAL(-EINVAL, result);
    TEST_ASSERT_EQUAL(0, read_value);
}

void test_module_get_delay_value_mixed_value_read(void) {
    int result;
    uint32_t read_value = 99;
    char delay_str[12];
    char delay_item[30] = KEY_CHIP_IN_100MBps;

    sysfs_get_configfile_item_ExpectAndReturn(KEY_CHIP_IN_100MBps,
            delay_str,
            sizeof (delay_str),
            0);
    sysfs_get_configfile_item_IgnoreArg_config_value();
    sysfs_get_configfile_item_ReturnMemThruPtr_config_value("42a5", strlen("42a5") + 1);
    //logging_Expect(0, "Module: unable to convert value %s of configuration item %s");
    result = module_get_delay_value(delay_item, &read_value);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(42, read_value);
}

void test_calc_nop_schedules_for_long_cycles(void) {
    uint result;
    struct fsc_command_list fsc_list = COMMANDLIST_INITIALIZER(fsc_list);
    struct fsc_command fsc_item1 = COMMAND_INITIALIZER(0, 70000);
    struct fsc_command fsc_item2 = COMMAND_INITIALIZER(0, 400);
    struct fsc_command fsc_item3 = COMMAND_INITIALIZER(0, 160000);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_item1, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_item2, entry);
    ACMLIST_INSERT_TAIL(&fsc_list, &fsc_item3, entry);

    result = calc_nop_schedules_for_long_cycles(&fsc_list);
    TEST_ASSERT_EQUAL(3, result);
}

void test_calc_nop_schedules_for_long_cycles_no_fsclist(void) {
    uint result;

    result = calc_nop_schedules_for_long_cycles(NULL);
    TEST_ASSERT_EQUAL(0, result);
}
