#include "unity.h"

#include "validate.h"
#include "hwconfig_def.h"
#include "tracing.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "mock_memory.h"
#include "mock_sysfs.h"
#include "mock_logging.h"
#include "mock_config.h"
#include "mock_module.h"
#include "mock_stream.h"
#include "mock_status.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_stream_sum_const_buffer(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation[10];
    memset(&operation, 0, sizeof (operation));
    int result, i;

    // set test preconditions
    // prepare operations list of stream
    ACMLIST_INIT(&stream.operations);
    for (i = 0; i < 10; i++) {
        operation[i].opcode = INSERT_CONSTANT;
        operation[i].length = i * 5 + 10;
        ACMLIST_INSERT_TAIL(&stream.operations, &operation[i], entry);
    }
    operation[4].opcode = INSERT;
    operation[8].opcode = READ;

    // execute test
    result = stream_sum_const_buffer(&stream);
    TEST_ASSERT_EQUAL(245, result);
}

void test_stream_sum_const_buffer_null(void) {
    int result;

    result = stream_sum_const_buffer(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_stream_check_periods(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct schedule_entry schedule[8];
    memset(&schedule, 0, sizeof (schedule));
    int result, i;
    // set test preconditions
    // prepare windows list of stream
    ACMLIST_INIT(&stream.windows);
    for (i = 0; i < 8; i++) {
        schedule[i].period_ns = 500 * 1;
        ACMLIST_INSERT_TAIL(&stream.windows, &schedule[i], entry);
    }

    // execute test
    result = stream_check_periods(&stream, 4000, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_stream_check_periods_not_okay(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct schedule_entry schedule[8];
    memset(&schedule, 0, sizeof (schedule));
    int result, i;
    // set test preconditions
    // prepare windows list of stream
    ACMLIST_INIT(&stream.windows);
    for (i = 0; i < 8; i++) {
        schedule[i].period_ns = 500 * 1;
        ACMLIST_INSERT_TAIL(&stream.windows, &schedule[i], entry);
    }
    schedule[5].period_ns = 700;

    // execute test
    logging_Expect(0, "Validate: stream schedule period not compatible to module period");
    result = stream_check_periods(&stream, 4000, false);
    TEST_ASSERT_EQUAL(-EACMINCOMPATIBLEPERIOD, result);
}

void test_stream_check_periods_neg_zero(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct schedule_entry schedule[8];
    memset(&schedule, 0, sizeof (schedule));
    int result, i;
    // set test preconditions
    // prepare windows list of stream
    ACMLIST_INIT(&stream.windows);
    for (i = 0; i < 8; i++) {
        schedule[i].period_ns = 500 * 1;
        ACMLIST_INSERT_TAIL(&stream.windows, &schedule[i], entry);
    }
    schedule[5].period_ns = 0;

    // execute test
    logging_Expect(0, "Validate: Period of window has value 0");
    result = stream_check_periods(&stream, 4000, false);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_stream_check_periods_null(void) {
    int result;

    result = stream_check_periods(NULL, 5000, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_check_module_scheduling_gaps(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct fsc_command fsc_item[10];
    memset(&fsc_item, 0, sizeof (fsc_item));
    int result, i;

    // set test preconditions
    // prepare fsc list of module
    ACMLIST_INIT(&module.fsc_list);
    for (i = 0; i < 10; i++) {
        fsc_item[i].hw_schedule_item.abs_cycle = (i + 1) * ANZ_MIN_TICKS;
        ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[i], entry);
    }

    // execute test
    result = check_module_scheduling_gaps(&module);
    TEST_ASSERT_EQUAL(0, result);
}

void test_check_module_scheduling_gaps_zero(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct fsc_command fsc_item[10];
    memset(&fsc_item, 0, sizeof (fsc_item));
    int result, i;

    // set test preconditions
    // prepare fsc list of module
    ACMLIST_INIT(&module.fsc_list);
    for (i = 0; i < 10; i++) {
        fsc_item[i].hw_schedule_item.abs_cycle = i * ANZ_MIN_TICKS;
        ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[i], entry);
    }

    // execute test
    result = check_module_scheduling_gaps(&module);
    TEST_ASSERT_EQUAL(0, result);
}

void test_check_module_scheduling_gaps_not_okay_later(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct fsc_command fsc_item[10];
    memset(&fsc_item, 0, sizeof (fsc_item));
    int result, i;

    // set test preconditions
    // prepare fsc list of module
    ACMLIST_INIT(&module.fsc_list);
    for (i = 0; i < 10; i++) {
        fsc_item[i].hw_schedule_item.abs_cycle = (i + 1) * ANZ_MIN_TICKS;
        ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[i], entry);
    }
    fsc_item[2].hw_schedule_item.abs_cycle = 20;

    // execute test
    logging_Expect(0, "Validate: scheduling gap: %d; cycle times: %d, %d");
    result = check_module_scheduling_gaps(&module);
    TEST_ASSERT_EQUAL(-EACMSCHEDTIME, result);
}

void test_check_module_scheduling_gaps_not_okay_first(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct fsc_command fsc_item[10];
    memset(&fsc_item, 0, sizeof (fsc_item));
    int result, i;

    // set test preconditions
    // prepare fsc list of module
    ACMLIST_INIT(&module.fsc_list);
    for (i = 0; i < 10; i++) {
        fsc_item[i].hw_schedule_item.abs_cycle = (i + 1) * ANZ_MIN_TICKS;
        ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[i], entry);
    }
    fsc_item[0].hw_schedule_item.abs_cycle = 7;

    // execute test
    logging_Expect(0, "Validate: scheduling gap: %d; cycle times: %d, %d");
    result = check_module_scheduling_gaps(&module);
    TEST_ASSERT_EQUAL(-EACMSCHEDTIME, result);
}

void test_check_module_scheduling_gaps_not_okay_equal_times(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct fsc_command fsc_item[10];
    memset(&fsc_item, 0, sizeof (fsc_item));
    int result, i;

    // set test preconditions
    // prepare fsc list of module
    ACMLIST_INIT(&module.fsc_list);
    for (i = 0; i < 10; i++) {
        fsc_item[i].hw_schedule_item.abs_cycle = (i + 1) * ANZ_MIN_TICKS;
        ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[i], entry);
    }
    fsc_item[9].hw_schedule_item.abs_cycle = 72;

    // execute test
    logging_Expect(0, "Validate: scheduling gap: %d; cycle times: %d, %d");
    result = check_module_scheduling_gaps(&module);
    TEST_ASSERT_EQUAL(-EACMSCHEDTIME, result);
}

void test_check_module_scheduling_gaps_null(void) {
    int result;

    logging_Expect(3, "Validate: no module got in %s");
    result = check_module_scheduling_gaps(NULL);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, result);
}

void test_check_module_op_exists(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[4];
    memset(&stream, 0, sizeof (stream));
    struct operation operation[10];
    memset(&operation, 0, sizeof (operation));
    bool result;
    int i;

    // set test preconditions
    // prepare streams list of module
    ACMLIST_INIT(&module.streams);
    for (i = 0; i < 4; i++) {
        // prepare operations list of streams
        ACMLIST_INIT(&stream[i].operations);
        // add stream to module
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);
    }
    stream[0].type = TIME_TRIGGERED_STREAM;
    stream[1].type = REDUNDANT_STREAM_TX;
    stream[2].type = INGRESS_TRIGGERED_STREAM;
    stream[3].type = EVENT_STREAM;
    stream[2].reference = &stream[3];
    // insert operations into streams
    for (i = 0; i < 3; i++) {
        ACMLIST_INSERT_TAIL(&stream[0].operations, &operation[i], entry);
    }
    ACMLIST_INSERT_TAIL(&stream[1].operations, &operation[3], entry);
    // no operation in Ingress Triggered Stream
    for (i = 4; i < 10; i++) {
        ACMLIST_INSERT_TAIL(&stream[3].operations, &operation[i], entry);
    }

    // execute test
    result = check_module_op_exists(&module);
    TEST_ASSERT_TRUE(result);
}

