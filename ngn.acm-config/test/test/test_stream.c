#include "unity.h"

#include <string.h>
#include <errno.h>

#include "stream.h"
#include "tracing.h"

#include "mock_logging.h"
#include "mock_memory.h"
#include "mock_lookup.h"
#include "mock_schedule.h"
#include "mock_module.h"
#include "mock_operation.h"
#include "mock_validate.h"
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

void test_stream_create(void) {
    int stream_type;
    struct acm_stream *stream;
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);

    for (stream_type = 0; stream_type < MAX_STREAM_TYPE; stream_type++) {
        acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
        operation_list_init_ExpectAndReturn(&stream_mem.operations, 0);
        schedule_list_init_ExpectAndReturn(&stream_mem.windows, 0);

        stream = stream_create(stream_type);
        TEST_ASSERT_EQUAL_PTR(&stream_mem, stream);
        TEST_ASSERT_EQUAL(stream_type, stream->type);
    }
}

void test_stream_create_invalid_type(void) {
    struct acm_stream *stream;

    logging_Expect(0, "Stream: Invalid type");
    stream = stream_create(4711);
    TEST_ASSERT_EQUAL_PTR(NULL, stream);
}

void test_stream_create_invalid_type2(void) {
    struct acm_stream *stream;

    logging_Expect(0, "Stream: Invalid type");
    stream = stream_create(MAX_STREAM_TYPE);
    TEST_ASSERT_EQUAL_PTR(NULL, stream);
}

void test_stream_create_no_mem(void) {
    struct acm_stream *stream;

    acm_zalloc_ExpectAndReturn(sizeof (*stream), NULL);
    logging_Expect(0, "Stream: Out of memory");
    stream = stream_create(INGRESS_TRIGGERED_STREAM);
    TEST_ASSERT_EQUAL_PTR(NULL, stream);
}

void test_stream_create_operations_fail(void) {
    struct acm_stream *stream;
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));

    acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
    operation_list_init_ExpectAndReturn(&stream_mem.operations, -EINVAL);
    logging_Expect(0, "Stream: Could not initialize operation list");
    acm_free_Expect(&stream_mem);

    stream = stream_create(INGRESS_TRIGGERED_STREAM);
    TEST_ASSERT_EQUAL_PTR(NULL, stream);
}

void test_stream_create_windows_fail(void) {
    struct acm_stream *stream;
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));

    acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
    operation_list_init_ExpectAndReturn(&stream_mem.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem.windows, -EINVAL);
    logging_Expect(0, "Stream: Could not initialize schedule list");
    operation_list_flush_Expect(&stream_mem.operations);
    acm_free_Expect(&stream_mem);

    stream = stream_create(INGRESS_TRIGGERED_STREAM);
    TEST_ASSERT_EQUAL_PTR(NULL, stream);
}

void test_stream_destroy(void) {
    struct acm_stream stream_mem = { 0 };

    operation_list_flush_Expect(&stream_mem.operations);
    schedule_list_flush_Expect(&stream_mem.windows);
    lookup_destroy_Expect(stream_mem.lookup);
    acm_free_Expect(&stream_mem);

    stream_destroy(&stream_mem);
}

void test_stream_destroy_Null(void) {
    stream_destroy(NULL);
}

void test_stream_delete_null(void) {

    stream_delete(NULL);
}

void test_stream_delete_neg_added_to_module1(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            SPEED_100MBps,
            MODULE_1,
            NULL);

    /* add stream to module */
    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    logging_Expect(0, "Stream: Destroy not possible - added to module");
    stream_delete(&stream);
}

void test_stream_delete_neg_added_to_module2(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1, NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    logging_Expect(0, "Stream: Destroy not possible - added to module");
    stream_delete(&stream);
}

void test_stream_delete_reference_redundant_changes_to_time_triggered(void) {
    struct acm_stream *stream_ref;
    struct acm_stream *stream;
    struct acm_stream stream_mem;
    struct acm_stream stream_mem_ref;
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));
    memset(&stream_mem, 0, sizeof (stream_mem));

    /* prepare test case */
    acm_zalloc_ExpectAndReturn(sizeof (*stream_ref), &stream_mem_ref);
    operation_list_init_ExpectAndReturn(&stream_mem_ref.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem_ref.windows, 0);
    stream_ref = stream_create(TIME_TRIGGERED_STREAM);
    TEST_ASSERT_EQUAL_PTR(&stream_mem_ref, stream_ref);
    TEST_ASSERT_EQUAL(TIME_TRIGGERED_STREAM, stream_ref->type);

    acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
    operation_list_init_ExpectAndReturn(&stream_mem.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem.windows, 0);
    stream = stream_create(TIME_TRIGGERED_STREAM);
    TEST_ASSERT_EQUAL_PTR(&stream_mem, stream);
    TEST_ASSERT_EQUAL(TIME_TRIGGERED_STREAM, stream->type);

    validate_stream_ExpectAndReturn(stream, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(stream, stream_ref));
    TEST_ASSERT_EQUAL(REDUNDANT_STREAM_TX, stream_ref->type);
    TEST_ASSERT_EQUAL(REDUNDANT_STREAM_TX, stream->type);
    TEST_ASSERT_EQUAL_PTR(stream->reference_redundant, stream_ref);
    TEST_ASSERT_EQUAL_PTR(stream_ref->reference_redundant, stream);

    /* execute test case */
    operation_list_flush_Expect(&stream_mem.operations);
    schedule_list_flush_Expect(&stream_mem.windows);
    lookup_destroy_Expect(stream_mem.lookup);
    acm_free_Expect(&stream_mem);

    stream_delete(stream);

    TEST_ASSERT_EQUAL(TIME_TRIGGERED_STREAM, stream_ref->type);
    TEST_ASSERT_EQUAL_PTR(NULL, stream_ref->reference);
}

void test_stream_delete_event_has_parent(void) {
    struct acm_stream *stream_ref;
    struct acm_stream *stream;
    struct acm_stream stream_mem;
    struct acm_stream stream_mem_ref;
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));
    memset(&stream_mem, 0, sizeof (stream_mem));

    /* prepare test case */
    acm_zalloc_ExpectAndReturn(sizeof (*stream_ref), &stream_mem_ref);
    operation_list_init_ExpectAndReturn(&stream_mem_ref.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem_ref.windows, 0);
    stream_ref = stream_create(EVENT_STREAM);

    acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
    operation_list_init_ExpectAndReturn(&stream_mem.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem.windows, 0);
    stream = stream_create(INGRESS_TRIGGERED_STREAM);

    TEST_ASSERT_EQUAL_PTR(&stream_mem_ref, stream_ref);
    TEST_ASSERT_EQUAL(EVENT_STREAM, stream_ref->type);
    TEST_ASSERT_EQUAL_PTR(&stream_mem, stream);
    TEST_ASSERT_EQUAL(INGRESS_TRIGGERED_STREAM, stream->type);

    validate_stream_ExpectAndReturn(stream, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(stream, stream_ref));
    TEST_ASSERT_EQUAL_PTR(stream->reference, stream_ref);
    TEST_ASSERT_EQUAL_PTR(stream_ref->reference_parent, stream);

    /* execute test case */
    logging_Expect(0,
            "Stream: Destroy not possible - type equal Event or Recovery Stream and reference exists");
    stream_delete(stream_ref);
}

