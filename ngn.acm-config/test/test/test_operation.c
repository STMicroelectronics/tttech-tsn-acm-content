#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "unity.h"

#include "operation.h"
#include "tracing.h"

#include "mock_stream.h"
#include "mock_memory.h"
#include "mock_validate.h"
#include "mock_sysfs.h"
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

void test_operation_create_insert(void) {
    struct operation *result;
    struct operation operation;
    memset(&operation, 0, sizeof(operation));
    char *bufname = "buffer";

    check_buff_name_against_sys_devices_ExpectAndReturn(bufname, 0);
    acm_zalloc_ExpectAndReturn(sizeof (operation), &operation);
    acm_strdup_ExpectAndReturn("buffer", bufname);

    result = operation_create_insert(200, "buffer");
    TEST_ASSERT_EQUAL(INSERT, result->opcode);
    TEST_ASSERT_EQUAL(0, result->offset);
    TEST_ASSERT_EQUAL(200, result->length);
    TEST_ASSERT_EQUAL_STRING("buffer", result->buffer_name);
    TEST_ASSERT_EQUAL_PTR(NULL, result->data);
    TEST_ASSERT_EQUAL(0, result->data_size);
    TEST_ASSERT_NULL(ACMLIST_NEXT(result, entry));
}

void test_operation_create_insert_buffer_null(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: invalid buffer name - length");
    result = operation_create_insert(200, NULL);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert__empty_name(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: invalid buffer name - length");
    result = operation_create_insert(200, "");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert_big_name(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: invalid buffer name - length");
    result = operation_create_insert(200, "0123456789"
            "0123456789"
            "0123456789"
            "0123456789"
            "0123456789"
            "012345");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert_min_size(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");

    result = operation_create_insert(1, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert_max_size(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");

    result = operation_create_insert(1501, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert_no_mem(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    acm_zalloc_ExpectAndReturn(sizeof (struct operation), NULL);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_insert(250, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert_strdup_fail(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;
    const char *bufname = "buffer";

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_strdup_ExpectAndReturn(bufname, NULL);
    acm_free_Expect(&opmem);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_insert(250, bufname);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insert_check_fail(void) {
    struct operation *result;
    const char *bufname = "buffer";

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", -1);

    result = operation_create_insert(200, bufname);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insertconstant(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;
    const char *const_data = "abcdefghijklmnopqrstuvwxyz";
    char data_mem[40];
    memset(&data_mem, 0, sizeof(data_mem));

    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_zalloc_ExpectAndReturn(26, &data_mem);

    result = operation_create_insertconstant(const_data, 26);
    TEST_ASSERT_EQUAL(INSERT_CONSTANT, result->opcode);
    TEST_ASSERT_EQUAL(0, result->offset);
    TEST_ASSERT_EQUAL(26, result->length);
    TEST_ASSERT_EQUAL_PTR(NULL, result->buffer_name);
    TEST_ASSERT_EQUAL_PTR(&data_mem, result->data);
    TEST_ASSERT_EQUAL(26, result->data_size);
    TEST_ASSERT_NULL(ACMLIST_NEXT(result, entry));
}

void test_operation_create_insertconstant_min_size(void) {
    struct operation *result;
    const char *const_data = "abcdefghijklmnopqrstuvwxyz";

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_insertconstant(const_data, 0);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insertconstant_max_size(void) {
    struct operation *result;
    const char *const_data = "abcdefghijklmnopqrstuvwxyz";

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_insertconstant(const_data, 1501);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insertconstant_no_mem(void) {
    struct operation *result;
    const char *const_data = "abcdefghijklmnopqrstuvwxyz";

    acm_zalloc_ExpectAndReturn(sizeof (struct operation), NULL);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_insertconstant(const_data, 26);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_insertconstant_alloc_data_fail(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;
    const char *const_data = "abcdefghijklmnopqrstuvwxyz";

    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_zalloc_ExpectAndReturn(26, NULL);
    acm_free_Expect(&opmem);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_insertconstant(const_data, 26);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_pad(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;
    char data_mem;
    memset(&opmem, 0, sizeof(opmem));

    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_zalloc_ExpectAndReturn(1, &data_mem);

    result = operation_create_pad(200, 0xab);
    TEST_ASSERT_EQUAL(PAD, result->opcode);
    TEST_ASSERT_EQUAL(0, result->offset);
    TEST_ASSERT_EQUAL(200, result->length);
    TEST_ASSERT_EQUAL_PTR(NULL, result->buffer_name);
    TEST_ASSERT_EQUAL_PTR(&data_mem, result->data);
    TEST_ASSERT_EQUAL(1, result->data_size);
    TEST_ASSERT_NULL(ACMLIST_NEXT(result, entry));
}

void test_operation_create_pad_min_size(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_pad(0, 0xab);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_pad_max_size(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_pad(1501, 0xab);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_pad_no_mem(void) {
    struct operation *result;

    acm_zalloc_ExpectAndReturn(sizeof (struct operation), NULL);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_pad(200, 0xab);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_pad_alloc_data_fail(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;

    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_zalloc_ExpectAndReturn(1, NULL);
    acm_free_Expect(&opmem);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_pad(200, 0xab);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_forward(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;

    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);

    result = operation_create_forward(100, 200);
    TEST_ASSERT_EQUAL(FORWARD, result->opcode);
    TEST_ASSERT_EQUAL(100, result->offset);
    TEST_ASSERT_EQUAL(200, result->length);
    TEST_ASSERT_EQUAL_PTR(NULL, result->buffer_name);
    TEST_ASSERT_EQUAL_PTR(NULL, result->data);
    TEST_ASSERT_EQUAL(0, result->data_size);
    TEST_ASSERT_NULL(ACMLIST_NEXT(result, entry));
}

void test_operation_create_forward_min_size(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_forward(0, 1);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_forward_max_size(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_forward(0, 1509);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_forward_exceeding_framesize(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");
    result = operation_create_forward(100, 1429);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_forward_no_mem(void) {
    struct operation *result;

    acm_zalloc_ExpectAndReturn(sizeof(struct operation), NULL);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_forward(100, 200);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;
    char *bufname = "buffer";

    check_buff_name_against_sys_devices_ExpectAndReturn(bufname, 0);
    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_strdup_ExpectAndReturn("buffer", bufname);

    result = operation_create_read(100, 200, "buffer");
    TEST_ASSERT_EQUAL(READ, result->opcode);
    TEST_ASSERT_EQUAL(100, result->offset);
    TEST_ASSERT_EQUAL(200, result->length);
    TEST_ASSERT_EQUAL_PTR(bufname, result->buffer_name);
    TEST_ASSERT_EQUAL_PTR(NULL, result->data);
    TEST_ASSERT_EQUAL(0, result->data_size);
    TEST_ASSERT_NULL(ACMLIST_NEXT(result, entry));
}

void test_operation_create_read_buffer_null(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: invalid buffer name - length");
    result = operation_create_read(100, 200, NULL);

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_empty_name(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: invalid buffer name - length");
    result = operation_create_read(100, 200, "");

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_name_too_long(void) {
    struct operation *result;

    logging_Expect(LOGLEVEL_ERR, "Operation: invalid buffer name - length");
    result = operation_create_read(100,
            200,
            "long_buffer_name_0123456789012345678901234567890123456789");

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_min_size(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");

    result = operation_create_read(100, 3, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_max_size(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");

    result = operation_create_read(0, 1529, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_exceeding_framesize(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    logging_Expect(LOGLEVEL_ERR, "Operation: Invalid size");

    result = operation_create_read(1, 1528, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_no_mem(void) {
    struct operation *result;

    check_buff_name_against_sys_devices_ExpectAndReturn("buffer", 0);
    acm_zalloc_ExpectAndReturn(sizeof (struct operation), NULL);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_read(100, 200, "buffer");
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_strdup_fails(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;
    const char *bufname = "buffer";

    check_buff_name_against_sys_devices_ExpectAndReturn(bufname, 0);
    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);
    acm_strdup_ExpectAndReturn(bufname, NULL);
    acm_free_Expect(&opmem);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_read(100, 200, bufname);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_read_check_fails(void) {
    struct operation *result;
    const char *bufname = "buffer";

    check_buff_name_against_sys_devices_ExpectAndReturn(bufname, -1);

    result = operation_create_read(100, 200, bufname);
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_create_forward_all(void) {
    struct operation opmem;
    memset(&opmem, 0, sizeof(opmem));
    struct operation *result;

    acm_zalloc_ExpectAndReturn(sizeof (opmem), &opmem);

    result = operation_create_forwardall();
    TEST_ASSERT_EQUAL(FORWARD_ALL, result->opcode);
    TEST_ASSERT_EQUAL(0, result->offset);
    TEST_ASSERT_EQUAL(0, result->length);
    TEST_ASSERT_EQUAL_PTR(NULL, result->buffer_name);
    TEST_ASSERT_EQUAL_PTR(NULL, result->data);
    TEST_ASSERT_EQUAL(0, result->data_size);
    TEST_ASSERT_NULL(ACMLIST_NEXT(result, entry));
}

void test_operation_create_forward_all_no_mem(void) {
    struct operation *result;

    acm_zalloc_ExpectAndReturn(sizeof (struct operation), NULL);
    logging_Expect(LOGLEVEL_ERR, "Operation: Out of memory");

    result = operation_create_forwardall();
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_operation_destroy(void) {
    struct operation operation = FORWARD_OPERATION_INITIALIZER(22, 12);

    acm_free_Expect(operation.buffer_name);
    acm_free_Expect(operation.data);
    acm_free_Expect(&operation);

    operation_destroy(&operation);
}

void test_operation_destroy_null(void) {
    operation_destroy(NULL);
}
void test_operation_init_list(void) {
    int result;
    struct operation_list oplist;
    memset(&oplist, 0xAA, sizeof (oplist));

    result = operation_list_init(&oplist);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&oplist));
    TEST_ASSERT(ACMLIST_EMPTY(&oplist));
    TEST_ASSERT_EQUAL(0, ACMLIST_LOCK(&oplist));

    ACMLIST_UNLOCK(&oplist);
}

void test_operation_init_list_null(void) {
    int result;

    result = operation_list_init(NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_operation_add_list(void)
{
    int i;
    struct operation *operation;
    const int max_ops = 10;
    char pad_data[10] = { 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab, 0xab };
    struct operation op_mem[10] = {
            PAD_OPERATION_INITIALIZER(10, &pad_data[0]),
            PAD_OPERATION_INITIALIZER(11, &pad_data[1]),
            PAD_OPERATION_INITIALIZER(12, &pad_data[2]),
            PAD_OPERATION_INITIALIZER(13, &pad_data[3]),
            PAD_OPERATION_INITIALIZER(14, &pad_data[4]),
            PAD_OPERATION_INITIALIZER(15, &pad_data[5]),
            PAD_OPERATION_INITIALIZER(16, &pad_data[6]),
            PAD_OPERATION_INITIALIZER(17, &pad_data[7]),
            PAD_OPERATION_INITIALIZER(18, &pad_data[8]),
            PAD_OPERATION_INITIALIZER(19, &pad_data[9])
    };
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream.operations));

    /* execute test case */
    for (i = 0; i < max_ops; ++i) {
        int result;
        validate_stream_ExpectAndReturn(operationlist_to_stream(&stream.operations), false, 0);
        result = operation_list_add_operation(&stream.operations, &op_mem[i]);
        TEST_ASSERT_EQUAL(0, result);
        TEST_ASSERT_EQUAL(i + 1, ACMLIST_COUNT(&stream.operations));
    }

    /* check if we have all operations in list */
    i = 0;
    ACMLIST_FOREACH(operation, &stream.operations, entry)
    {
        TEST_ASSERT_EQUAL(PAD, operation->opcode);
        TEST_ASSERT_EQUAL(10 + i, operation->length);
        TEST_ASSERT_EQUAL_PTR(&pad_data[i], operation->data);
        i++;
    }
}

void test_operation_add_list_list_null(void) {
    struct operation opmem = FORWARD_OPERATION_INITIALIZER(10, 20);
    int result;

    logging_Expect(LOGLEVEL_ERR, "Operation: wrong parameter in %s");
    result = operation_list_add_operation(NULL, &opmem);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_operation_add_list_op_null(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);

    logging_Expect(LOGLEVEL_ERR, "Operation: wrong parameter in %s");
    result = operation_list_add_operation(&stream.operations, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream.operations));
}

void test_operation_add_list_two_times_same_op(void) {
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation operation = FORWARD_OPERATION_INITIALIZER(10, 20);
    int result;

    /* prepare test case */
    validate_stream_ExpectAndReturn(operationlist_to_stream(&stream.operations), false, 0);
    result = operation_list_add_operation(&stream.operations, &operation);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&stream.operations));

    /* execute test case */
    logging_Expect(LOGLEVEL_ERR, "Operation: cannot be added multiple times");
    result = operation_list_add_operation(&stream.operations, &operation);
    TEST_ASSERT_EQUAL(-EPERM, result);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&stream.operations));
}

void test_operation_add_list_two_times_same_buffer(void) {
    int result;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    char *buffername1 = "buffer1";
    char *buffername2 = "buffer1";
    struct operation operation1 = INSERT_OPERATION_INITIALIZER(10, buffername1, NULL);
    struct operation operation2 = INSERT_OPERATION_INITIALIZER(20, buffername2, NULL);

    /* prepare test case */
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream.operations));

    validate_stream_ExpectAndReturn(operationlist_to_stream(&stream.operations), false, 0);
    result = operation_list_add_operation(&stream.operations, &operation1);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&stream.operations));

    /* execute test case */
    validate_stream_ExpectAndReturn(operationlist_to_stream(&stream.operations), false, -EPERM);
    result = operation_list_add_operation(&stream.operations, &operation2);
    TEST_ASSERT_EQUAL(-EPERM, result);
    TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&stream.operations));
}

void test_operation_list_remove_operation(void) {
    int i;
    const int max_ops = 10;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation operation[10] = {
            PAD_OPERATION_INITIALIZER(7, NULL),
            INSERT_OPERATION_INITIALIZER(20, "buffer1", NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 10),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 11),
            PAD_OPERATION_INITIALIZER(5, NULL),
            PAD_OPERATION_INITIALIZER(5, NULL),
            PAD_OPERATION_INITIALIZER(9, NULL),
            INSERT_OPERATION_INITIALIZER(20, "buffer3", NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 12),
            INSERT_OPERATION_INITIALIZER(20, "buffer3", NULL)
   };

    /* prepare test case*/
    for (i = 0; i < max_ops; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &operation[i], entry);
    }
    TEST_ASSERT_EQUAL(max_ops, ACMLIST_COUNT(&stream.operations));
    TEST_ASSERT_EQUAL_PTR(&operation[5], ACMLIST_NEXT(&operation[4], entry));

    /* execute test case*/
    operation_list_remove_operation(&stream.operations, &operation[5]);
    TEST_ASSERT_EQUAL(max_ops-1, ACMLIST_COUNT(&stream.operations));
    TEST_ASSERT_EQUAL_PTR(&operation[6], ACMLIST_NEXT(&operation[4], entry));
}

void test_operation_empty_list_null(void) {
    logging_Expect(LOGLEVEL_ERR, "Operation: wrong parameter in %s");
    operation_list_flush(NULL);
}

void test_operation_empty_list(void) {
    int i;
    const int max_ops = 10;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation operation[10] = {
            PAD_OPERATION_INITIALIZER(7, NULL),
            INSERT_OPERATION_INITIALIZER(20, "buffer1", NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 10),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 11),
            PAD_OPERATION_INITIALIZER(5, NULL),
            PAD_OPERATION_INITIALIZER(5, NULL),
            PAD_OPERATION_INITIALIZER(9, NULL),
            INSERT_OPERATION_INITIALIZER(20, "buffer3", NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(NULL, 12),
            INSERT_OPERATION_INITIALIZER(20, "buf_name", NULL)
   };

    /* prepare test case*/
    for (i = 0; i < max_ops; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &operation[i], entry);
    }
    TEST_ASSERT_EQUAL(max_ops, ACMLIST_COUNT(&stream.operations));
    TEST_ASSERT_EQUAL_PTR(&operation[0], ACMLIST_FIRST(&stream.operations));
    TEST_ASSERT_EQUAL_PTR(&operation[9], ACMLIST_LAST(&stream.operations, operation_list));

    for (i = 0; i < max_ops; i++) {
        acm_free_Expect(operation[i].data);
        acm_free_Expect(operation[i].buffer_name);
        acm_free_Expect(&operation[i]);
    }
    operation_list_flush(&stream.operations);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream.operations));
}

void test_operation_empty_user_list_stream_with_useroperations(void) {
    int i;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation opforward[3] = {
            FORWARD_OPERATION_INITIALIZER(40, 15),
            FORWARD_OPERATION_INITIALIZER(30, 10),
            FORWARD_OPERATION_INITIALIZER(20, 9),};
    char data[4] = {0xAA, 0xCC, 0xFF, 0x13};
    struct operation op_pad[4] = {
            PAD_OPERATION_INITIALIZER(5, &data[0]),
            PAD_OPERATION_INITIALIZER(6, &data[1]),
            PAD_OPERATION_INITIALIZER(7, &data[2]),
            PAD_OPERATION_INITIALIZER(8, &data[3])};

    /* prepare test case */
    for (i = 0; i < 3; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &opforward[i], entry);
    }

    for (i = 0; i < 4; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &op_pad[i], entry);
    }

    TEST_ASSERT_EQUAL(4 + 3, ACMLIST_COUNT(&stream.operations));

    /* execute test case */
    for (i = 0; i < 4; i++) {
        acm_free_Expect(&data[i]);
        acm_free_Expect(NULL);
        acm_free_Expect(&op_pad[i]);
    }
    operation_list_flush_user(&stream.operations);
    TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&stream.operations));
    TEST_ASSERT_EQUAL(&opforward[0], ACMLIST_FIRST(&stream.operations));
    TEST_ASSERT_EQUAL(&opforward[1], ACMLIST_NEXT(&opforward[0], entry));
    TEST_ASSERT_EQUAL(&opforward[2], ACMLIST_NEXT(&opforward[1], entry));
    for (i = 0; i < 3; i++) {
        acm_free_Expect(NULL);
        acm_free_Expect(NULL);
        acm_free_Expect(&opforward[i]);
    }
    operation_list_flush(&stream.operations);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream.operations));
}

void test_operation_empty_user_list_without_useroperations(void) {
    int i;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation opforward[3] = {
            FORWARD_OPERATION_INITIALIZER(40, 15),
            FORWARD_OPERATION_INITIALIZER(30, 10),
            FORWARD_OPERATION_INITIALIZER(20, 9),};

    /* prepare test case */
    for (i = 0; i < 3; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &opforward[i], entry);
    }

    TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&stream.operations));

    /* execute test case */
    operation_list_flush_user(&stream.operations);
    TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&stream.operations));
    TEST_ASSERT_EQUAL(&opforward[0], ACMLIST_FIRST(&stream.operations));
    TEST_ASSERT_EQUAL(&opforward[1], ACMLIST_NEXT(&opforward[0], entry));
    TEST_ASSERT_EQUAL(&opforward[2], ACMLIST_NEXT(&opforward[1], entry));
    for (i = 0; i < 3; i++) {
        acm_free_Expect(NULL);
        acm_free_Expect(NULL);
        acm_free_Expect(&opforward[i]);
    }
    operation_list_flush(&stream.operations);
    TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&stream.operations));
}


void test_operation_list_update_smac_module_0(void) {
    int ret;
    int i;
    const int max_ops = 5;
    char const_data[] = LOCAL_SMAC_CONST;

    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation opmem[5] = {
            PAD_OPERATION_INITIALIZER(50, NULL),
            PAD_OPERATION_INITIALIZER(51, NULL),
            PAD_OPERATION_INITIALIZER(52, NULL),
            PAD_OPERATION_INITIALIZER(53, NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(const_data, sizeof(LOCAL_SMAC_CONST))
    };
    uint8_t mac_data[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

    /* prepare test case */
    for (i = 0; i < 5; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &opmem[i], entry);
    }
    TEST_ASSERT_EQUAL(max_ops, ACMLIST_COUNT(&stream.operations));

    /* execute test case */
    get_mac_address_ExpectAndReturn(PORT_MODULE_0, (uint8_t*)mac_data, 0);
    get_mac_address_IgnoreArg_mac();
    get_mac_address_ReturnMemThruPtr_mac(mac_data, sizeof(mac_data)+1);
    ret = operation_list_update_smac(&stream.operations, MODULE_0);
    TEST_ASSERT_EQUAL(0,ret);
    /* was the smac updated? */
    TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*) (opmem[4].data), (uint8_t*)mac_data, 6);
}

void test_operation_list_update_smac_module_1(void) {
    int ret;
    int i;
    const int max_ops = 5;
    char const_data[] = LOCAL_SMAC_CONST;
    char mac_set[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa };
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    struct operation opmem[5] = {
            PAD_OPERATION_INITIALIZER(50, NULL),
            PAD_OPERATION_INITIALIZER(51, NULL),
            PAD_OPERATION_INITIALIZER(52, NULL),
            INSERT_CONSTANT_OPERATION_INITIALIZER(const_data, sizeof(LOCAL_SMAC_CONST)),
            INSERT_CONSTANT_OPERATION_INITIALIZER(mac_set, sizeof(mac_set))
    };
    char mac_data[] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };

    /* prepare test case */
    for (i = 0; i < 5; i++) {
        ACMLIST_INSERT_TAIL(&stream.operations, &opmem[i], entry);
    }
    TEST_ASSERT_EQUAL(max_ops, ACMLIST_COUNT(&stream.operations));


    /* execute test case */
    get_mac_address_ExpectAndReturn(PORT_MODULE_1, (uint8_t*)mac_data, 0);
    get_mac_address_IgnoreArg_mac();
    get_mac_address_ReturnMemThruPtr_mac(mac_data, sizeof(mac_data)+1);
    ret = operation_list_update_smac(&stream.operations, MODULE_1);
    TEST_ASSERT_EQUAL(0,ret);
    /* was the smac opmem[3] updated? */
    TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*) (opmem[3].data), (uint8_t*)mac_data, 6);
    /* smac opmem[4] unchanged? */
    TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*) (opmem[4].data), (uint8_t*)mac_set, 6);
}

void test_operation_list_update_smac_invalid_module(void) {
    int ret;
    struct acm_stream stream = STREAM_INITIALIZER(stream, TIME_TRIGGERED_STREAM);
    logging_Expect(LOGLEVEL_ERR, "Operation: problem reading MAC address of module");
    ret = operation_list_update_smac(&stream.operations, 50);
    TEST_ASSERT_EQUAL(-EACMINTERNAL, ret);
}