void test_check_module_op_exists_not_okay_ingress_triggered(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[4];
    memset(&stream, 0, sizeof (stream));
    struct operation operation[10];
    memset(&operation, 0, sizeof (operation));
    bool result;
    int i;

    // set test preconditions
    // prepare streams list of module
    ACMLIST_INIT(&module.streams);
    for (i = 0; i < 4; i++) {
        // prepare operations list of streams
        ACMLIST_INIT(&stream[i].operations);
        // add stream to module
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);
    }
    stream[0].type = TIME_TRIGGERED_STREAM;
    stream[1].type = REDUNDANT_STREAM_TX;
    stream[2].type = INGRESS_TRIGGERED_STREAM;
    stream[3].type = EVENT_STREAM;
    // don't set the reference field of ingress triggered stream!
    stream[2].reference = NULL;
    // insert operations into streams
    for (i = 0; i < 3; i++)
        ACMLIST_INSERT_TAIL(&stream[0].operations, &operation[i], entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &operation[3], entry);
    // no operation in Ingress Triggered Stream
    for (i = 4; i < 10; i++)
        ACMLIST_INSERT_TAIL(&stream[3].operations, &operation[i], entry);

    // execute test
    result = check_module_op_exists(&module);
    TEST_ASSERT_FALSE(result);
}