void test_stream_delete_reference_event_also_destroyed(void) {
    struct acm_stream *stream_ref;
    struct acm_stream *stream;
    struct acm_stream stream_mem;
    struct acm_stream stream_mem_ref;
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));
    memset(&stream_mem, 0, sizeof (stream_mem));

    /* execute test case */
    acm_zalloc_ExpectAndReturn(sizeof (*stream_ref), &stream_mem_ref);
    operation_list_init_ExpectAndReturn(&stream_mem_ref.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem_ref.windows, 0);
    stream_ref = stream_create(EVENT_STREAM);

    acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
    operation_list_init_ExpectAndReturn(&stream_mem.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem.windows, 0);
    stream = stream_create(INGRESS_TRIGGERED_STREAM);

    TEST_ASSERT_EQUAL_PTR(&stream_mem_ref, stream_ref);
    TEST_ASSERT_EQUAL(EVENT_STREAM, stream_ref->type);
    TEST_ASSERT_EQUAL_PTR(&stream_mem, stream);
    TEST_ASSERT_EQUAL(INGRESS_TRIGGERED_STREAM, stream->type);

    validate_stream_ExpectAndReturn(stream, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(stream, stream_ref));
    TEST_ASSERT_EQUAL_PTR(stream->reference, stream_ref);

    /* execute test case */
    operation_list_flush_Expect(&stream_mem_ref.operations);
    schedule_list_flush_Expect(&stream_mem_ref.windows);
    lookup_destroy_Expect(stream_mem_ref.lookup);
    acm_free_Expect(&stream_mem_ref);
    operation_list_flush_Expect(&stream_mem.operations);
    schedule_list_flush_Expect(&stream_mem.windows);
    lookup_destroy_Expect(stream_mem.lookup);
    acm_free_Expect(&stream_mem);

    stream_delete(stream);
}

void test_stream_delete_reference_event_recovery_also_destroyed(void) {
    struct acm_stream *stream_ref_ref;
    struct acm_stream *stream_ref;
    struct acm_stream *stream;
    struct acm_stream stream_mem;
    struct acm_stream stream_mem_ref;
    struct acm_stream stream_mem_ref_ref;
    memset(&stream_mem_ref_ref, 0, sizeof (stream_mem_ref_ref));
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));
    memset(&stream_mem, 0, sizeof (stream_mem));

    /* prepare test case */
    acm_zalloc_ExpectAndReturn(sizeof (*stream_ref_ref), &stream_mem_ref_ref);
    operation_list_init_ExpectAndReturn(&stream_mem_ref_ref.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem_ref_ref.windows, 0);
    stream_ref_ref = stream_create(RECOVERY_STREAM);

    acm_zalloc_ExpectAndReturn(sizeof (*stream_ref), &stream_mem_ref);
    operation_list_init_ExpectAndReturn(&stream_mem_ref.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem_ref.windows, 0);
    stream_ref = stream_create(EVENT_STREAM);

    acm_zalloc_ExpectAndReturn(sizeof (*stream), &stream_mem);
    operation_list_init_ExpectAndReturn(&stream_mem.operations, 0);
    schedule_list_init_ExpectAndReturn(&stream_mem.windows, 0);
    stream = stream_create(INGRESS_TRIGGERED_STREAM);

    TEST_ASSERT_EQUAL_PTR(&stream_mem_ref_ref, stream_ref_ref);
    TEST_ASSERT_EQUAL(RECOVERY_STREAM, stream_ref_ref->type);
    TEST_ASSERT_EQUAL_PTR(&stream_mem_ref, stream_ref);
    TEST_ASSERT_EQUAL(EVENT_STREAM, stream_ref->type);
    TEST_ASSERT_EQUAL_PTR(&stream_mem, stream);
    TEST_ASSERT_EQUAL(INGRESS_TRIGGERED_STREAM, stream->type);

    validate_stream_ExpectAndReturn(stream_ref, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(stream_ref, stream_ref_ref));
    TEST_ASSERT_EQUAL_PTR(stream_ref->reference, stream_ref_ref);
    validate_stream_ExpectAndReturn(stream, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(stream, stream_ref));
    TEST_ASSERT_EQUAL_PTR(stream->reference, stream_ref);

    /* execute test case */
    operation_list_flush_Expect(&stream_mem_ref_ref.operations);
    schedule_list_flush_Expect(&stream_mem_ref_ref.windows);
    lookup_destroy_Expect(stream_mem_ref_ref.lookup);
    acm_free_Expect(&stream_mem_ref_ref);
    operation_list_flush_Expect(&stream_mem_ref.operations);
    schedule_list_flush_Expect(&stream_mem_ref.windows);
    lookup_destroy_Expect(stream_mem_ref.lookup);
    acm_free_Expect(&stream_mem_ref);
    operation_list_flush_Expect(&stream_mem.operations);
    schedule_list_flush_Expect(&stream_mem.windows);
    lookup_destroy_Expect(stream_mem.lookup);
    acm_free_Expect(&stream_mem);

    stream_delete(stream);
}

void test_stream_add_operation(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation operation_mem;
    memset(&operation_mem, 0, sizeof (operation_mem));

    stream_mem.type = INGRESS_TRIGGERED_STREAM;
    operation_mem.opcode = FORWARD_ALL;
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &operation_mem, 0);
    TEST_ASSERT_EQUAL(0, stream_add_operation(&stream_mem, &operation_mem));
}

void test_stream_add_operation_list_null(void) {
    struct operation operation_mem;
    memset(&operation_mem, 0, sizeof (operation_mem));

    logging_Expect(0, "Stream: stream or operation is null in %s");
    TEST_ASSERT_EQUAL(-EINVAL, stream_add_operation(NULL, &operation_mem));
}

void test_stream_add_operation_op_null(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));

    logging_Expect(0, "Stream: stream or operation is null in %s");
    TEST_ASSERT_EQUAL(-EINVAL, stream_add_operation(&stream_mem, NULL));
}

void test_stream_add_operation_matrix(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation operation_mem;
    memset(&operation_mem, 0, sizeof (operation_mem));

    char message[100];
    struct stream_op {
        enum acm_operation_code op;
        enum stream_type stream;
        int ret_val;
    };

    struct stream_op matrix[] = {
        { .stream = INGRESS_TRIGGERED_STREAM, .op = INSERT, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .op = INSERT_CONSTANT, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .op = PAD, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .op = FORWARD, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .op = READ, .ret_val = 0 },
        { .stream = INGRESS_TRIGGERED_STREAM, .op = FORWARD_ALL, .ret_val = 0 },

        { .stream = TIME_TRIGGERED_STREAM, .op = INSERT, .ret_val = 0 },
        { .stream = TIME_TRIGGERED_STREAM, .op = INSERT_CONSTANT, .ret_val = 0 },
        { .stream = TIME_TRIGGERED_STREAM, .op = PAD, .ret_val = 0 },
        { .stream = TIME_TRIGGERED_STREAM, .op = FORWARD, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .op = READ, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .op = FORWARD_ALL, .ret_val = -EINVAL },

        { .stream = EVENT_STREAM, .op = INSERT, .ret_val = 0 },
        { .stream = EVENT_STREAM, .op = INSERT_CONSTANT, .ret_val = 0 },
        { .stream = EVENT_STREAM, .op = PAD, .ret_val = 0 },
        { .stream = EVENT_STREAM, .op = FORWARD, .ret_val = 0 },
        { .stream = EVENT_STREAM, .op = READ, .ret_val = -EINVAL },
        { .stream = EVENT_STREAM, .op = FORWARD_ALL, .ret_val = -EINVAL },

        { .stream = RECOVERY_STREAM, .op = INSERT, .ret_val = 0 },
        { .stream = RECOVERY_STREAM, .op = INSERT_CONSTANT, .ret_val = 0 },
        { .stream = RECOVERY_STREAM, .op = PAD, .ret_val = 0 },
        { .stream = RECOVERY_STREAM, .op = FORWARD, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .op = READ, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .op = FORWARD_ALL, .ret_val = -EINVAL },

        { .stream = REDUNDANT_STREAM_TX, .op = INSERT, .ret_val = 0 },
        { .stream = REDUNDANT_STREAM_TX, .op = INSERT_CONSTANT, .ret_val = 0 },
        { .stream = REDUNDANT_STREAM_TX, .op = PAD, .ret_val = 0 },
        { .stream = REDUNDANT_STREAM_TX, .op = FORWARD, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .op = READ, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .op = FORWARD_ALL, .ret_val = -EINVAL },

        { .stream = REDUNDANT_STREAM_RX, .op = INSERT, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .op = INSERT_CONSTANT, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .op = PAD, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .op = FORWARD, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .op = READ, .ret_val = 0 },
        { .stream = REDUNDANT_STREAM_RX, .op = FORWARD_ALL, .ret_val = -EINVAL },

        { .stream = MAX_STREAM_TYPE, .op = INSERT, .ret_val = -EINVAL }, };

    for (int i = 0; i < 6 * 6 + 1; i++) {
        stream_mem.type = matrix[i].stream;
        operation_mem.opcode = matrix[i].op;
        if (matrix[i].ret_val == 0) {
            operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &operation_mem, 0);
        } else {
            if (matrix[i].ret_val == -EINVAL) {
                if (matrix[i].stream >= MAX_STREAM_TYPE) {
                    logging_Expect(0, "Stream: Invalid stream type");
                } else {
                    if (matrix[i].stream == REDUNDANT_STREAM_RX) {
                        logging_Expect(0,
                                "Stream: Cannot add operation to stream. REDUNDANT_STREAM_RX allows only READ operations");

                    } else {
                        logging_Expect(0, "Stream: Cannot add operation to stream");
                    }
                }
            }
        }

        sprintf(message, "Stream: %d, Operation: %d", matrix[i].stream + 1, matrix[i].op + 1);
        TEST_ASSERT_EQUAL_MESSAGE(matrix[i].ret_val,
                stream_add_operation(&stream_mem, &operation_mem),
                message);
    }
}

