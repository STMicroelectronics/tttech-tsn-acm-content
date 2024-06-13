#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* Unity Test Framework */
#include "unity.h"

/* Module under test */
#include "libacmconfig.h"

/* directly added modules */

/* mock modules needed */
#include "mock_memory.h"
#include "mock_status.h"
#include "mock_operation.h"
#include "mock_stream.h"
#include "mock_module.h"
#include "mock_schedule.h"
#include "mock_config.h"
#include "mock_lookup.h"
#include "mock_validate.h"
#include "mock_logging.h"
#include "mock_sysfs.h"
#include "mock_tracing.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
    /* ignore all trace calls */
    trace_Ignore();
}

void tearDown(void)
{
}

void test_acm_read_status_item(void) {
    int64_t result;

    status_read_item_ExpectAndReturn(1, STATUS_DISABLE_OVERRUN_PREV_CYCLE, 25);
    result = acm_read_status_item(1, STATUS_DISABLE_OVERRUN_PREV_CYCLE);
    TEST_ASSERT_EQUAL_INT64(25, result);
}

void test_acm_read_config_identifier(void) {
    int64_t result;

    status_read_config_identifier_ExpectAndReturn(17);
    result = acm_read_config_identifier();
    TEST_ASSERT_EQUAL_INT64(17, result);
}

void test_acm_read_diagnostics(void) {
    struct acm_diagnostic *result;
    struct acm_diagnostic diagnostic_values;
    memset(&diagnostic_values, 0, sizeof (diagnostic_values));
    int i;

    diagnostic_values.timestamp.tv_sec = 1570150656;
    diagnostic_values.timestamp.tv_nsec = 0;
    diagnostic_values.scheduleCycleCounter = 0;
    diagnostic_values.txFramesCounter = 0;
    diagnostic_values.rxFramesCounter = 0;
    diagnostic_values.ingressWindowClosedFlags = 0;
    diagnostic_values.noFrameReceivedFlags = 0;
    diagnostic_values.recoveryFlags = 0;
    diagnostic_values.additionalFilterMismatchFlags = 0;
    for (i = 0; i < 16; i++) {
        diagnostic_values.ingressWindowClosedCounter[i] = 0;
        diagnostic_values.noFrameReceivedCounter[i] = 0;
        diagnostic_values.recoveryCounter[i] = 0;
        diagnostic_values.additionalFilterMismatchCounter[i] = 0;
    }
    status_read_diagnostics_ExpectAndReturn(0, &diagnostic_values);
    result = acm_read_diagnostics(0);
    TEST_ASSERT_EQUAL_MEMORY(&diagnostic_values, result, sizeof(struct acm_diagnostic));
}

void test_acm_set_diagnostics_poll_time(void) {
    int return_value;

    status_set_diagnostics_poll_time_ExpectAndReturn(1, 40, 0);
    return_value = acm_set_diagnostics_poll_time(1, 40);
    TEST_ASSERT_EQUAL_INT(0, return_value);
}

void test_acm_read_capability_item(void) {
    int32_t result;

    status_read_capability_item_ExpectAndReturn(CAP_CONFIG_READBACK, 1);
    result = acm_read_capability_item(CAP_CONFIG_READBACK);
    TEST_ASSERT_EQUAL_INT32(1, result);
}

void test_acm_read_lib_version(void) {
    char expected[] = "testversion";
    const char *result;

    result = acm_read_lib_version();
    TEST_ASSERT_EQUAL_STRING(expected, result);
}

void test_acm_read_buffer_locking_vector(void) {
    int64_t result;

    status_read_buffer_locking_vector_ExpectAndReturn(17);
    result = acm_read_buffer_locking_vector();
    TEST_ASSERT_EQUAL_INT64(17, result);
}