void test_check_module_op_exists_not_okay_noops(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[4];
    memset(&stream, 0, sizeof (stream));
    struct operation operation[10];
    memset(&operation, 0, sizeof (operation));
    bool result;
    int i;

    // set test preconditions
    // prepare streams list of module
    ACMLIST_INIT(&module.streams);
    for (i = 0; i < 4; i++) {
        // prepare operations list of streams
        ACMLIST_INIT(&stream[i].operations);
        // add stream to module
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);
    }
    stream[0].type = TIME_TRIGGERED_STREAM;
    stream[1].type = REDUNDANT_STREAM_TX;
    stream[2].type = INGRESS_TRIGGERED_STREAM;
    stream[3].type = EVENT_STREAM;
    // don't set the reference field of ingress triggered stream!
    stream[2].reference = &stream[3];
    // insert operations into streams
    for (i = 0; i < 3; i++)
        ACMLIST_INSERT_TAIL(&stream[0].operations, &operation[i], entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &operation[3], entry);
    // no operation in Ingress Triggered Stream but reference exists
    // no operation in stream[3]	}

    // execute test
    result = check_module_op_exists(&module);
    TEST_ASSERT_FALSE(result);
}

void test_check_module_op_exists_null(void) {
    bool result;

    result = check_module_op_exists(NULL);
    TEST_ASSERT_TRUE(result);
}

void test_calc_stream_egress_framesize(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation[7];
    memset(&operation, 0, sizeof (operation));
    int result, i;

    // set test preconditions
    // prepare operations list of streams
    ACMLIST_INIT(&stream.operations);
    // prepare operations
    operation[0].opcode = INSERT_CONSTANT;
    operation[1].opcode = FORWARD;
    operation[2].opcode = INSERT_CONSTANT;
    operation[3].opcode = FORWARD_ALL;
    operation[4].opcode = PAD;
    operation[5].opcode = INSERT;
    operation[6].opcode = READ;
    // insert operations into streams
    for (i = 0; i < 7; i++) {
        operation[i].length = i * 50 + 17;
        ACMLIST_INSERT_TAIL(&stream.operations, &operation[i], entry);
    }

    // execute test
    result = calc_stream_egress_framesize(&stream);
    TEST_ASSERT_EQUAL(685, result);
}

void test_calc_stream_egress_framesize_null(void) {
    int result;

    result = calc_stream_egress_framesize(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_config_final_missing_module(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    int result;

    /* a configuration without any configured module is fine */
    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&config, 0);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);
    result = validate_config(&config, true);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_config_final_error_validate_module(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;

    // prepare test
    config.bypass[0] = &module;

    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    logging_Expect(0, "Validate: module period equal zero");
    result = validate_config(&config, true);
    TEST_ASSERT_EQUAL(-EACMMODCYCLE, result);
}

void test_validate_config_final_missing_operation(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_0, &config),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_1, &config)
    };
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    // prepare test
    config.bypass[0] = &module[0];
    config.bypass[1] = &module[1];
    module[0].cycle_ns = 5000;
    module[1].cycle_ns = 5000;
    // prepare streams and add them to module
    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream, entry);

    // execute test
    stream_num_operations_x_ExpectAndReturn(&stream, INSERT, 0);
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module[0].fsc_list, 0);
    stream_num_prefetch_ops_ExpectAndReturn(&stream, 0);
    stream_num_gather_ops_ExpectAndReturn(&stream, 0);
    stream_num_scatter_ops_ExpectAndReturn(&stream, 0);
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module[1].fsc_list, 0);
    logging_Expect(0, "Validate: stream without operation");
    result = validate_config(&config, true);
    TEST_ASSERT_EQUAL(-EACMOPMISSING, result);
}

void test_validate_config_final_problem_egress_framesize(void){
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_0, &config),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_1, &config)
    };
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    uint8_t data[20] = { 0 };
    struct operation operation = INSERT_CONSTANT_OPERATION_INITIALIZER(data, sizeof (data));

    // prepare test
    config.bypass[0] = &module[0];
    config.bypass[1] = &module[1];
    module[0].cycle_ns = 5000;
    module[1].cycle_ns = 5000;
    // prepare operation
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);
    // prepare stream and add them to module
    ACMLIST_INSERT_TAIL(&module[0].streams, &stream, entry);

    // execute test
    logging_Expect(0, "Validate: frame size of egress operations < %d");
    result = validate_config(&config, true);
    TEST_ASSERT_EQUAL(-EACMEGRESSFRAMESIZE, result);
}