void test_stream_add_operation_fwdall_twice(void) {
    int result;
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation operation_mem1, operation_mem2;
    memset(&operation_mem1, 0, sizeof (operation_mem1));
    memset(&operation_mem2, 0, sizeof (operation_mem2));

    /* prepare test case */
    ACMLIST_INIT(&stream_mem.operations);
    stream_mem.type = INGRESS_TRIGGERED_STREAM;
    operation_mem1.opcode = FORWARD_ALL;
    operation_mem2.opcode = FORWARD_ALL;
    ACMLIST_INSERT_TAIL(&stream_mem.operations, &operation_mem1, entry);

    /* execute test case */
    logging_Expect(0,
            "Stream: operation FORWARD_ALL not possible, has already FORWARD_ALL operation");
    result = stream_add_operation(&stream_mem, &operation_mem2);
    TEST_ASSERT_EQUAL(-EPERM, result);
}

void test_stream_add_operation_fwdall_reference(void) {
    int result;
    struct acm_stream stream_mem, event_stream;
    memset(&stream_mem, 0, sizeof (stream_mem));
    memset(&event_stream, 0, sizeof (event_stream));
    struct operation operation_mem1;
    memset(&operation_mem1, 0, sizeof (operation_mem1));

    /* prepare test case */
    ACMLIST_INIT(&stream_mem.operations);
    stream_mem.type = INGRESS_TRIGGERED_STREAM;
    event_stream.type = EVENT_STREAM;
    stream_mem.reference = &event_stream;
    event_stream.reference_parent = &stream_mem;
    operation_mem1.opcode = FORWARD_ALL;

    /* execute test case */
    logging_Expect(0, "Stream: operation FORWARD_ALL not allowed, has an Event Stream");
    result = stream_add_operation(&stream_mem, &operation_mem1);
    TEST_ASSERT_EQUAL(-EPERM, result);
}

void test_stream_set_reference_matrix(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct acm_stream stream_mem_ref;
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));

    char message[100];
    struct stream_op {
        enum stream_type stream;
        enum stream_type stream_ref;
        int ret_val;
    };

    struct stream_op matrix[] =
    {   { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = 0 },
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = EVENT_STREAM, .ret_val = 0 },
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = 0 },
        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

        { .stream = EVENT_STREAM, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = EVENT_STREAM, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = EVENT_STREAM, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },
        { .stream = EVENT_STREAM, .stream_ref = RECOVERY_STREAM, .ret_val = 0 },
        { .stream = EVENT_STREAM, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = EVENT_STREAM, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = EVENT_STREAM, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

        { .stream = RECOVERY_STREAM, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = RECOVERY_STREAM, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

        { .stream = REDUNDANT_STREAM_TX, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_TX, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

        { .stream = REDUNDANT_STREAM_RX, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = REDUNDANT_STREAM_RX, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

        { .stream = MAX_STREAM_TYPE, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = MAX_STREAM_TYPE, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },
        { .stream = MAX_STREAM_TYPE, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },
        { .stream = MAX_STREAM_TYPE, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },
        { .stream = MAX_STREAM_TYPE, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },
        { .stream = MAX_STREAM_TYPE, .stream_ref = REDUNDANT_STREAM_RX, .ret_val = -EINVAL },
        { .stream = MAX_STREAM_TYPE, .stream_ref = MAX_STREAM_TYPE, .ret_val = -EINVAL },

    };

    for (int i = 0; i < 7 * 7; i++) {
        stream_mem.type = matrix[i].stream;
        stream_mem_ref.type = matrix[i].stream_ref;
        stream_mem.reference = NULL;
        stream_mem.reference_parent = NULL;
        stream_mem.reference_redundant = NULL;
        stream_mem_ref.reference = NULL;
        stream_mem_ref.reference_parent = NULL;
        stream_mem_ref.reference_redundant = NULL;

        if (matrix[i].ret_val == -EINVAL) {
            logging_Expect(0, "Stream: Cannot set stream reference");
        }
        if ( (matrix[i].stream == INGRESS_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == EVENT_STREAM)) {
            validate_stream_ExpectAndReturn(&stream_mem, false, 0);
        }
        if ( ( (matrix[i].stream == EVENT_STREAM) && (matrix[i].stream_ref == RECOVERY_STREAM))
                || ( (matrix[i].stream == TIME_TRIGGERED_STREAM)
                        && (matrix[i].stream_ref == TIME_TRIGGERED_STREAM))) {
            validate_stream_ExpectAndReturn(&stream_mem, false, 0);
        }
        if ( (matrix[i].stream == INGRESS_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == INGRESS_TRIGGERED_STREAM)) {
            get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_RX_REDUNDANCY), 1);
            validate_stream_ExpectAndReturn(&stream_mem, false, 0);
        }

        sprintf(message,
                "Stream: %d, Stream_ref: %d",
                matrix[i].stream + 1,
                matrix[i].stream_ref + 1);
        TEST_ASSERT_EQUAL_MESSAGE(matrix[i].ret_val,
                stream_set_reference(&stream_mem, &stream_mem_ref),
                message);

        if ( ( (matrix[i].stream == INGRESS_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == EVENT_STREAM))
                || ( (matrix[i].stream == EVENT_STREAM) && (matrix[i].stream_ref == RECOVERY_STREAM))) {
            // stream types mustn't be changed, reference and reference parent
            TEST_ASSERT_EQUAL(stream_mem.type, matrix[i].stream);
            TEST_ASSERT_EQUAL(stream_mem_ref.type, matrix[i].stream_ref);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference, &stream_mem_ref);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_redundant, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_parent, &stream_mem);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_redundant, NULL);

        } else if ( (matrix[i].stream == TIME_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == TIME_TRIGGERED_STREAM)) {
            /* stream types are changed to REDUNDANT_STREAM_TX and
             reference_redundant values are set */
            TEST_ASSERT_EQUAL(stream_mem.type, REDUNDANT_STREAM_TX);
            TEST_ASSERT_EQUAL(stream_mem_ref.type, REDUNDANT_STREAM_TX);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_redundant, &stream_mem_ref);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_redundant, &stream_mem);

        } else if ( (matrix[i].stream == INGRESS_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == INGRESS_TRIGGERED_STREAM)) {
            /* stream types are changed to REDUNDANT_STREAM_TX and
             * reference_redundant values are set */
            TEST_ASSERT_EQUAL(stream_mem.type, REDUNDANT_STREAM_RX);
            TEST_ASSERT_EQUAL(stream_mem_ref.type, REDUNDANT_STREAM_RX);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_redundant, &stream_mem_ref);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_redundant, &stream_mem);

        } else {
            // stream type and stream references mustn't be changed
            TEST_ASSERT_EQUAL(stream_mem.type, matrix[i].stream);
            TEST_ASSERT_EQUAL(stream_mem_ref.type, matrix[i].stream_ref);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem.reference_redundant, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_parent, NULL);
            TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_redundant, NULL);
        }
    }
}