void test_acm_set_buffer_locking_mask(void) {
    int result;

    sysfs_write_buffer_control_mask_ExpectAndReturn(2777, "lock_msg_bufs", 0);
    result = acm_set_buffer_locking_mask(2777);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_set_buffer_unlocking_mask(void) {
    int result;

    sysfs_write_buffer_control_mask_ExpectAndReturn(11111, "unlock_msg_bufs", 0);
    result = acm_set_buffer_unlocking_mask(11111);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_insert(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char buffer[] = "Buffer12";
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insert_ExpectAndReturn(12, buffer, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    result = acm_add_stream_operation_insert(&stream, 12, buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_insert_ref(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char buffer[] = "Buffer12";
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    /* prepare test case */
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insert_ExpectAndReturn(12, buffer, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    calculate_gather_indizes_Expect(ACMLIST_REF(&stream, entry));
    result = acm_add_stream_operation_insert(&stream, 12, buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_insert_config_applied(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char buffer[] = "Buffer1";
    int result;

    stream_config_applied_ExpectAndReturn(&stream, true);
    logging_Expect(0, "Libacmconfig: configuration of stream already applied to HW");
    result = acm_add_stream_operation_insert(&stream, 8, buffer);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_operation_insert_operation_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char buffer[] = "Buffer1";
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insert_ExpectAndReturn(8, buffer, NULL);
    result = acm_add_stream_operation_insert(&stream, 8, buffer);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_operation_insert_op_no_add_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char buffer[] = "Buffer12";
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insert_ExpectAndReturn(7, buffer, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, -EINVAL);
    operation_destroy_Expect(&operation);
    result = acm_add_stream_operation_insert(&stream, 7, buffer);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_operation_insertconstant(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char content[] = "constantcontent";
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t size = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insertconstant_ExpectAndReturn(content, size, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    result = acm_add_stream_operation_insertconstant(&stream, content, size);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_insertconstant_config_applied(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char content[] = "constantcontent";
    uint16_t size = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, true);
    logging_Expect(0, "Libacmconfig: configuration of stream already applied to HW");
    result = acm_add_stream_operation_insertconstant(&stream, content, size);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_operation_insertconstant_operation_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char content[] = "constantcontent";
    uint16_t size = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insertconstant_ExpectAndReturn(content, size, NULL);
    result = acm_add_stream_operation_insertconstant(&stream, content, size);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_operation_insertconstant_op_no_add_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char content[] = "constantcontent";
    uint16_t size = 16;
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insertconstant_ExpectAndReturn(content, size, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, -EINVAL);
    operation_destroy_Expect(&operation);
    result = acm_add_stream_operation_insertconstant(&stream, content, size);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_operation_insertconstant_ref(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char content[] = "constantcontent";
    uint16_t size = 16;
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    /* prepare test case */
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_insertconstant_ExpectAndReturn(content, size, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    calculate_gather_indizes_Expect(ACMLIST_REF(&stream, entry));
    result = acm_add_stream_operation_insertconstant(&stream, content, size);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_pad(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char value = 'x';
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t length = 10;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_pad_ExpectAndReturn(length, value, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    result = acm_add_stream_operation_pad(&stream, length, value);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_pad_ref(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char value = 'x';
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t length = 10;
    int result;

    /* prepare test case */
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_pad_ExpectAndReturn(length, value, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    calculate_gather_indizes_Expect(ACMLIST_REF(&stream, entry));
    result = acm_add_stream_operation_pad(&stream, length, value);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_pad_config_applied(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char value = 'a';
    uint16_t length = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, true);
    logging_Expect(0, "Libacmconfig: configuration of stream already applied to HW");
    result = acm_add_stream_operation_pad(&stream, length, value);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_operation_pad_operation_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char value = 'a';
    uint16_t length = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_pad_ExpectAndReturn(length, value, NULL);
    result = acm_add_stream_operation_pad(&stream, length, value);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_operation_pad_op_no_add_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    const char value = 'V';
    uint16_t length = 32;
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_pad_ExpectAndReturn(length, value, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, -EINVAL);
    operation_destroy_Expect(&operation);
    result = acm_add_stream_operation_pad(&stream, length, value);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_operation_forward(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t length = 10;
    uint16_t offset = 20;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forward_ExpectAndReturn(offset, length, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    result = acm_add_stream_operation_forward(&stream, offset, length);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_forward_ref(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t length = 10;
    uint16_t offset = 20;
    int result;

    /* prepare test case */
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forward_ExpectAndReturn(offset, length, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    calculate_gather_indizes_Expect(ACMLIST_REF(&stream, entry));
    result = acm_add_stream_operation_forward(&stream, offset, length);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_forward_config_applied(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    uint16_t offset = 30;
    uint16_t length = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, true);
    logging_Expect(0, "Libacmconfig: configuration of stream already applied to HW");
    result = acm_add_stream_operation_forward(&stream, offset, length);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_operation_forward_operation_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    uint16_t offset = 30;
    uint16_t length = 16;
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forward_ExpectAndReturn(offset, length, NULL);
    result = acm_add_stream_operation_forward(&stream, offset, length);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_operation_forward_op_no_add_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    uint16_t offset = 40;
    uint16_t length = 32;
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forward_ExpectAndReturn(offset, length, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, -EINVAL);
    operation_destroy_Expect(&operation);
    result = acm_add_stream_operation_forward(&stream, offset, length);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_operation_read(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t length = 10;
    uint16_t offset = 20;
    const char buffer[] = "Buffername";
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_read_ExpectAndReturn(offset, length, buffer, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    result = acm_add_stream_operation_read(&stream, offset, length, buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_read_ref(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    uint16_t length = 10;
    uint16_t offset = 20;
    const char buffer[] = "Buffername";
    int result;

    /* prepare test case */
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_read_ExpectAndReturn(offset, length, buffer, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    calculate_scatter_indizes_Expect(ACMLIST_REF(&stream, entry));
    result = acm_add_stream_operation_read(&stream, offset, length, buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_read_config_applied(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    uint16_t offset = 30;
    uint16_t length = 16;
    const char buffer[] = "Buffername";
    int result;

    stream_config_applied_ExpectAndReturn(&stream, true);
    logging_Expect(0, "Libacmconfig: configuration of stream already applied to HW");
    result = acm_add_stream_operation_read(&stream, offset, length, buffer);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_operation_read_operation_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    uint16_t offset = 30;
    uint16_t length = 16;
    const char buffer[] = "Buffername";
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_read_ExpectAndReturn(offset, length, buffer, NULL);
    result = acm_add_stream_operation_read(&stream, offset, length, buffer);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_operation_read_op_no_add_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    uint16_t offset = 40;
    uint16_t length = 32;
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    const char buffer[] = "Buffername";
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_read_ExpectAndReturn(offset, length, buffer, &operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, -EINVAL);
    operation_destroy_Expect(&operation);
    result = acm_add_stream_operation_read(&stream, offset, length, buffer);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_operation_forwardall(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forwardall_ExpectAndReturn(&operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    result = acm_add_stream_operation_forwardall(&stream);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_forwardall_ref(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    /* prepare test case */
    ACMLIST_INIT(&module.streams);
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forwardall_ExpectAndReturn(&operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, 0);
    calculate_gather_indizes_Expect(ACMLIST_REF(&stream, entry));
    result = acm_add_stream_operation_forwardall(&stream);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_operation_forwardall_config_applied(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, true);
    logging_Expect(0, "Libacmconfig: configuration of stream already applied to HW");
    result = acm_add_stream_operation_forwardall(&stream);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_operation_forwardall_operation_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forwardall_ExpectAndReturn(NULL);
    result = acm_add_stream_operation_forwardall(&stream);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_operation_forwardall_op_no_add_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation operation;
    memset(&operation, 0, sizeof (operation));
    int result;

    stream_config_applied_ExpectAndReturn(&stream, false);
    operation_create_forwardall_ExpectAndReturn(&operation);
    stream_add_operation_ExpectAndReturn(&stream, &operation, -EINVAL);
    operation_destroy_Expect(&operation);
    result = acm_add_stream_operation_forwardall(&stream);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_clean_operations_null(void) {

    logging_Expect(0, "Libacmconfig: wrong parameter in %s");
    acm_clean_operations(NULL);
}

void test_acm_clean_operations_config_applied(void) {
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, true);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            0,
            MODULE_0,
            &configuration);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    /* prepare test case */
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    logging_Expect(0, "Libacmconfig: Config. already applied to HW.");
    acm_clean_operations(&stream);
}

void test_acm_clean_operations_config_applied_no(void) {
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            0,
            MODULE_0,
            &configuration);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    /* prepare test case */
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    operation_list_flush_Expect(&stream.operations);
    calculate_scatter_indizes_Expect(&module.streams);
    calculate_gather_indizes_Expect(&module.streams);
    acm_clean_operations(&stream);
}

void test_acm_clean_operations_ingress_stream(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct stream_list streamlist = STREAMLIST_INITIALIZER(streamlist);

    /* prepare test case */
    _ACMLIST_INSERT_TAIL(&streamlist, &stream, entry);

    /* execute test case */
    operation_list_flush_Expect(&stream.operations);
    calculate_scatter_indizes_Expect(&streamlist);
    calculate_gather_indizes_Expect(&streamlist);
    acm_clean_operations(&stream);
}

void test_acm_clean_operations_time_triggered_stream(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct stream_list streamlist = STREAMLIST_INITIALIZER(streamlist);

    /* prepare test case */
    _ACMLIST_INSERT_TAIL(&streamlist, &stream, entry);

    /* execute test case */
    operation_list_flush_user_Expect(&stream.operations);
    calculate_scatter_indizes_Expect(&streamlist);
    calculate_gather_indizes_Expect(&streamlist);
    acm_clean_operations(&stream);
}

void test_acm_clean_operations_event_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = EVENT_STREAM;
    operation_list_flush_user_Expect(& (stream.operations));
    acm_clean_operations(&stream);
}

void test_acm_clean_operations_recovery_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = RECOVERY_STREAM;
    operation_list_flush_user_Expect(& (stream.operations));
    acm_clean_operations(&stream);
}

void test_acm_clean_operations_redundant_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = REDUNDANT_STREAM_TX;
    operation_list_flush_user_Expect(& (stream.operations));
    acm_clean_operations(&stream);
}

void test_acm_add_stream_schedule_event(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t send_time = 40;
    uint32_t period = 60;

    schedule_create_ExpectAndReturn(0, 0, send_time, period, &schedule);
    schedule_list_add_schedule_ExpectAndReturn(&stream.windows, &schedule, 0);
    module_add_schedules_ExpectAndReturn(&stream, &schedule, 0);
    result = acm_add_stream_schedule_event(&stream, period, send_time);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_schedule_event_null(void) {
    int result;

    logging_Expect(0, "Libacmconfig: no stream as input");
    result = acm_add_stream_schedule_event(NULL, 80, 40);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_event_wrong_type(void) {
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = RECOVERY_STREAM;
    logging_Expect(0, "Invalid stream type");
    result = acm_add_stream_schedule_event(&stream, 120, 60);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_schedule_event_neg_send_time(void) {
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = REDUNDANT_STREAM_TX;
    logging_Expect(0, "Libacmconfig: send time not within period");
    result = acm_add_stream_schedule_event(&stream, 120, 190);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_event_neg_create(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);

    schedule_create_ExpectAndReturn(0, 0, 40, 60, NULL);
    result = acm_add_stream_schedule_event(&stream, 60, 40);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_schedule_event_neg_schedule_add_list(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    ;
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t send_time = 40;
    uint32_t period = 60;

    schedule_create_ExpectAndReturn(0, 0, send_time, period, &schedule);
    schedule_list_add_schedule_ExpectAndReturn(&stream.windows, &schedule, -EINVAL);
    result = acm_add_stream_schedule_event(&stream, 60, 40);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_event_neg_module_add_schedules(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t send_time = 40;
    uint32_t period = 60;

    schedule_create_ExpectAndReturn(0, 0, send_time, period, &schedule);
    schedule_list_add_schedule_ExpectAndReturn(&stream.windows, &schedule, 0);
    module_add_schedules_ExpectAndReturn(&stream, &schedule, -EFAULT);
    schedule_list_remove_schedule_Expect(&stream.windows, &schedule);
    result = acm_add_stream_schedule_event(&stream, 60, 40);
    TEST_ASSERT_EQUAL_INT(-EFAULT, result);
}

void test_acm_add_stream_schedule_window(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t start_time = 30;
    uint32_t end_time = 40;
    uint32_t period = 80;

    schedule_create_ExpectAndReturn(start_time, end_time, 0, period, &schedule);
    schedule_list_add_schedule_ExpectAndReturn(&stream.windows, &schedule, 0);
    module_add_schedules_ExpectAndReturn(&stream, &schedule, 0);
    result = acm_add_stream_schedule_window(&stream, period, start_time, end_time);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_stream_schedule_window_null(void) {
    int result;

    logging_Expect(0, "Libacmconfig: wrong parameter in %s");
    result = acm_add_stream_schedule_window(NULL, 40, 30, 40);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_window_neg_time_start_inkomp(void) {
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = INGRESS_TRIGGERED_STREAM;
    logging_Expect(0, "Libacmconfig: window start or window end not within period");
    result = acm_add_stream_schedule_window(&stream, 40, 50, 40);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_window_neg_time_end_inkomp(void) {
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = REDUNDANT_STREAM_RX;
    logging_Expect(0, "Libacmconfig: window start or window end not within period");
    result = acm_add_stream_schedule_window(&stream, 40, 20, 70);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_window_wrong_type(void) {
    int result;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));

    stream.type = RECOVERY_STREAM;
    logging_Expect(0, "Invalid stream type");
    result = acm_add_stream_schedule_window(&stream, 40, 30, 40);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_stream_schedule_window_neg_schedule_create(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    uint32_t start_time = 30;
    uint32_t end_time = 40;
    uint32_t period = 80;

    schedule_create_ExpectAndReturn(start_time, end_time, 0, period, NULL);
    result = acm_add_stream_schedule_window(&stream, period, start_time, end_time);
    TEST_ASSERT_EQUAL_INT(-ENOMEM, result);
}

void test_acm_add_stream_schedule_window_neg_schedule_add_list(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t start_time = 30;
    uint32_t end_time = 40;
    uint32_t period = 80;

    schedule_create_ExpectAndReturn(start_time, end_time, 0, period, &schedule);
    schedule_list_add_schedule_ExpectAndReturn(&stream.windows, &schedule, -EINVAL);
    result = acm_add_stream_schedule_window(&stream, period, start_time, end_time);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_stream_schedule_window_neg_module_add_schedules(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct schedule_entry schedule = SCHEDULE_ENTRY_INITIALIZER;
    uint32_t start_time = 30;
    uint32_t end_time = 40;
    uint32_t period = 80;

    schedule_create_ExpectAndReturn(start_time, end_time, 0, period, &schedule);
    schedule_list_add_schedule_ExpectAndReturn(&stream.windows, &schedule, 0);
    module_add_schedules_ExpectAndReturn(&stream, &schedule, -EFAULT);
    schedule_list_remove_schedule_Expect(&stream.windows, &schedule);
    result = acm_add_stream_schedule_window(&stream, 80, 30, 40);
    TEST_ASSERT_EQUAL_INT(-EFAULT, result);
}

void test_acm_clean_schedule(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    schedule_list_flush_Expect(&stream.windows);
    acm_clean_schedule(&stream);
}

void test_acm_clean_schedule_stream_added_to_module(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
    NULL);

    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    remove_schedule_sysfs_items_stream_Expect(&stream, &module);
    schedule_list_flush_Expect(&stream.windows);
    acm_clean_schedule(&stream);
}

void test_acm_clean_schedule_null(void) {
    logging_Expect(0, "Libacmconfig: wrong parameter in %s");
    acm_clean_schedule(NULL);
}

void test_acm_create_time_triggered_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(20, 3, 0);
    stream_create_ExpectAndReturn(TIME_TRIGGERED_STREAM, &stream);
    stream_set_egress_header_ExpectAndReturn(&stream, dmac, smac, 20, 3, 0);
    result = acm_create_time_triggered_stream(dmac, smac, 20, 3);
    TEST_ASSERT_EQUAL_INT(&stream, result);
}

void test_acm_create_time_triggered_stream_neg_vlan_par(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(20, 3, -EINVAL);
    result = acm_create_time_triggered_stream(dmac, smac, 20, 3);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_time_triggered_stream_no_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(20, 3, 0);
    stream_create_ExpectAndReturn(TIME_TRIGGERED_STREAM, NULL);
    result = acm_create_time_triggered_stream(dmac, smac, 20, 3);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_time_triggered_stream_no_egress_header(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(20, 3, 0);
    stream_create_ExpectAndReturn(TIME_TRIGGERED_STREAM, &stream);
    stream_set_egress_header_ExpectAndReturn(&stream, dmac, smac, 20, 3, -EINVAL);
    stream_destroy_Expect(&stream);
    result = acm_create_time_triggered_stream(dmac, smac, 20, 3);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_ingress_triggered_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    struct lookup lookup_item;
    uint8_t header[ACM_MAX_LOOKUP_SIZE] = "lookup";
    uint8_t header_mask[ACM_MAX_LOOKUP_SIZE] = "FFFFFF";
    uint8_t *additional_filter;
    uint8_t *additional_filter_mask;

    stream_create_ExpectAndReturn(INGRESS_TRIGGERED_STREAM, &stream);
    lookup_create_ExpectAndReturn(header,
            header_mask,
            additional_filter,
            additional_filter_mask,
            6,
            &lookup_item);
    result = acm_create_ingress_triggered_stream(header,
            header_mask,
            additional_filter,
            additional_filter_mask,
            6);
    TEST_ASSERT_EQUAL_INT(&stream, result);
}

void test_acm_create_ingress_triggered_stream_no_stream(void) {
    struct acm_stream *result;

    uint8_t header[ACM_MAX_LOOKUP_SIZE];
    uint8_t header_mask[ACM_MAX_LOOKUP_SIZE];
    uint8_t additional_filter;
    uint8_t additional_filter_mask;

    stream_create_ExpectAndReturn(INGRESS_TRIGGERED_STREAM, NULL);
    result = acm_create_ingress_triggered_stream(header,
            header_mask,
            &additional_filter,
            &additional_filter_mask,
            6);
    TEST_ASSERT_NULL(result);
}

void test_acm_create_ingress_triggered_stream_no_lookup(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    struct lookup lookup_item;
    memset(&lookup_item, 0, sizeof (lookup_item));
    uint8_t header[ACM_MAX_LOOKUP_SIZE] = "lookup";
    uint8_t header_mask[ACM_MAX_LOOKUP_SIZE] = "FFFFFF";
    uint8_t *additional_filter;
    uint8_t *additional_filter_mask;

    stream_create_ExpectAndReturn(INGRESS_TRIGGERED_STREAM, &stream);
    lookup_create_ExpectAndReturn(header,
            header_mask,
            additional_filter,
            additional_filter_mask,
            6,
            NULL);
    stream_destroy_Expect(&stream);
    result = acm_create_ingress_triggered_stream(header,
            header_mask,
            additional_filter,
            additional_filter_mask,
            6);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_recovery_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, 0);
    stream_create_ExpectAndReturn(RECOVERY_STREAM, &stream);
    stream_set_egress_header_ExpectAndReturn(&stream, dmac, smac, 15, 1, 0);
    result = acm_create_recovery_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(&stream, result);
}

void test_acm_create_recovery_stream_neg_vlan_par(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, -EINVAL);
    result = acm_create_recovery_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_recovery_stream_no_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, 0);
    stream_create_ExpectAndReturn(RECOVERY_STREAM, NULL);
    result = acm_create_recovery_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_recovery_stream_no_egress_header(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, 0);
    stream_create_ExpectAndReturn(RECOVERY_STREAM, &stream);
    stream_set_egress_header_ExpectAndReturn(&stream, dmac, smac, 15, 1, -EINVAL);
    stream_destroy_Expect(&stream);
    result = acm_create_recovery_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_event_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, 0);
    stream_create_ExpectAndReturn(EVENT_STREAM, &stream);
    stream_set_egress_header_ExpectAndReturn(&stream, dmac, smac, 15, 1, 0);
    result = acm_create_event_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(&stream, result);
}

void test_acm_create_event_stream_neg_vlan_pars(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, -EINVAL);
    result = acm_create_event_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_event_stream_no_stream(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, 0);
    stream_create_ExpectAndReturn(EVENT_STREAM, NULL);
    result = acm_create_event_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_create_event_stream_no_egress_header(void) {
    struct acm_stream *result, stream;
    memset(&stream, 0, sizeof (stream));
    uint8_t dmac[ETHER_ADDR_LEN] = "dmacad";
    uint8_t smac[ETHER_ADDR_LEN] = "etsmac";

    stream_check_vlan_parameter_ExpectAndReturn(15, 1, 0);
    stream_create_ExpectAndReturn(EVENT_STREAM, &stream);
    stream_set_egress_header_ExpectAndReturn(&stream, dmac, smac, 15, 1, -EINVAL);
    stream_destroy_Expect(&stream);
    result = acm_create_event_stream(dmac, smac, 15, 1);
    TEST_ASSERT_EQUAL_INT(NULL, result);
}

void test_acm_destroy_stream(void) {
    struct acm_stream stream = { 0 };

    stream_delete_Expect(&stream);
    acm_destroy_stream(&stream);
}

void test_acm_set_reference_stream(void) {
    struct acm_stream stream, stream_ref;
    memset(&stream, 0, sizeof (stream));
    memset(&stream_ref, 0, sizeof (stream_ref));
    int64_t result;

    stream_set_reference_ExpectAndReturn(&stream, &stream_ref, 0);
    result = acm_set_reference_stream(&stream, &stream_ref);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_create_module(void) {
    struct acm_module *result, module;
    memset(&module, 0, sizeof (module));

    module_create_ExpectAndReturn(CONN_MODE_PARALLEL, SPEED_1GBps, 1, &module);
    result = acm_create_module(CONN_MODE_PARALLEL, SPEED_1GBps, 1);
    TEST_ASSERT_EQUAL_INT(&module, result);
}

void test_acm_destroy_module(void) {
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    module_destroy_Expect(&module);
    acm_destroy_module(&module);
}

void test_acm_destroy_module_null(void) {

    acm_destroy_module(NULL);
}

void test_acm_destroy_module_configref(void) {
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            0,
            MODULE_0,
            &configuration);

    logging_Expect(0, "Module: Destroy not possible - added to config");
    acm_destroy_module(&module);
}

void test_acm_set_module_schedule(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct timespec start;
    memset(&start, 0, sizeof (start));
    int result;

    module_set_schedule_ExpectAndReturn(&module, 20, start, 0);
    result = acm_set_module_schedule(&module, 20, start);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_module_stream(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    int result;

    module_add_stream_ExpectAndReturn(&module, &stream, 0);
    result = acm_add_module_stream(&module, &stream);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_module_stream_null_stream(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;

    logging_Expect(0, "Module: Invalid stream input");
    result = acm_add_module_stream(&module, NULL);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_module_stream_null_module(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    int result;

    logging_Expect(0, "Module: Invalid stream input");
    result = acm_add_module_stream(NULL, &stream);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_module_stream_config_applied(void) {
    int result;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, true);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            &configuration);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    /* prepare test case */
    configuration.bypass[0] = &module;

    /* execute test case */
    logging_Expect(0, "Module: Associated configuration already applied to HW");
    result = acm_add_module_stream(&module, &stream);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_module_stream_neg_stream_type_wrong(void) {
    int result;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, false);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_0,
            &configuration);
    struct acm_stream stream = STREAM_INITIALIZER(stream, EVENT_STREAM);

    /* prepare test case */
    configuration.bypass[0] = &module;

    /* execute test case */
    logging_Expect(0, "Module: only Time Triggered and Ingress Triggered streams can be added");
    result = acm_add_module_stream(&module, &stream);
    TEST_ASSERT_EQUAL_INT(-EINVAL, result);
}

void test_acm_add_module_stream_neg_ret_module_add_stream(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    int result;

    /* prepare test case */
    stream.type = TIME_TRIGGERED_STREAM;

    /* execute test case */
    module_add_stream_ExpectAndReturn(&module, &stream, -EPERM);
    result = acm_add_module_stream(&module, &stream);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_module_stream_neg_ret_module_add_stream_ref(void) {
    int result, i;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[2];
    for (i = 0; i < 2; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }

    /* prepare test case */
    stream[0].type = INGRESS_TRIGGERED_STREAM;
    stream[1].type = EVENT_STREAM;
    stream[0].reference = &stream[1];
    stream[1].reference_parent = &stream[0];

    /* execute test case */
    module_add_stream_ExpectAndReturn(&module, &stream[0], 0);
    module_add_stream_ExpectAndReturn(&module, &stream[1], -EPERM);
    result = acm_add_module_stream(&module, &stream[0]);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_module_stream_neg_ret_module_add_stream_ref_ref(void) {
    int result, i;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[3];
    for (i = 0; i < 3; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }

    /* prepare test case */
    stream[0].type = INGRESS_TRIGGERED_STREAM;
    stream[1].type = EVENT_STREAM;
    stream[2].type = RECOVERY_STREAM;
    stream[0].reference = &stream[1];
    stream[1].reference_parent = &stream[0];
    stream[1].reference = &stream[2];
    stream[2].reference_parent = &stream[1];

    /* execute test case */
    module_add_stream_ExpectAndReturn(&module, &stream[0], 0);
    module_add_stream_ExpectAndReturn(&module, &stream[1], 0);
    module_add_stream_ExpectAndReturn(&module, &stream[2], -EPERM);
    result = acm_add_module_stream(&module, &stream[0]);
    TEST_ASSERT_EQUAL_INT(-EPERM, result);
}

void test_acm_add_module_stream_with_two_references(void) {
    int result, i;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[3];
    for (i = 0; i < 3; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }

    /* prepare test case */
    stream[0].type = INGRESS_TRIGGERED_STREAM;
    stream[1].type = EVENT_STREAM;
    stream[2].type = RECOVERY_STREAM;
    stream[0].reference = &stream[1];
    stream[1].reference_parent = &stream[0];
    stream[1].reference = &stream[2];
    stream[2].reference_parent = &stream[1];

    /* execute test case */
    module_add_stream_ExpectAndReturn(&module, &stream[0], 0);
    module_add_stream_ExpectAndReturn(&module, &stream[1], 0);
    module_add_stream_ExpectAndReturn(&module, &stream[2], 0);
    result = acm_add_module_stream(&module, &stream[0]);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_add_module_stream_with_one_reference(void) {
    int result, i;
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_stream stream[2];
    for (i = 0; i < 2; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }

    /* prepare test case */
    stream[0].type = INGRESS_TRIGGERED_STREAM;
    stream[1].type = EVENT_STREAM;
    stream[0].reference = &stream[1];
    stream[1].reference_parent = &stream[0];

    /* execute test case */
    module_add_stream_ExpectAndReturn(&module, &stream[0], 0);
    module_add_stream_ExpectAndReturn(&module, &stream[1], 0);
    result = acm_add_module_stream(&module, &stream[0]);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_create(void) {
    struct acm_config *result, configuration;
    memset(&configuration, 0, sizeof (configuration));

    config_create_ExpectAndReturn(&configuration);
    result = acm_create();
    TEST_ASSERT_EQUAL_INT(&configuration, result);
}

void test_acm_destroy(void) {
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));

    config_destroy_Expect(&configuration);
    acm_destroy(&configuration);
}

void test_acm_add_module(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    int result;

    config_add_module_ExpectAndReturn(&configuration, &module, 0);
    result = acm_add_module(&configuration, &module);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_validate_stream(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    int result;

    validate_stream_ExpectAndReturn(&stream, true, 0);
    result = acm_validate_stream(&stream);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_validate_module(void) {
    struct acm_module module;
    memset(&module, 0, sizeof (module));
    int result;

    validate_module_ExpectAndReturn(&module, true, 0);
    result = acm_validate_module(&module);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_validate_config(void) {
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    int result;

    validate_config_ExpectAndReturn(&configuration, true, 0);
    result = acm_validate_config(&configuration);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_apply_config(void) {
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    int result;

    config_enable_ExpectAndReturn(&configuration, 27, 0);
    result = acm_apply_config(&configuration, 27);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_apply_schedule(void) {
    struct acm_config configuration;
    memset(&configuration, 0, sizeof (configuration));
    int result;

    config_schedule_ExpectAndReturn(&configuration, 27, 27, 0);
    result = acm_apply_schedule(&configuration, 27, 27);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_disable_config(void) {
    int result;

    config_disable_ExpectAndReturn(0);
    result = acm_disable_config();
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_acm_get_buffer_id(void) {
    int ret;

    status_get_buffer_id_from_name_ExpectAndReturn("acm_rx_main", 0);
    ret = acm_get_buffer_id("acm_rx_main");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_acm_read_ip_version(void) {
    char *version;

    status_get_ip_version_ExpectAndReturn("SomeIpVersionString");
    version = acm_read_ip_version();
    TEST_ASSERT_EQUAL_MEMORY("SomeIpVersionString", version, strlen("SomeIpVersionString"));
}

void test_acm_set_rtag_stream(void) {
    int ret;
    struct acm_stream stream;

    stream_set_indiv_recov_ExpectAndReturn(&stream, 100, 0);
    ret = acm_set_rtag_stream(&stream, 100);
    TEST_ASSERT_EQUAL_INT(0, ret);
}