void test_validate_config_final_egress_framesize_ok(void){
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_0, &config),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, SPEED_1GBps, MODULE_1, &config)
    };
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    uint8_t data[65] = { 0 };
    struct operation operation = INSERT_CONSTANT_OPERATION_INITIALIZER(data, sizeof(data));

    // prepare test
    config.bypass[0] = &module[0];
    config.bypass[1] = &module[1];
    module[0].cycle_ns = 5000;
    module[1].cycle_ns = 5000;
    // prepare operation
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);
    // prepare stream and add them to module
    ACMLIST_INSERT_TAIL(&module[0].streams, &stream, entry);

    // execute test
    stream_num_operations_x_ExpectAndReturn(&stream, INSERT, 0);
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module[0].fsc_list, 0);
    stream_num_prefetch_ops_ExpectAndReturn(&stream, 1);
    stream_num_gather_ops_ExpectAndReturn(&stream, 1);
    stream_num_scatter_ops_ExpectAndReturn(&stream, 0);
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module[1].fsc_list, 0);
    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&config, 0);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);

    result = validate_config(&config, true);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_config_non_final(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    int result;

    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&config, 0);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);

    result = validate_config(&config, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_config_neg_recalc_buffers(void){
    struct acm_config config;
    memset(&config, 0, sizeof(config));
    int result;

    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&config, -ENODEV);

    result = validate_config(&config, false);
    TEST_ASSERT_EQUAL(-ENODEV, result);
}

void test_validate_config_neg_number_buffers(void){
    struct acm_config config;
    memset(&config, 0, sizeof(config));
    int result;

    /* prepare test case */
    config.msg_buffs.num = 45;

    /* execute test case */
    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&config, 0);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);
    logging_Expect(LOGLEVEL_ERR, "Validate: too many message buffers: %d");

    result = validate_config(&config, false);
    TEST_ASSERT_EQUAL(-EACMNUMMESSBUFF, result);
}

void test_validate_module_final(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;

    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    logging_Expect(0, "Validate: module period equal zero");
    result = validate_module(&module, true);
    TEST_ASSERT_EQUAL(-EACMMODCYCLE, result);
}

void test_validate_module_null(void) {
    int result;

    logging_Expect(0, "Validate: no module as input");
    result = validate_module(NULL, true);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_validate_module_non_final(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;

    module.cycle_ns = 2000;

    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_module_buffer_size_too_long(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    // set test preconditions
    module.cycle_ns = 2000;

    // prepare streamlist
    ACMLIST_INIT(&module.streams);
    // prepare stream and add it to module
    ACMLIST_INIT(&stream.operations);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    // prepare operation
    operation.opcode = INSERT_CONSTANT;
    operation.length = 5000;
    // insert operations into streams
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);

    // execute test
    logging_Expect(0, "Validate: constant message buffer %d too long");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMCONSTBUFFER, result);
}

void test_validate_module_too_many_redundand_streams(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[32];
    memset(&stream, 0, sizeof (stream));
    int result, i;

    // set test preconditions
    module.cycle_ns = 2000;

    // prepare streamlist
    ACMLIST_INIT(&module.streams);
    // prepare stream and add it to module
    for (i = 0; i < 32; i++) {
        stream[i].type = REDUNDANT_STREAM_TX;
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);
    }

    // execute test
    logging_Expect(0, "Validate: too many redundant streams: %d");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMREDUNDANDSTREAMS, result);
}

void test_validate_module_too_many_fsc_schedules_1(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;
    struct fsc_command fsc_item;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare fsclist
    ACMLIST_INIT(&module.fsc_list);
    ACMLIST_COUNT(&module.fsc_list) = 1025;
    fsc_item.hw_schedule_item.abs_cycle = 0;
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item, entry);

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    logging_Expect(0, "Validate: too many schedule events: %d");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMSCHEDULEEVENTS, result);
}

void test_validate_module_too_many_fsc_schedules_2(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;
    struct fsc_command fsc_item;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare fsclist
    ACMLIST_INIT(&module.fsc_list);
    ACMLIST_COUNT(&module.fsc_list) = 1024;
    fsc_item.hw_schedule_item.abs_cycle = 100;
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item, entry);

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    logging_Expect(0, "Validate: too many schedule events: %d");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMSCHEDULEEVENTS, result);
}