void test_stream_set_reference_matrix_negative_cases(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct acm_stream stream_mem_ref;
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));

    char message[100];
    struct stream_op {
        enum stream_type stream;
        enum stream_type stream_ref;
        int ret_val;
    };

    struct stream_op matrix[] = {
        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = EVENT_STREAM, .ret_val = -EINVAL },

        { .stream = TIME_TRIGGERED_STREAM, .stream_ref = TIME_TRIGGERED_STREAM, .ret_val = -EINVAL },

        { .stream = EVENT_STREAM, .stream_ref = RECOVERY_STREAM, .ret_val = -EINVAL },

        { .stream = REDUNDANT_STREAM_TX, .stream_ref = REDUNDANT_STREAM_TX, .ret_val = -EINVAL },

        { .stream = INGRESS_TRIGGERED_STREAM, .stream_ref = INGRESS_TRIGGERED_STREAM, .ret_val = -EINVAL },
    };

    for (int i = 0; i < 1 * 5; i++) {
        stream_mem.type = matrix[i].stream;
        stream_mem_ref.type = matrix[i].stream_ref;
        stream_mem.reference = NULL;
        stream_mem.reference_parent = NULL;
        stream_mem.reference_redundant = NULL;
        stream_mem_ref.reference = NULL;
        stream_mem_ref.reference_parent = NULL;
        stream_mem_ref.reference_redundant = NULL;

        if ( (matrix[i].stream == INGRESS_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == EVENT_STREAM)) {
            validate_stream_ExpectAndReturn(&stream_mem, false, -EACMEGRESSFRAMESIZE);
            logging_Expect(0, "Stream: Cannot set stream reference - validation failed");
        }
        if ( ( (matrix[i].stream == EVENT_STREAM) && (matrix[i].stream_ref == RECOVERY_STREAM))
                || ( (matrix[i].stream == TIME_TRIGGERED_STREAM)
                        && (matrix[i].stream_ref == TIME_TRIGGERED_STREAM))) {
            validate_stream_ExpectAndReturn(&stream_mem, false, -EINVAL);
            logging_Expect(0, "Stream: Cannot set stream reference - validation failed");
        }
        if ( (matrix[i].stream == INGRESS_TRIGGERED_STREAM)
                && (matrix[i].stream_ref == INGRESS_TRIGGERED_STREAM)) {
            get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_RX_REDUNDANCY),
                    RX_REDUNDANCY_SET);
            validate_stream_ExpectAndReturn(&stream_mem, false, -EINVAL);
            logging_Expect(0, "Stream: Cannot set stream reference - validation failed");
        }
        if ( (matrix[i].stream == REDUNDANT_STREAM_TX)
                && (matrix[i].stream_ref == REDUNDANT_STREAM_TX)) {
            logging_Expect(0, "Stream: Cannot set stream reference");
        }

        sprintf(message,
                "Stream: %d, Stream_ref: %d",
                matrix[i].stream + 1,
                matrix[i].stream_ref + 1);
        TEST_ASSERT_EQUAL_MESSAGE(matrix[i].ret_val,
                stream_set_reference(&stream_mem, &stream_mem_ref),
                message);

        // stream type and stream references mustn't be changed
        TEST_ASSERT_EQUAL(stream_mem.type, matrix[i].stream);
        TEST_ASSERT_EQUAL(stream_mem_ref.type, matrix[i].stream_ref);
        TEST_ASSERT_EQUAL_PTR(stream_mem.reference, NULL);
        TEST_ASSERT_EQUAL_PTR(stream_mem.reference_parent, NULL);
        TEST_ASSERT_EQUAL_PTR(stream_mem.reference_redundant, NULL);
        TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference, NULL);
        TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_parent, NULL);
        TEST_ASSERT_EQUAL_PTR(stream_mem_ref.reference_redundant, NULL);

    }
}

void test_stream_set_reference_stream_null(void) {
    struct acm_stream stream_mem_ref;
    memset(&stream_mem_ref, 0, sizeof (stream_mem_ref));

    logging_Expect(0, "Stream: stream or stream reference is null");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(NULL, &stream_mem_ref));
}

void test_stream_set_reference_stream_ref_null(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));

    logging_Expect(0, "Stream: stream or stream reference is null");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, NULL));
}

void test_stream_set_reference_stream_ref_set1(void) {
    struct acm_stream stream_mem, stream_ref;
    memset(&stream_mem, 0, sizeof (stream_mem));
    memset(&stream_ref, 0, sizeof (stream_ref));
    stream_mem.reference = &stream_ref;

    logging_Expect(0, "Stream: stream or reference already referenced");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
}

void test_stream_set_reference_stream_ref_set2(void) {
    struct acm_stream stream_mem, stream_ref;
    memset(&stream_mem, 0, sizeof (stream_mem));
    memset(&stream_ref, 0, sizeof (stream_ref));
    stream_mem.reference_redundant = &stream_ref;

    logging_Expect(0, "Stream: stream or reference already referenced");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
}

void test_stream_set_reference_stream_ref_set3(void) {
    struct acm_stream stream_mem, stream_ref;
    memset(&stream_mem, 0, sizeof (stream_mem));
    memset(&stream_ref, 0, sizeof (stream_ref));
    stream_ref.reference_parent = &stream_mem;

    logging_Expect(0, "Stream: stream or reference already referenced");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
}

void test_stream_set_reference_stream_ref_set4(void) {
    struct acm_stream stream_mem, stream_ref;
    memset(&stream_mem, 0, sizeof (stream_mem));
    memset(&stream_ref, 0, sizeof (stream_ref));
    stream_ref.reference_redundant = &stream_mem;

    logging_Expect(0, "Stream: stream or reference already referenced");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
}

void test_stream_set_reference_stream_at_module1(void) {
    int ret;
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, EVENT_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1, NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    operation_list_update_smac_ExpectAndReturn(&stream_ref.operations, module.module_id, 0);
    validate_stream_list_ExpectAndReturn(&module.streams, false, 0);
    validate_stream_ExpectAndReturn(&stream_mem, false, 0);
    ret = stream_set_reference(&stream_mem, &stream_ref);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(2, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_stream_at_module2(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, EVENT_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1, NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    operation_list_update_smac_ExpectAndReturn(&stream_ref.operations,
            module.module_id,
            -EACMINTERNAL);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_stream_at_module3(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, EVENT_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1, NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    operation_list_update_smac_ExpectAndReturn(&stream_ref.operations, module.module_id, 0);
    validate_stream_list_ExpectAndReturn(&module.streams, false, 0);
    validate_stream_ExpectAndReturn(&stream_mem, false, -EACMEGRESSFRAMESIZE);
    logging_Expect(0, "Stream: Cannot set stream reference - validation failed");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_config_applied(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, EVENT_STREAM);
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, true);
    struct acm_module module = MODULE_INITIALIZER(module,
            CONN_MODE_SERIAL,
            0,
            MODULE_1,
            &configuration);

    /* prepare test case */
    configuration.bypass[0] = &module;
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    logging_Expect(0, "Stream: configuration of stream already applied to HW");
    TEST_ASSERT_EQUAL(-EPERM, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_neg_forward_all1(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, EVENT_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1,
    NULL);
    struct operation operation = FORWARD_ALL_OPERATION_INITIALIZER;

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);
    ACMLIST_INSERT_TAIL(&stream_mem.operations, &operation, entry);

    /* execute test case */
    logging_Expect(0, "Stream: Ingress Triggered Stream already contains a ForwardAll Operation");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_neg_forward_all2(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1,
    NULL);
    struct operation operation = FORWARD_ALL_OPERATION_INITIALIZER;

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);
    ACMLIST_INSERT_TAIL(&stream_mem.operations, &operation, entry);

    /* execute test case */
    logging_Expect(0,
            "Stream: One Ingress Triggered Stream already contains a ForwardAll Operation");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_neg_rx_redund_disabled(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1,
    NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_RX_REDUNDANCY), 0);
    logging_Expect(0, "Stream: RX redundancy not set. Has value %d");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_neg_rx_redund_enabled_ingress(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1,
    NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_RX_REDUNDANCY), RX_REDUNDANCY_SET);
    validate_stream_ExpectAndReturn(&stream_mem, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_reference_neg_rx_redund_enabled_time(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, TIME_TRIGGERED_STREAM);
    struct acm_stream stream_ref = STREAM_INITIALIZER(stream_ref, TIME_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_1,
    NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    validate_stream_ExpectAndReturn(&stream_mem, false, 0);
    TEST_ASSERT_EQUAL(0, stream_set_reference(&stream_mem, &stream_ref));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
}

void test_stream_set_egress_header(void) {
    int result;
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, TIME_TRIGGERED_STREAM);
    struct operation op_dmac = INSERT_OPERATION_INITIALIZER(0, NULL, NULL);
    struct operation op_smac = INSERT_OPERATION_INITIALIZER(0, NULL, NULL);
    struct operation op_vlan = INSERT_OPERATION_INITIALIZER(0, NULL, NULL);
    char dmac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char smac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char vlan[4] = { 0x81, 0x00, 0xC3, 0xE8 };

    operation_create_insertconstant_ExpectAndReturn(dmac, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, 0);
    operation_create_insertconstant_ExpectAndReturn(smac, 6, &op_smac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_smac, 0);
    operation_create_insertconstant_ExpectAndReturn(vlan, 4, &op_vlan);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_vlan, 0);
    result = stream_set_egress_header(&stream_mem, dmac, smac, 1000, 6);
    TEST_ASSERT_EQUAL(0, result);
}