void test_validate_module_schedule_period_problem(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct schedule_entry schedule;
    memset(&schedule, 0, sizeof (schedule));
    int result;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare streamlist
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    // prepare schedule list
    ACMLIST_INIT(&stream.windows);
    ACMLIST_INSERT_TAIL(&stream.windows, &schedule, entry);
    //prepare schedule
    schedule.period_ns = 700;

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    logging_Expect(0, "Validate: stream schedule period not compatible to module period");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMINCOMPATIBLEPERIOD, result);
}

void test_validate_module_schedule_gap_too_short(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct schedule_entry schedule;
    memset(&schedule, 0, sizeof (schedule));
    struct fsc_command fsc_item[2];
    memset(&fsc_item, 0, sizeof (fsc_item));
    int result;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare streamlist
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    // prepare schedule list
    ACMLIST_INIT(&stream.windows);
    ACMLIST_INSERT_TAIL(&stream.windows, &schedule, entry);
    //prepare schedule
    schedule.period_ns = 2000;
    schedule.time_start_ns = 700;
    schedule.time_end_ns = 900;
    // prepare fsc list and items
    ACMLIST_INIT(&module.fsc_list);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[0], entry);
    ACMLIST_INSERT_TAIL(&module.fsc_list, &fsc_item[1], entry);
    fsc_item[0].hw_schedule_item.abs_cycle = 9;
    fsc_item[1].hw_schedule_item.abs_cycle = 11;

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    logging_Expect(0, "Validate: scheduling gap: %d; cycle times: %d, %d");
    logging_Expect(0, "Validate: scheduling gap too short");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMSCHEDTIME, result);
}

void test_validate_module_too_many_egressops(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[5];
    memset(&stream, 0, sizeof (stream));
    struct operation operation[260];
    memset(&operation, 0, sizeof (operation));
    int result, i;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare streamlist
    ACMLIST_INIT(&module.streams);
    for (i = 0; i < 5; i++) {
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);
        // prepare operations list
        ACMLIST_INIT(&stream[i].operations);
    }
    //prepare operations
    for (i = 0; i < 52; i++) {
        operation[i].opcode = INSERT;
        ACMLIST_INSERT_TAIL(&stream[0].operations, &operation[i], entry);
    }
    for (i = 52; i < 104; i++) {
        operation[i].opcode = INSERT_CONSTANT;
        ACMLIST_INSERT_TAIL(&stream[1].operations, &operation[i], entry);
    }
    for (i = 104; i < 156; i++) {
        operation[i].opcode = PAD;
        ACMLIST_INSERT_TAIL(&stream[2].operations, &operation[i], entry);
    }
    for (i = 156; i < 208; i++) {
        operation[i].opcode = FORWARD;
        ACMLIST_INSERT_TAIL(&stream[3].operations, &operation[i], entry);
    }
    for (i = 208; i < 260; i++) {
        operation[i].opcode = FORWARD_ALL;
        ACMLIST_INSERT_TAIL(&stream[4].operations, &operation[i], entry);
    }

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    for (i = 0; i < 5; i++) {
        stream_num_prefetch_ops_ExpectAndReturn(&stream[i], 1);
        stream_num_gather_ops_ExpectAndReturn(&stream[i], 52);
    }

    logging_Expect(0, "Validate: too many egress operations: %d");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMEGRESSOPERATIONS, result);
}

void test_validate_module_too_many_ingressops(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation[257];
    memset(&operation, 0, sizeof (operation));
    int result, i;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare streamlist
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);
    // prepare operations list
    ACMLIST_INIT(&stream.operations);
    //prepare operations
    for (i = 0; i < 257; i++) {
        operation[i].opcode = READ;
        ACMLIST_INSERT_TAIL(&stream.operations, &operation[i], entry);
    }

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    stream_num_prefetch_ops_ExpectAndReturn(&stream, 1);
    stream_num_gather_ops_ExpectAndReturn(&stream, 0);
    stream_num_scatter_ops_ExpectAndReturn(&stream, 257);
    logging_Expect(0, "Validate: too many ingress operations: %d");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMINGRESSOPERATIONS, result);
}

void test_validate_module_too_many_lookup_entries(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[17];
    memset(&stream, 0, sizeof (stream));
    int result, i;

    // set test preconditions
    module.cycle_ns = 2000;
    //prepare streamlist
    ACMLIST_INIT(&module.streams);
    // prepare streams
    for (i = 0; i < 17; i++) {
        stream[i].type = INGRESS_TRIGGERED_STREAM;
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);
    }
    stream[9].type = REDUNDANT_STREAM_RX;

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    for (i = 0; i < 17; i++) {
        stream_num_prefetch_ops_ExpectAndReturn(&stream[i], 0);
        stream_num_gather_ops_ExpectAndReturn(&stream[i], 0);
    }
    for (i = 0; i < 17; i++) {
        stream_num_scatter_ops_ExpectAndReturn(&stream[i], 0);
    }
    logging_Expect(0, "Validate: too many lookup entries: %d");
    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(-EACMLOOKUPENTRIES, result);
}

void test_validate_module_nonfinal_configref(void) {
    int result;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            &configuration);

    // set test preconditions
    module.cycle_ns = 2000;
    configuration.bypass[0] = &module;

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&configuration, 0);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);

    result = validate_module(&module, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_stream_non_final(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    // execute test
    stream_num_operations_x_ExpectAndReturn(&stream, INSERT, 0);
    result = validate_stream(&stream, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_stream_neg_null(void) {
    int result;

    // execute test
    logging_Expect(LOGLEVEL_ERR, "Validate: no stream as input");
    result = validate_stream(NULL, false);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_validate_stream_neg_payload(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct operation operation = FORWARD_OPERATION_INITIALIZER(20, 15);
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);

    // execute test
    logging_Expect(LOGLEVEL_ERR, "Validate: forward operation truncates too many bytes: %d");
    result = validate_stream(&stream, false);
    TEST_ASSERT_EQUAL(-EACMFWDOFFSET, result);
}
void test_validate_stream_neg_num_insert_ops(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    // execute test
    stream_num_operations_x_ExpectAndReturn(&stream, INSERT, 9);
    result = validate_stream(&stream, false);
    TEST_ASSERT_EQUAL(-EACMNUMINSERT, result);
}

void test_validate_stream_redundant_streams(void) {
    int result;
    struct acm_stream stream1 = STREAM_INITIALIZER(stream1, REDUNDANT_STREAM_RX);
    struct acm_stream stream2 = STREAM_INITIALIZER(stream2, REDUNDANT_STREAM_RX);
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module1 =
            MODULE_INITIALIZER(module1, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &configuration);
    struct acm_module module2 =
            MODULE_INITIALIZER(module2, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_1, &configuration);

    /* prepare test case */
    stream1.reference_redundant = &stream2;
    stream2.reference_redundant = &stream1;
    configuration.bypass[0] = &module1;
    configuration.bypass[1] = &module2;
    module1.cycle_ns = 2000;
    module2.cycle_ns = 1000;
    ACMLIST_INSERT_TAIL(&module1.streams, &stream1, entry);
    ACMLIST_INSERT_TAIL(&module2.streams, &stream2, entry);

    /* execute test case */
    stream_num_operations_x_ExpectAndReturn(&stream1, INSERT, 0);
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module1.fsc_list, 0);
    stream_num_prefetch_ops_ExpectAndReturn(&stream1, 0);
    stream_num_gather_ops_ExpectAndReturn(&stream1, 0);
    stream_num_scatter_ops_ExpectAndReturn(&stream1, 0);
    clean_and_recalculate_hw_msg_buffs_ExpectAndReturn(&configuration, 0);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_COUNT), 32);
    result = validate_stream(&stream1, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_validate_stream_neg_red_streams_same_module(void) {
    int result;
    struct acm_stream stream1 = STREAM_INITIALIZER(stream1, REDUNDANT_STREAM_RX);
    struct acm_stream stream2 = STREAM_INITIALIZER(stream2, REDUNDANT_STREAM_RX);
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module1 =
            MODULE_INITIALIZER(module1, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &configuration);

    /* prepare test case */
    stream1.reference_redundant = &stream2;
    stream2.reference_redundant = &stream1;
//    configuration.bypass[0] = &module1; removed due to cpp check, but logically senseful
    ACMLIST_INSERT_TAIL(&module1.streams, &stream1, entry);
    ACMLIST_INSERT_TAIL(&module1.streams, &stream2, entry);

    /* execute test case */
    logging_Expect(LOGLEVEL_ERR, "Validate: two redundant streams are added to same module: %d");
    result = validate_stream(&stream1, false);
    TEST_ASSERT_EQUAL(-EACMREDSAMEMOD, result);
}

void test_validate_stream_neg_red_streams_diff_config(void) {
    int result;
    struct acm_stream stream1 = STREAM_INITIALIZER(stream1, REDUNDANT_STREAM_TX);
    struct acm_stream stream2 = STREAM_INITIALIZER(stream2, REDUNDANT_STREAM_TX);
    struct acm_config configuration1 = CONFIGURATION_INITIALIZER(configuration1, false);
    struct acm_config configuration2 = CONFIGURATION_INITIALIZER(configuration2, false);
    struct acm_module module1 =
            MODULE_INITIALIZER(module1, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &configuration1);
    struct acm_module module2 =
            MODULE_INITIALIZER(module2, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_1, &configuration2);

    /* prepare test case */
    stream1.reference_redundant = &stream2;
    stream2.reference_redundant = &stream1;
//    configuration1.bypass[0] = &module1; removed due to cpp check, but logically senseful
//    configuration2.bypass[1] = &module2; removed due to cpp check, but logically senseful
    ACMLIST_INSERT_TAIL(&module1.streams, &stream1, entry);
    ACMLIST_INSERT_TAIL(&module2.streams, &stream2, entry);

    /* execute test case */
    logging_Expect(LOGLEVEL_ERR, "Validate: two redundant streams are added to different configurations");
    result = validate_stream(&stream1, false);
    TEST_ASSERT_EQUAL(-EACMDIFFCONFIG, result);
}

void test_validate_stream_neg_red_streams_final_no_config(void) {
    int result;
    struct acm_stream stream1 = STREAM_INITIALIZER(stream1, REDUNDANT_STREAM_RX);
    struct acm_stream stream2 = STREAM_INITIALIZER(stream2, REDUNDANT_STREAM_RX);
    struct acm_config configuration1 = CONFIGURATION_INITIALIZER(configuration1, false);
    struct acm_module module1 =
            MODULE_INITIALIZER(module1, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &configuration1);
    struct acm_module module2 =
            MODULE_INITIALIZER(module2, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_1, NULL);

    /* prepare test case */
    stream1.reference_redundant = &stream2;
    stream2.reference_redundant = &stream1;
//    configuration1.bypass[0] = &module1; removed due to cpp check, but logically senseful
    ACMLIST_INSERT_TAIL(&module1.streams, &stream1, entry);
    ACMLIST_INSERT_TAIL(&module2.streams, &stream2, entry);

    /* execute test case */
    logging_Expect(LOGLEVEL_ERR, "Validate: stream not added to configuration");
    result = validate_stream(&stream1, true);
    TEST_ASSERT_EQUAL(-EACMSTREAMCONFIG, result);
}

void test_validate_stream_list_non_final(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;

    // set test preconditions
    module.cycle_ns = 2000;
    // prepare stream list
    ACMLIST_INIT(&module.streams);

    // execute test
    calc_nop_schedules_for_long_cycles_ExpectAndReturn(&module.fsc_list, 0);
    result = validate_stream_list(&module.streams, false);
    TEST_ASSERT_EQUAL(0, result);
}

void test_rx_buffername_unique_empty_list(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    struct sysfs_buffer msg_buf;
    memset(&msg_buf, 0, sizeof (msg_buf));
    char name[] = "first_buffer";
    int result;
    struct sysfs_buffer *reusable;
    bool reuse_flag = false;

    /*prepare testcase */
    msg_buf.msg_buff_name = name;
    ACMLIST_INIT(&config.msg_buffs);

    /*execute testcase */
    result = buffername_check(&config.msg_buffs, &msg_buf, &reuse_flag, &reusable);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FALSE(reuse_flag);
    TEST_ASSERT_NULL(reusable);
}

void test_rx_buffername_unique_yes(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    struct sysfs_buffer msg_buf, list_item1, list_item2;
    memset(&msg_buf, 0, sizeof (msg_buf));
    memset(&list_item1, 0, sizeof (list_item1));
    memset(&list_item2, 0, sizeof (list_item2));
    char name[] = "buffer";
    char name_item1[] = "buffer_1";
    char name_item2[] = "buffer_2";
    int result;
    struct sysfs_buffer *reusable;
    bool reuse_flag = false;

    /*prepare testcase */
    msg_buf.msg_buff_name = name;
    list_item1.msg_buff_name = name_item1;
    list_item2.msg_buff_name = name_item2;
    ACMLIST_INIT(&config.msg_buffs);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item1, entry);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item2, entry);

    /*execute testcase */
    result = buffername_check(&config.msg_buffs, &msg_buf, &reuse_flag, &reusable);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FALSE(reuse_flag);
    TEST_ASSERT_NULL(reusable);
}