void test_stream_set_egress_header_stream_null(void) {
    char dmac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char smac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    logging_Expect(0, "Stream: parameter stream is NULL in %s");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_egress_header(NULL, dmac, smac, 1000, 6));
}

void test_stream_set_egress_header_stream_invalid_type(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));

    char dmac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char smac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    stream_mem.type = INGRESS_TRIGGERED_STREAM;

    logging_Expect(0, "Stream: operation not supported for this stream-type");
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_egress_header(&stream_mem, dmac, smac, 1000, 6));
}

void test_stream_set_egress_header_event_stream(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation op_dmac, op_smac, op_vlan;
    memset(&op_dmac, 0, sizeof (op_dmac));
    memset(&op_smac, 0, sizeof (op_smac));
    memset(&op_vlan, 0, sizeof (op_vlan));

    char dmac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char smac[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
    char vlan[4] = { 0x81, 0x00, 0xC3, 0xE8 };
    stream_mem.type = EVENT_STREAM;

    operation_create_insertconstant_ExpectAndReturn(dmac, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, 0);
    operation_create_insertconstant_ExpectAndReturn(smac, 6, &op_smac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_smac, 0);
    operation_create_insertconstant_ExpectAndReturn(vlan, 4, &op_vlan);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_vlan, 0);
    TEST_ASSERT_EQUAL(0, stream_set_egress_header(&stream_mem, dmac, smac, 1000, 6));
}

void test_stream_set_egress_header_event_stream_dmac_smac_ff(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation op_dmac, op_smac, op_vlan;
    memset(&op_dmac, 0, sizeof (op_dmac));
    memset(&op_smac, 0, sizeof (op_smac));
    memset(&op_vlan, 0, sizeof (op_vlan));

    char dmac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    char smac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    stream_mem.type = EVENT_STREAM;

    operation_create_forward_ExpectAndReturn(OFFSET_DEST_MAC_IN_FRAME, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, 0);
    operation_create_forward_ExpectAndReturn(OFFSET_SOURCE_MAC_IN_FRAME, 6, &op_smac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_smac, 0);
    operation_create_forward_ExpectAndReturn(OFFSET_VLAN_TAG_IN_FRAME, 4, &op_vlan);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_vlan, 0);
    TEST_ASSERT_EQUAL(0, stream_set_egress_header(&stream_mem, dmac, smac, ACM_VLAN_ID_MAX, 6));
}

void test_stream_set_egress_header_event_stream_empty_smac(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation op_dmac, op_smac, op_vlan;
    memset(&op_dmac, 0, sizeof (op_dmac));
    memset(&op_smac, 0, sizeof (op_smac));
    memset(&op_vlan, 0, sizeof (op_vlan));

    char dmac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    char smac[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
    stream_mem.type = EVENT_STREAM;

    operation_create_forward_ExpectAndReturn(OFFSET_DEST_MAC_IN_FRAME, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, -EINVAL);
    acm_free_Expect(&op_dmac);
    TEST_ASSERT_EQUAL(-EINVAL,
            stream_set_egress_header(&stream_mem, dmac, smac, ACM_VLAN_ID_MAX, 6));
}

void test_stream_set_egress_header_no_event_stream_empty_smac(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation op_dmac, op_smac, op_vlan;
    memset(&op_dmac, 0, sizeof (op_dmac));
    memset(&op_smac, 0, sizeof (op_smac));
    memset(&op_vlan, 0, sizeof (op_vlan));

    char dmac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    char smac[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
    stream_mem.type = RECOVERY_STREAM;

    operation_create_insertconstant_ExpectAndReturn(dmac, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, 0);
    operation_create_insertconstant_ExpectAndReturn(LOCAL_SMAC_CONST, 6, &op_smac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_smac, -EINVAL);
    operation_list_remove_operation_Expect(&stream_mem.operations, &op_dmac);
    acm_free_Expect(&op_dmac);
    acm_free_Expect(&op_smac);
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_egress_header(&stream_mem, dmac, smac, 200, 6));
}

void test_stream_set_egress_header_neg_add_operation(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation op_dmac, op_smac, op_vlan;
    memset(&op_dmac, 0, sizeof (op_dmac));
    memset(&op_smac, 0, sizeof (op_smac));
    memset(&op_vlan, 0, sizeof (op_vlan));

    char dmac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    char smac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    char vlan[4] = { 0x81, 0x00, 0xC3, 0xE8 };
    stream_mem.type = RECOVERY_STREAM;

    operation_create_insertconstant_ExpectAndReturn(dmac, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, 0);
    operation_create_insertconstant_ExpectAndReturn(smac, 6, &op_smac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_smac, 0);
    operation_create_insertconstant_ExpectAndReturn(vlan, 4, &op_vlan);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_vlan, -EINVAL);
    operation_list_remove_operation_Expect(&stream_mem.operations, &op_dmac);
    operation_list_remove_operation_Expect(&stream_mem.operations, &op_smac);
    //operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &operation_mem, 0);
    acm_free_Expect(&op_dmac);
    acm_free_Expect(&op_smac);
    acm_free_Expect(&op_vlan);
    TEST_ASSERT_EQUAL(-EINVAL, stream_set_egress_header(&stream_mem, dmac, smac, 200, 6));
}

void test_stream_set_egress_header_neg_no_vlan(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));
    struct operation op_dmac, op_smac, op_vlan;
    memset(&op_dmac, 0, sizeof (op_dmac));
    memset(&op_smac, 0, sizeof (op_smac));
    memset(&op_vlan, 0, sizeof (op_vlan));

    char dmac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    char smac[6] = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
    stream_mem.type = RECOVERY_STREAM;

    operation_create_insertconstant_ExpectAndReturn(dmac, 6, &op_dmac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_dmac, 0);
    operation_create_insertconstant_ExpectAndReturn(LOCAL_SMAC_CONST, 6, &op_smac);
    operation_list_add_operation_ExpectAndReturn(&stream_mem.operations, &op_smac, 0);
    logging_Expect(0, "Stream: No VLAN-ID defined");
    operation_list_remove_operation_Expect(&stream_mem.operations, &op_dmac);
    operation_list_remove_operation_Expect(&stream_mem.operations, &op_smac);
    acm_free_Expect(&op_dmac);
    acm_free_Expect(&op_smac);
    TEST_ASSERT_EQUAL(-EINVAL,
            stream_set_egress_header(&stream_mem, dmac, smac, ACM_VLAN_ID_MAX, 6));
}