void test_rx_buffername_unique_no_equal_tx(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    struct sysfs_buffer msg_buf, list_item1, list_item2;
    memset(&msg_buf, 0, sizeof (msg_buf));
    memset(&list_item1, 0, sizeof (list_item1));
    memset(&list_item2, 0, sizeof (list_item2));
    char name[] = "buffer_2";
    char name_item1[] = "buffer_1";
    char name_item2[] = "buffer_2";
    int result;
    struct sysfs_buffer *reusable;
    bool reuse_flag = false;

    /*prepare testcase */
    msg_buf.msg_buff_name = name;
    list_item1.msg_buff_name = name_item1;
    list_item2.msg_buff_name = name_item2;
    list_item2.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
    ACMLIST_INIT(&config.msg_buffs);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item1, entry);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item2, entry);

    /*execute testcase */
    logging_Expect(0, "Validate: buffername %s equal but stream direction different");
    result = buffername_check(&config.msg_buffs, &msg_buf, &reuse_flag, &reusable);
    TEST_ASSERT_EQUAL(-EPERM, result);
}

void test_rx_buffername_unique_zero_length_buffername(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    struct sysfs_buffer msg_buf, list_item1, list_item2;
    memset(&msg_buf, 0, sizeof (msg_buf));
    memset(&list_item1, 0, sizeof (list_item1));
    memset(&list_item2, 0, sizeof (list_item2));
    char name[] = "";
    char name_item1[] = "buffer_1";
    char name_item2[] = "buffer_2";
    int result;
    struct sysfs_buffer *reusable;
    bool reuse_flag = false;

    /*prepare testcase */
    msg_buf.msg_buff_name = name;
    list_item1.msg_buff_name = name_item1;
    list_item2.msg_buff_name = name_item2;
    ACMLIST_INIT(&config.msg_buffs);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item1, entry);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item2, entry);

    /*execute testcase */
    result = buffername_check(&config.msg_buffs, &msg_buf, &reuse_flag, &reusable);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_FALSE(reuse_flag);
    TEST_ASSERT_NULL(reusable);
}