void test_stream_init_list(void) {
    struct stream_list stream_list_mem = STREAMLIST_INITIALIZER(stream_list_mem);

    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream_list_mem));
    TEST_ASSERT_EQUAL(0, ACMLIST_LOCK(&stream_list_mem));
    TEST_ASSERT(ACMLIST_EMPTY(&stream_list_mem));

    ACMLIST_UNLOCK(&stream_list_mem);
}

void test_stream_add_list(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
    NULL);

    operation_list_update_smac_ExpectAndReturn(&stream_mem.operations, 0, 0);
    validate_stream_list_ExpectAndReturn(&module.streams, false, 0);
    TEST_ASSERT_EQUAL(0, stream_add_list(&module.streams, &stream_mem));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
    TEST_ASSERT_EQUAL_PTR(&stream_mem, ACMLIST_FIRST(&module.streams));
}

void test_stream_add_list_neg_calc_indizes(void) {
    //set an invalid stream type
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, MAX_STREAM_TYPE);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
    NULL);

    operation_list_update_smac_ExpectAndReturn(&stream_mem.operations, 0, 0);
    validate_stream_list_ExpectAndReturn(&module.streams, false, 0);
    logging_Expect(0, "Stream: stream without a stream type ");
    TEST_ASSERT_EQUAL(-EACMINTERNAL, stream_add_list(&module.streams, &stream_mem));
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&module.streams));
    TEST_ASSERT_EQUAL_PTR(NULL, ACMLIST_FIRST(&module.streams));
}

void test_stream_add_list_list_null(void) {
    struct acm_stream stream_mem;
    memset(&stream_mem, 0, sizeof (stream_mem));

    logging_Expect(0, "Stream: stream or streamlist is NULL in %s");
    TEST_ASSERT_EQUAL(-EINVAL, stream_add_list(NULL, &stream_mem));
}

void test_stream_add_list_entry_null(void) {
    struct stream_list stream_list_mem;
    memset(&stream_list_mem, 0, sizeof (stream_list_mem));

    logging_Expect(0, "Stream: stream or streamlist is NULL in %s");
    TEST_ASSERT_EQUAL(-EINVAL, stream_add_list(&stream_list_mem, NULL));
}

void test_stream_add_list_validate_fails(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
    NULL);

    operation_list_update_smac_ExpectAndReturn(&stream_mem.operations, 0, 0);
    validate_stream_list_ExpectAndReturn(&module.streams, false, -EINVAL);
    TEST_ASSERT_EQUAL(-EINVAL, stream_add_list(&module.streams, &stream_mem));
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&module.streams));
    TEST_ASSERT_EQUAL_PTR(NULL, ACMLIST_FIRST(&module.streams));
}

void test_stream_add_list_add_twice(void) {
    struct acm_stream stream_mem = STREAM_INITIALIZER(stream_mem, INGRESS_TRIGGERED_STREAM);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
    NULL);

    /* test preparation */
    _ACMLIST_INSERT_TAIL(&module.streams, &stream_mem, entry);

    /* execute test case */
    logging_Expect(0, "Stream: Cannot be added a second time");
    TEST_ASSERT_EQUAL(-EPERM, stream_add_list(&module.streams, &stream_mem));
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&module.streams));
    TEST_ASSERT_EQUAL_PTR(&stream_mem, ACMLIST_FIRST(&module.streams));
}

void test_stream_empty_list(void) {
    int i;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
    NULL);
    struct acm_stream streams[10] = {
    STREAM_INITIALIZER(streams[0], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[1], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[2], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[3], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[4], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[5], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[6], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[7], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[8], INGRESS_TRIGGERED_STREAM),
    STREAM_INITIALIZER(streams[9], INGRESS_TRIGGERED_STREAM), };

    /* fill list */
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[0], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[1], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[2], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[3], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[4], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[5], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[6], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[7], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[8], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &streams[9], entry);

    TEST_ASSERT_EQUAL(10, ACMLIST_COUNT(&module.streams));
    for (i = 0; i < 10; ++i) {
        operation_list_flush_Expect(&streams[i].operations);
        schedule_list_flush_Expect(&streams[i].windows);
        lookup_destroy_Expect(streams[i].lookup);
        acm_free_Expect(&streams[i]);
    }
    stream_empty_list(&module.streams);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&module.streams));
}

void test_stream_empty_list_null(void) {
    stream_empty_list(NULL);
}

void test_stream_clean_msg_buff_links(void) {
    int i;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation op_item[5];
    for (i = 0; i < 5; i++)
        memset(&op_item[i], 0, sizeof (op_item[i]));
    struct sysfs_buffer msg_buff[3];
    for (i = 0; i < 3; i++)
        memset(&msg_buff[i], 0, sizeof (msg_buff[i]));

    /* prepare test case */
    /* fill list */
    ACMLIST_INIT(&stream.operations);
    op_item[0].opcode = READ;
    op_item[0].msg_buf = &msg_buff[0];
    op_item[1].opcode = PAD;
    op_item[2].opcode = FORWARD;
    op_item[3].opcode = INSERT;
    op_item[3].msg_buf = &msg_buff[1];
    op_item[4].opcode = READ;
    op_item[4].msg_buf = &msg_buff[2];
    for (i = 0; i < 5; ++i)
        ACMLIST_INSERT_TAIL(&stream.operations, &op_item[i], entry);

    /* execute test case */
    stream_clean_msg_buff_links(&stream);
    for (i = 0; i < 5; ++i)
        TEST_ASSERT_EQUAL_PTR(NULL, op_item[i].msg_buf);
}

void test_stream_clean_msg_buff_links_null(void) {

    stream_clean_msg_buff_links(NULL);
}

void test_stream_remove_list_stream_not_in_list(void) {
    struct stream_list streamlist;
    memset(&streamlist, 0, sizeof (streamlist));
    int i;
    struct acm_stream stream[5];
    for (i = 0; i < 5; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }
    /* prepare test case */
    ACMLIST_INIT(&streamlist);
    for (i = 0; i < 4; ++i) {
        ACMLIST_INSERT_TAIL(&streamlist, &stream[i], entry);
    }

    /* prepare test case */
    stream_remove_list(&streamlist, &stream[4]);
    TEST_ASSERT_EQUAL(4, ACMLIST_COUNT(&streamlist)); // still 4 - no stream removed
}

void test_stream_remove_list_last(void) {
    struct stream_list streamlist;
    memset(&streamlist, 0, sizeof (streamlist));
    int i;
    struct acm_stream stream[5];
    for (i = 0; i < 5; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }
    /* prepare test case */
    ACMLIST_INIT(&streamlist);
    for (i = 0; i < 5; ++i) {
        ACMLIST_INSERT_TAIL(&streamlist, &stream[i], entry);
    }

    /* prepare test case */
    stream_remove_list(&streamlist, &stream[4]);
    TEST_ASSERT_EQUAL(4, ACMLIST_COUNT(&streamlist)); // last stream removed
}

void test_stream_remove_list_middle_item(void) {
    struct stream_list streamlist;
    memset(&streamlist, 0, sizeof (streamlist));
    int i;
    struct acm_stream stream[5];
    for (i = 0; i < 5; i++) {
        memset(&stream[i], 0, sizeof (stream[i]));
    }
    /* prepare test case */
    ACMLIST_INIT(&streamlist);
    for (i = 0; i < 5; ++i) {
        ACMLIST_INSERT_TAIL(&streamlist, &stream[i], entry);
    }

    /* prepare test case */
    stream_remove_list(&streamlist, &stream[2]);
    TEST_ASSERT_EQUAL(4, ACMLIST_COUNT(&streamlist)); //  stream[2] removed
}

void test_stream_check_vlan_parameter(void) {
    int result;

    result = stream_check_vlan_parameter(ACM_VLAN_ID_MIN, ACM_VLAN_PRIO_MIN);
    TEST_ASSERT_EQUAL(0, result);
    result = stream_check_vlan_parameter(ACM_VLAN_ID_MAX, ACM_VLAN_PRIO_MAX);
    TEST_ASSERT_EQUAL(0, result);
    result = stream_check_vlan_parameter(ACM_VLAN_ID_MIN + 1,
    ACM_VLAN_PRIO_MAX - 1);
    TEST_ASSERT_EQUAL(0, result);
}

void test_stream_check_vlan_parameter_neg(void) {
    int result;

    logging_Expect(0, "Stream: VLAN id out of range.");
    result = stream_check_vlan_parameter(ACM_VLAN_ID_MIN - 1, ACM_VLAN_PRIO_MIN);
    TEST_ASSERT_EQUAL(-EINVAL, result);
    logging_Expect(0, "Stream: VLAN id out of range.");
    result = stream_check_vlan_parameter(ACM_VLAN_ID_MAX + 1, ACM_VLAN_PRIO_MIN);
    TEST_ASSERT_EQUAL(-EINVAL, result);
    logging_Expect(0, "Stream: VLAN priority out of range.");
    result = stream_check_vlan_parameter(2222, ACM_VLAN_PRIO_MAX + 1);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_stream_config_applied_stream_null(void) {
    bool result;

    result = stream_config_applied(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_stream_config_applied_streamlist_no_module(void) {
    struct stream_list streamlist = STREAMLIST_INITIALIZER(streamlist);
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);
    bool result;

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&streamlist, &stream, entry);

    /* execute test case */
    result = stream_config_applied(&stream);
    TEST_ASSERT_FALSE(result);
}

void test_stream_has_operation_x(void) {
    bool result;
    int i;
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    struct operation op_item[4];
    for (i = 0; i < 4; i++) {
        memset(&op_item[i], 0, sizeof (op_item[i]));
    }

    /* prepare test case */
    ACMLIST_INIT(&stream.operations);
    for (i = 0; i < 4; i++) {
        op_item[i].opcode = INSERT;
        ACMLIST_INSERT_TAIL(&stream.operations, &op_item[i], entry);
    }

    /* execute test case */
    result = stream_has_operation_x(&stream, READ);
    TEST_ASSERT_FALSE(result);
    result = stream_has_operation_x(&stream, INSERT);
    TEST_ASSERT_TRUE(result);
}

void test_stream_has_operation_x_null(void) {
    bool result;

    result = stream_has_operation_x(NULL, READ);
    TEST_ASSERT_FALSE(result);
}

void test_calculate_indizes_for_HW_tables(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct acm_stream recovery_stream = STREAM_INITIALIZER(recovery_stream, RECOVERY_STREAM);
    struct acm_stream redundand_rx_stream1 =
            STREAM_INITIALIZER(recovery_stream, REDUNDANT_STREAM_RX);
    struct acm_stream redundand_rx_stream2 =
            STREAM_INITIALIZER(recovery_stream, REDUNDANT_STREAM_RX);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);

    /* prepare test case */
    ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    /* execute test case */
    result = calculate_indizes_for_HW_tables(&module.streams, &stream);
    TEST_ASSERT_EQUAL(0, result);

    ACMLIST_INSERT_TAIL(&module.streams, &recovery_stream, entry);
    result = calculate_indizes_for_HW_tables(&module.streams, &recovery_stream);
    TEST_ASSERT_EQUAL(0, result);

    ACMLIST_INSERT_TAIL(&module.streams, &redundand_rx_stream1, entry);
    redundand_rx_stream1.reference_redundant = &redundand_rx_stream2;
    redundand_rx_stream2.reference_redundant = &redundand_rx_stream1;
    result = calculate_indizes_for_HW_tables(&module.streams, &redundand_rx_stream1);
    TEST_ASSERT_EQUAL(0, result);
}

void test_calculate_lookup_indizes(void) {
    int i;
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, 0, MODULE_0, NULL);
    struct acm_stream stream[8] = {
        STREAM_INITIALIZER(stream[0], TIME_TRIGGERED_STREAM),
        STREAM_INITIALIZER(stream[1], INGRESS_TRIGGERED_STREAM),
        STREAM_INITIALIZER(stream[2], EVENT_STREAM),
        STREAM_INITIALIZER(stream[3], INGRESS_TRIGGERED_STREAM),
        STREAM_INITIALIZER(stream[4], RECOVERY_STREAM),
        STREAM_INITIALIZER(stream[5], REDUNDANT_STREAM_TX),
        STREAM_INITIALIZER(stream[6], INGRESS_TRIGGERED_STREAM),
        STREAM_INITIALIZER(stream[7], INGRESS_TRIGGERED_STREAM)
    };

    /* prepare test case */
    for (i = 0; i < 8; i++)
        ACMLIST_INSERT_TAIL(&module.streams, &stream[i], entry);

    /* execute test case */
    calculate_lookup_indizes(&module.streams);
    TEST_ASSERT_EQUAL(0, stream[0].lookup_index);
    TEST_ASSERT_EQUAL(0, stream[1].lookup_index);
    TEST_ASSERT_EQUAL(0, stream[2].lookup_index);
    TEST_ASSERT_EQUAL(1, stream[3].lookup_index);
    TEST_ASSERT_EQUAL(0, stream[4].lookup_index);
    TEST_ASSERT_EQUAL(0, stream[5].lookup_index);
    TEST_ASSERT_EQUAL(2, stream[6].lookup_index);
    TEST_ASSERT_EQUAL(3, stream[7].lookup_index);
}

void test_calculate_lookup_indizes_null(void) {

    calculate_lookup_indizes(NULL);
}

void test_calculate_redundancy_indizes(void) {
    int i;
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, 0, MODULE_0, NULL),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, 0, MODULE_1, NULL)
    };
    struct acm_stream stream[12] = {
            STREAM_INITIALIZER(stream[0], TIME_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[2], EVENT_STREAM),
            STREAM_INITIALIZER(stream[3], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[4], RECOVERY_STREAM),
            STREAM_INITIALIZER(stream[5], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[6], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[7], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[8], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[9], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[10], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[11], TIME_TRIGGERED_STREAM),
    };

    /* prepare test case */
    stream[3].reference_redundant = &stream[8];
    stream[8].reference_redundant = &stream[3];
    stream[5].reference_redundant = &stream[10];
    stream[10].reference_redundant = &stream[5];
    stream[7].reference_redundant = &stream[9];
    stream[9].reference_redundant = &stream[7];

    for (i = 0; i < 8; i++)
        ACMLIST_INSERT_TAIL(&module[0].streams, &stream[i], entry);
    for (i = 8; i < 12; i++)
        ACMLIST_INSERT_TAIL(&module[1].streams, &stream[i], entry);

    /* execute test case */
    calculate_redundancy_indizes(&module[0].streams);
    TEST_ASSERT_EQUAL(0, stream[0].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[1].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[2].redundand_index);
    TEST_ASSERT_EQUAL(1, stream[3].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[4].redundand_index);
    TEST_ASSERT_EQUAL(2, stream[5].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[6].redundand_index);
    TEST_ASSERT_EQUAL(3, stream[7].redundand_index);
    TEST_ASSERT_EQUAL(1, stream[8].redundand_index);
    TEST_ASSERT_EQUAL(3, stream[9].redundand_index);
    TEST_ASSERT_EQUAL(2, stream[10].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[11].redundand_index);
    calculate_redundancy_indizes(&module[1].streams);
    TEST_ASSERT_EQUAL(0, stream[0].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[1].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[2].redundand_index);
    TEST_ASSERT_EQUAL(1, stream[3].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[4].redundand_index);
    TEST_ASSERT_EQUAL(3, stream[5].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[6].redundand_index);
    TEST_ASSERT_EQUAL(2, stream[7].redundand_index);
    TEST_ASSERT_EQUAL(1, stream[8].redundand_index);
    TEST_ASSERT_EQUAL(2, stream[9].redundand_index);
    TEST_ASSERT_EQUAL(3, stream[10].redundand_index);
    TEST_ASSERT_EQUAL(0, stream[11].redundand_index);
}