void test_rx_buffername_unique_no_equal_rx(void) {
    struct acm_config config;
    memset(&config, 0, sizeof (config));
    struct sysfs_buffer msg_buf, list_item1, list_item2;
    memset(&msg_buf, 0, sizeof (msg_buf));
    memset(&list_item1, 0, sizeof (list_item1));
    memset(&list_item2, 0, sizeof (list_item2));
    char name[] = "buffer_2";
    char name_item1[] = "buffer_1";
    char name_item2[] = "buffer_2";
    int result;
    struct sysfs_buffer *reusable;
    bool reuse_flag = false;

    /*prepare testcase */
    msg_buf.msg_buff_name = name;
    list_item1.msg_buff_name = name_item1;
    list_item2.msg_buff_name = name_item2;
    list_item1.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
    list_item2.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
    ACMLIST_INIT(&config.msg_buffs);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item1, entry);
    ACMLIST_INSERT_TAIL(&config.msg_buffs, &list_item2, entry);

    /*execute testcase */
    result = buffername_check(&config.msg_buffs, &msg_buf, &reuse_flag, &reusable);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_TRUE(reuse_flag);
    TEST_ASSERT_EQUAL_PTR(&list_item2, reusable);
}

void test_check_stream_payload_null(void) {
    int result;

    result = check_stream_payload(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_check_stream_payload_read_op(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    char buffer_name[] = "read_buffer";
    struct operation operation = READ_OPERATION_INITIALIZER(5, 10, &buffer_name[0]);
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);

    result = check_stream_payload(&stream);
    TEST_ASSERT_EQUAL(0, result);
}

void test_check_stream_payload_forward_op(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct operation operation = FORWARD_OPERATION_INITIALIZER(10, 20);
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);

    result = check_stream_payload(&stream);
    TEST_ASSERT_EQUAL(0, result);
}

void test_check_stream_payload_forward_op_high_offset(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct operation operation = FORWARD_OPERATION_INITIALIZER(20, 15);
    ACMLIST_INSERT_TAIL(&stream.operations, &operation, entry);

    logging_Expect(LOGLEVEL_ERR, "Validate: forward operation truncates too many bytes: %d");
    result = check_stream_payload(&stream);
    TEST_ASSERT_EQUAL(-EACMFWDOFFSET, result);
}

void test_check_stream_payload_neg_max_payload(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    char buffer_name1[] = "insert_buffer1";
    char buffer_name2[] = "insert_buffer2";
    struct sysfs_buffer msg_buf1, msg_buf2;
    struct operation operation1 = INSERT_OPERATION_INITIALIZER(1000, &buffer_name1[0], &msg_buf1);
    struct operation operation2 = INSERT_OPERATION_INITIALIZER(1000, &buffer_name2[0], &msg_buf2);
    ACMLIST_INSERT_TAIL(&stream.operations, &operation1, entry);
    ACMLIST_INSERT_TAIL(&stream.operations, &operation2, entry);

    logging_Expect(LOGLEVEL_ERR, "Validate: size of payload and header %d higher than maximum: %d");
    result = check_stream_payload(&stream);
    TEST_ASSERT_EQUAL(-EACMPAYLOAD, result);
}