void test_calculate_redundancy_indizes_neg_streamlist(void) {

    calculate_redundancy_indizes(NULL);
}


void test_calculate_gather_indizes(void) {
    int i;
    struct operation op_fwd_all = FORWARD_ALL_OPERATION_INITIALIZER;
    struct stream_list streamlist = STREAMLIST_INITIALIZER(streamlist);
    struct acm_stream stream[5] = {
            STREAM_INITIALIZER(stream[0], TIME_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], TIME_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[2], TIME_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[3], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[4], TIME_TRIGGERED_STREAM)
    };

    struct operation gather_op[8] = {
            INSERT_OPERATION_INITIALIZER(0, NULL, NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 0),
            PAD_OPERATION_INITIALIZER(0, NULL),
            INSERT_OPERATION_INITIALIZER(0, NULL, NULL),
            FORWARD_OPERATION_INITIALIZER(0, 0),
            FORWARD_OPERATION_INITIALIZER(0, 0),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 0),
            INSERT_OPERATION_INITIALIZER(0, NULL, NULL)
    };

    /* prepare test case */
    for (i = 0; i < 5; i++)
        ACMLIST_INSERT_TAIL(&streamlist, &stream[i], entry);

    ACMLIST_INSERT_TAIL(&stream[2].operations, &op_fwd_all, entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &gather_op[0], entry);
    ACMLIST_INSERT_TAIL(&stream[3].operations, &gather_op[1], entry);
    ACMLIST_INSERT_TAIL(&stream[3].operations, &gather_op[2], entry);
    ACMLIST_INSERT_TAIL(&stream[3].operations, &gather_op[3], entry);
    ACMLIST_INSERT_TAIL(&stream[3].operations, &gather_op[4], entry);
    ACMLIST_INSERT_TAIL(&stream[4].operations, &gather_op[5], entry);
    ACMLIST_INSERT_TAIL(&stream[4].operations, &gather_op[6], entry);
    ACMLIST_INSERT_TAIL(&stream[4].operations, &gather_op[7], entry);

    /* execute test case */
    calculate_gather_indizes(&streamlist);
    TEST_ASSERT_EQUAL(0, stream[0].gather_dma_index);
    TEST_ASSERT_EQUAL(2, stream[1].gather_dma_index);
    TEST_ASSERT_EQUAL(1, stream[2].gather_dma_index);
    TEST_ASSERT_EQUAL(7, stream[3].gather_dma_index);
    TEST_ASSERT_EQUAL(12, stream[4].gather_dma_index);
}

void test_calculate_gather_indizes_null(void) {
    calculate_gather_indizes(NULL);
}

void test_calculate_scatter_indizes(void) {
	int i;
    struct stream_list streamlist = STREAMLIST_INITIALIZER(streamlist);
    struct acm_stream stream[6] = {
            STREAM_INITIALIZER(stream[0], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[2], REDUNDANT_STREAM_TX),
            STREAM_INITIALIZER(stream[3], TIME_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[4], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[5], INGRESS_TRIGGERED_STREAM)
    };

    struct operation scatter_op[9] = {
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL),
            READ_OPERATION_INITIALIZER(0, 0, NULL) };

    /* prepare test case */
    for (i = 0; i < 6; i++)
        ACMLIST_INSERT_TAIL(&streamlist, &stream[i], entry);

    ACMLIST_INSERT_TAIL(&stream[0].operations, &scatter_op[0], entry);
    ACMLIST_INSERT_TAIL(&stream[0].operations, &scatter_op[1], entry);
    ACMLIST_INSERT_TAIL(&stream[0].operations, &scatter_op[2], entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &scatter_op[3], entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &scatter_op[4], entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &scatter_op[5], entry);
    ACMLIST_INSERT_TAIL(&stream[1].operations, &scatter_op[6], entry);
    ACMLIST_INSERT_TAIL(&stream[4].operations, &scatter_op[7], entry);
    ACMLIST_INSERT_TAIL(&stream[4].operations, &scatter_op[8], entry);

    /* execute test case */
    calculate_scatter_indizes(&streamlist);
    TEST_ASSERT_EQUAL(1, stream[0].scatter_dma_index);
    TEST_ASSERT_EQUAL(4, stream[1].scatter_dma_index);
    TEST_ASSERT_EQUAL(0, stream[2].scatter_dma_index);
    TEST_ASSERT_EQUAL(0, stream[3].scatter_dma_index);
    TEST_ASSERT_EQUAL(8, stream[4].scatter_dma_index);
    TEST_ASSERT_EQUAL(0, stream[5].scatter_dma_index);
}

void test_calculate_scatter_indizes_null(void) {
    calculate_scatter_indizes(NULL);
}

void test_stream_in_list_null(void) {
    struct acm_stream stream;
    memset(&stream, 0, sizeof (stream));
    bool result;

    result = stream_in_list(NULL, &stream);
    TEST_ASSERT_FALSE(result);
}

void test_stream_num_scatter_ops(void) {
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

    result = stream_num_scatter_ops(&stream);
    TEST_ASSERT_EQUAL(1, result);
}

void test_stream_num_scatter_ops_null(void) {
    int result;

    result = stream_num_scatter_ops(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_stream_num_gather_ops(void) {
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
    operation[1].opcode = PAD;
    operation[3].opcode = FORWARD_ALL;
    operation[9].opcode = FORWARD;

    result = stream_num_gather_ops(&stream);
    TEST_ASSERT_EQUAL(9, result);
}

void test_stream_num_gather_ops_null(void) {
    int result;

    result = stream_num_gather_ops(NULL);
    TEST_ASSERT_EQUAL(0, result);
}

void test_stream_set_indiv_recov_stream_null(void) {
    int ret;

    logging_Expect(LOGLEVEL_ERR, "Stream: stream is null");
    ret = stream_set_indiv_recov(NULL, 100);
    TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_stream_set_indiv_recov_ingress_triggered_stream(void) {
    int ret;
    struct acm_stream stream = STREAM_INITIALIZER(stream, INGRESS_TRIGGERED_STREAM);

    ret = stream_set_indiv_recov(&stream, 100);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(100, stream.indiv_recov_timeout_ms);
}

void test_stream_set_indiv_recov_redundant_stream_rx(void) {
    int ret;
    struct acm_stream stream = STREAM_INITIALIZER(stream, REDUNDANT_STREAM_RX);

    ret = stream_set_indiv_recov(&stream, 100);
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(100, stream.indiv_recov_timeout_ms);
}

void test_stream_set_indiv_recov_event_stream(void) {
    int ret;
    struct acm_stream stream = STREAM_INITIALIZER(stream, EVENT_STREAM);

    logging_Expect(LOGLEVEL_ERR, "Stream: only allowed for Ingress Triggered streams. is %d");
    ret = stream_set_indiv_recov(&stream, 100);
    TEST_ASSERT_EQUAL(-EPERM, ret);
    TEST_ASSERT_EQUAL(0, stream.indiv_recov_timeout_ms);
}

void test_stream_set_indiv_recov_config_applied(void) {
    int ret;
    struct acm_config configuration = CONFIGURATION_INITIALIZER(configuration, true);
    struct acm_module module = MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0,
            &configuration);
    struct acm_stream stream = STREAM_INITIALIZER(stream, EVENT_STREAM);

    _ACMLIST_INSERT_TAIL(&module.streams, &stream, entry);

    logging_Expect(LOGLEVEL_ERR, "Stream: configuration of stream already applied to HW");
    ret = stream_set_indiv_recov(&stream, 100);
    TEST_ASSERT_EQUAL(-EPERM, ret);
    TEST_ASSERT_EQUAL(0, stream.indiv_recov_timeout_ms);
}

void test_stream_num_prefetch_ops_null(void) {
    int result;

    result = stream_num_prefetch_ops(NULL);
    TEST_ASSERT_EQUAL(0, result);
}
