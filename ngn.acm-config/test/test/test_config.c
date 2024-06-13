#include "unity.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "config.h"
#include "tracing.h"

#include "mock_logging.h"
#include "mock_memory.h"
#include "mock_module.h"
#include "mock_application.h"
#include "mock_validate.h"
#include "mock_sysfs.h"
#include "mock_buffer.h"
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

void test_config_create(void) {
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_config *result;

    acm_zalloc_ExpectAndReturn(sizeof (config), &config);
    buffer_init_list_ExpectAndReturn(&config.msg_buffs, 0);

    result = config_create();
    TEST_ASSERT_EQUAL_PTR(&config, result);
}

void test_config_create_null(void) {
    struct acm_config *result;

    acm_zalloc_ExpectAndReturn(sizeof(struct acm_config), NULL);
    logging_Expect(0, "Config: Out of memory");
    result = config_create();
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_config_create_neg_bufferinitlist(void) {
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_config *result;

    acm_zalloc_ExpectAndReturn(sizeof (config), &config);
    buffer_init_list_ExpectAndReturn(&config.msg_buffs, -EINVAL);

    result = config_create();
    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_config_destroy(void) {
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    module_destroy_Expect(config.bypass[0]);
    module_destroy_Expect(config.bypass[1]);
    buffer_empty_list_Expect(&config.msg_buffs);
    acm_free_Expect(&config);

    config_destroy(&config);
}

void test_config_destroy_NULL(void) {
    config_destroy(NULL);
}

void test_config_add_module(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    validate_config_ExpectAndReturn(&config, false, 0);

    result = config_add_module(&config, &module);
    TEST_ASSERT_EQUAL(0, result);
}

void test_config_add_module__module_null(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    logging_Expect(0, "Config: Invalid module input");

    result = config_add_module(&config, NULL);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_add_module_config_null(void) {
    int result;
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    logging_Expect(0, "Config: Invalid module input");

    result = config_add_module(NULL, &module);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_add_module_config_applied(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, true);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);

    logging_Expect(0, "Config: Configuration already applied to ACM HW");

    result = config_add_module(&config, &module);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_add_module_already_added_to_other_config(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_config other_config = CONFIGURATION_INITIALIZER(other_config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    /* prepare test case */
    validate_config_ExpectAndReturn(&other_config, false, 0);
    TEST_ASSERT_EQUAL(0, config_add_module(&other_config, &module));

    /* execute test case */
    logging_Expect(0, "Config: Module already added to other configuration");

    result = config_add_module(&config, &module);
    TEST_ASSERT_EQUAL(-EINVAL, result);

}

void test_config_add_module_invalid_module_id(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_1 + 2, &config);

    logging_Expect(0, "Config: Invalid module id");
    result = config_add_module(&config, &module);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_add_module_twice(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    /* prepare test case */
    validate_config_ExpectAndReturn(&config, false, 0);
    TEST_ASSERT_EQUAL(0, config_add_module(&config, &module));

    /* execute test case */
    logging_Expect(0, "Config: Configuration already has a module with this id configured");
    result = config_add_module(&config, &module);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_add_module_validate_fail(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, NULL);

    validate_config_ExpectAndReturn(&config, false, -EINVAL);
    result = config_add_module(&config, &module);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_enable_validate_okay(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    validate_config_ExpectAndReturn(&config, true, 0);
    apply_configuration_ExpectAndReturn(&config, 100, 0);
    sysfs_write_configuration_id_ExpectAndReturn(100, 0);

    result = config_enable(&config, 100);
    TEST_ASSERT_EQUAL(0, result);
}

void test_config_enable_identifier_zero(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    logging_Expect(0, "Config: Configuration identifier 0 not allowed");

    result = config_enable(&config, 0);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_enable_validate_not_okay(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    validate_config_ExpectAndReturn(&config, true, -EACMOPMISSING);
    logging_Expect(0, "Config: final validation before applying config to HW failed");

    result = config_enable(&config, 100);
    TEST_ASSERT_EQUAL(-EACMOPMISSING, result);
}

void test_config_enable_neg_apply_config(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    validate_config_ExpectAndReturn(&config, true, 0);
    apply_configuration_ExpectAndReturn(&config, 100, -ENOMEM);
    logging_Expect(0, "Config: applying configuration to HW failed");

    result = config_enable(&config, 100);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_config_enable_neg_write_config_id(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    validate_config_ExpectAndReturn(&config, true, 0);
    apply_configuration_ExpectAndReturn(&config, 100, 0);
    sysfs_write_configuration_id_ExpectAndReturn(100, -ENOMEM);

    result = config_enable(&config, 100);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_config_enable_null(void) {
    int result;

    logging_Expect(0, "Config: Configuration not defined");

    result = config_enable(NULL, 100);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_schedule(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    sysfs_read_configuration_id_ExpectAndReturn(200);
    validate_config_ExpectAndReturn(&config, true, 0);
    apply_schedule_ExpectAndReturn(&config, 0);
    sysfs_write_configuration_id_ExpectAndReturn(100, 0);

    result = config_schedule(&config, 100, 200);
    TEST_ASSERT_EQUAL(0, result);
}

void test_config_schedule_null(void) {
    int result;

    logging_Expect(0, "Config: Configuration not defined");

    result = config_schedule(NULL, 200, 100);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_schedule_identifier_zero(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    logging_Expect(0, "Config: Configuration identifier 0 not allowed");

    result = config_schedule(&config, 0, 100);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_schedule_neg_read_config_id(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    sysfs_read_configuration_id_ExpectAndReturn(-ENOMEM);

    result = config_schedule(&config, 100, 200);
    TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_config_schedule_neg_diff_expected_id(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    sysfs_read_configuration_id_ExpectAndReturn(220);
    logging_Expect(0, "Config: read identifier %d not equal expected identifier %d");

    result = config_schedule(&config, 100, 200);
    TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_config_schedule_neg_validate_config(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    sysfs_read_configuration_id_ExpectAndReturn(200);
    validate_config_ExpectAndReturn(&config, true, -EACMOPMISSING);
    logging_Expect(0, "Config: final validation before applying schedule to HW failed");

    result = config_schedule(&config, 100, 200);
    TEST_ASSERT_EQUAL(-EACMOPMISSING, result);
}

void test_config_schedule_neg_apply_schedule(void) {
    int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);

    sysfs_read_configuration_id_ExpectAndReturn(200);
    validate_config_ExpectAndReturn(&config, true, 0);
    apply_schedule_ExpectAndReturn(&config, -EACMNOFREESCHEDTAB);
    logging_Expect(0, "Config: applying schedule to HW failed");

    result = config_schedule(&config, 100, 200);
    TEST_ASSERT_EQUAL(-EACMNOFREESCHEDTAB, result);
}

void test_config_disable(void) {
    int result;

    remove_configuration_ExpectAndReturn(0);
    result = config_disable();
    TEST_ASSERT_EQUAL(0, result);
}

void test_clean_and_recalculate_hw_msg_buffs(void) {
	int result;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
	struct acm_stream ingress, event, recovery;
	memset(&ingress, 0, sizeof(ingress));
	memset(&event, 0, sizeof(event));
	memset(&recovery, 0, sizeof(recovery));
	struct operation read_op;
	memset(&read_op, 0, sizeof(read_op));
	char buffername[] = "special name";
	struct sysfs_buffer new_msg_buf, *reusable;
	memset(&new_msg_buf, 0, sizeof(new_msg_buf));
	bool reuse_flag = false;

	/* prepare testcase */
	config.bypass[1] = &module;
	/*initialize msg buffer list of config */
	ACMLIST_INIT(&config.msg_buffs);
	/*initialize stream list of module */
	ACMLIST_INIT(&module.streams);
	/*initialize operation lists of streams */
	ACMLIST_INIT(&ingress.operations);
	ACMLIST_INIT(&event.operations);
	ACMLIST_INIT(&recovery.operations);
	/* set stream types and references of streams */
	ingress.type = INGRESS_TRIGGERED_STREAM;
	event.type = EVENT_STREAM;
	recovery.type = RECOVERY_STREAM;
	ingress.reference = &event;
	event.reference = &recovery;
	event.reference_parent = &ingress;
	recovery.reference_parent = &event;
	ACMLIST_INSERT_TAIL(&module.streams, &ingress, entry);
	ACMLIST_INSERT_TAIL(&module.streams, &event, entry);
	ACMLIST_INSERT_TAIL(&module.streams, &recovery, entry);
	/* initialize operation and add it to operation list of event stream */
	read_op.opcode = READ;
	read_op.length = 40;
	read_op.offset = 10;
	read_op.buffer_name = buffername;
	ACMLIST_INSERT_TAIL(&event.operations, &read_op, entry);
	/* initialize item of type sysfs_buffer as it would be done by buffer_create*/
	new_msg_buf.msg_buff_name = strdup(read_op.buffer_name);
	new_msg_buf.msg_buff_index = 0;
	new_msg_buf.msg_buff_offset = 0;
	new_msg_buf.reset = false;
	new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
	new_msg_buf.buff_size = read_op.length/4;
	new_msg_buf.timestamp = true;
	new_msg_buf.valid = true;

	/* execute testcase */
	module_clean_msg_buff_links_Expect(&module);
	buffer_empty_list_Expect(&config.msg_buffs);
	get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
	buffer_create_ExpectAndReturn(&read_op, 0, 0, (read_op.length/4)+1,
			&new_msg_buf);
	buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
			&reuse_flag, &reusable, 0);
	buffername_check_IgnoreArg_reuse_msg_buff();
	buffer_add_list_Expect(&config.msg_buffs, &new_msg_buf);
	get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE),
			16384);
	result = clean_and_recalculate_hw_msg_buffs(&config);
	TEST_ASSERT_EQUAL(0, result);
}

/* check config global buffer index and offset using a single module */
void test_create_hw_msg_buf_list_one_module(void) {
    int ret;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module =
            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
    struct acm_stream stream[2] = {
            STREAM_INITIALIZER(stream[0], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], INGRESS_TRIGGERED_STREAM),
    };
    struct operation operation[2] = {
            READ_OPERATION_INITIALIZER(10, 100, "acm_rx0"),
            READ_OPERATION_INITIALIZER(20, 300, "acm_rx1"),
    };
    /* do not care for buffer, just for reference */
    struct sysfs_buffer buffer =
        BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL);

    /* add operations to stream */
    _ACMLIST_INSERT_TAIL(&stream[0].operations, &operation[0], entry);
    _ACMLIST_INSERT_TAIL(&stream[1].operations, &operation[1], entry);

    /* add streams to module */
    _ACMLIST_INSERT_TAIL(&module.streams, &stream[0], entry);
    _ACMLIST_INSERT_TAIL(&module.streams, &stream[1], entry);

    config.bypass[0] = &module;

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
    buffer_create_ExpectAndReturn(&operation[0], 0, 0, operation[0].length / 4 + 1, &buffer);
    buffername_check_ExpectAndReturn(&config.msg_buffs, &buffer, NULL, NULL, 0);
    buffername_check_IgnoreArg_reuse();
    buffername_check_IgnoreArg_reuse_msg_buff();
    buffer_add_list_Expect(&config.msg_buffs, &buffer);
    buffer_create_ExpectAndReturn(&operation[1], 1, operation[0].length / 4 + 1,
            operation[1].length / 4 + 1, &buffer);
    buffername_check_ExpectAndReturn(&config.msg_buffs, &buffer, NULL, NULL, 0);
    buffername_check_IgnoreArg_reuse();
    buffername_check_IgnoreArg_reuse_msg_buff();
    buffer_add_list_Expect(&config.msg_buffs, &buffer);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE), 16384);
    ret = create_hw_msg_buf_list(&config);
    TEST_ASSERT_EQUAL(0, ret);
}

/* check config global buffer index and offset using both modules */
void test_create_hw_msg_buf_list_two_modules(void) {
    int ret;
    struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
    struct acm_module module[2] = {
            MODULE_INITIALIZER(module[0], CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config),
            MODULE_INITIALIZER(module[1], CONN_MODE_SERIAL, SPEED_100MBps, MODULE_1, &config)
    };
    struct acm_stream stream[2] = {
            STREAM_INITIALIZER(stream[0], INGRESS_TRIGGERED_STREAM),
            STREAM_INITIALIZER(stream[1], INGRESS_TRIGGERED_STREAM),
    };
    struct operation operation[2] = {
            READ_OPERATION_INITIALIZER(10, 100, "acm_rx0"),
            READ_OPERATION_INITIALIZER(20, 300, "acm_rx1"),
    };
    /* do not care for buffer, just for reference */
    struct sysfs_buffer buffer =
        BUFFER_INITIALIZER(0, 0, false, ACMDRV_BUFF_DESC_BUFF_TYPE_RX, 0, false, false, NULL);

    /* add operations to stream */
    _ACMLIST_INSERT_TAIL(&stream[0].operations, &operation[0], entry);
    _ACMLIST_INSERT_TAIL(&stream[1].operations, &operation[1], entry);

    /* add streams to modules */
    _ACMLIST_INSERT_TAIL(&module[0].streams, &stream[0], entry);
    _ACMLIST_INSERT_TAIL(&module[1].streams, &stream[1], entry);

    /* join modules to configuration */
    config.bypass[0] = &module[0];
    config.bypass[1] = &module[1];

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
    buffer_create_ExpectAndReturn(&operation[0], 0, 0, operation[0].length / 4 + 1, &buffer);
    buffername_check_ExpectAndReturn(&config.msg_buffs, &buffer, NULL, NULL, 0);
    buffername_check_IgnoreArg_reuse();
    buffername_check_IgnoreArg_reuse_msg_buff();
    buffer_add_list_Expect(&config.msg_buffs, &buffer);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE), 16384);
    buffer_create_ExpectAndReturn(&operation[1], 1, operation[0].length / 4 + 1,
            operation[1].length / 4 + 1, &buffer);
    buffername_check_ExpectAndReturn(&config.msg_buffs, &buffer, NULL, NULL, 0);
    buffername_check_IgnoreArg_reuse();
    buffername_check_IgnoreArg_reuse_msg_buff();
    buffer_add_list_Expect(&config.msg_buffs, &buffer);
    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE), 16384);
    ret = create_hw_msg_buf_list(&config);
    TEST_ASSERT_EQUAL(0, ret);
}

void test_create_hw_msg_buf_list_insert_operation(void){
		int result;
		struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
		struct acm_module module =
	            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
		struct acm_stream ingress;
		memset(&ingress, 0, sizeof(ingress));
		struct operation insert_op;
		memset(&insert_op, 0, sizeof(insert_op));
		char buffername[] = "special name";
		struct sysfs_buffer new_msg_buf, *reusable;
		memset(&new_msg_buf, 0, sizeof(new_msg_buf));
		bool reuse_flag = false;

		/* prepare testcase */
		config.bypass[1] = &module;
		/*initialize msg buffer list of config */
		ACMLIST_INIT(&config.msg_buffs);
		/*initialize stream list of module */
		ACMLIST_INIT(&module.streams);
		/*initialize operation lists of streams */
		ACMLIST_INIT(&ingress.operations);
		/* set stream types and references of streams */
		ingress.type = INGRESS_TRIGGERED_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &ingress, entry);
		/* initialize operation and add it to operation list of event stream */
		insert_op.opcode = INSERT;
		insert_op.length = 45;
		insert_op.offset = 10;
		insert_op.buffer_name = buffername;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op, entry);
		/* initialize item of type sysfs_buffer as it would be done by buffer_create*/
		new_msg_buf.msg_buff_name = strdup(insert_op.buffer_name);
		new_msg_buf.msg_buff_index = 0;
		new_msg_buf.msg_buff_offset = 0;
		new_msg_buf.reset = false;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg_buf.buff_size = insert_op.length/4 + 1;
		new_msg_buf.timestamp = true;
		new_msg_buf.valid = true;

		/* execute testcase */
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
		buffer_create_ExpectAndReturn(&insert_op, 0, 0, (insert_op.length/4 + 1),
				&new_msg_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffer_add_list_Expect(&config.msg_buffs, &new_msg_buf);
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE),
				16384);
		result = create_hw_msg_buf_list(&config);
		TEST_ASSERT_EQUAL(0, result);
}

void test_create_hw_msg_buf_list_several_ops(void){
		int result;
		struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
		struct acm_module module =
	            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
		struct acm_stream ingress, event;
		memset(&ingress, 0, sizeof(ingress));
		memset(&event, 0, sizeof(event));
		struct operation insert_op1, insert_op2, read_op, pad_op;
		memset(&insert_op1, 0, sizeof(insert_op1));
		memset(&insert_op2, 0, sizeof(insert_op2));
		memset(&read_op, 0, sizeof(read_op));
		memset(&pad_op, 0, sizeof(pad_op));
		char buffernameread[] = "special name";
		char buffername_i1[] = "name_insert";
		char buffername_i2[] = "name_insert";
		struct sysfs_buffer new_msg_buf, new_msg2_buf, *reusable;
		memset(&new_msg_buf, 0, sizeof(new_msg_buf));
		bool reuse_flag = false;

		/* prepare testcase */
		config.bypass[1] = &module;
		/*initialize msg buffer list of config */
		ACMLIST_INIT(&config.msg_buffs);
		/*initialize stream list of module */
		ACMLIST_INIT(&module.streams);
		/*initialize operation lists of streams */
		ACMLIST_INIT(&ingress.operations);
		ACMLIST_INIT(&event.operations);
		/* set stream types and references of streams */
		event.type = EVENT_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &event, entry);
		ingress.type = INGRESS_TRIGGERED_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &ingress, entry);
		event.reference = &ingress;
		ingress.reference_parent = &event;
		/* initialize operation and add it to operation list of event stream */
		read_op.opcode = READ;
		read_op.length = 400;
		read_op.offset = 10;
		read_op.buffer_name = buffernameread;
		ACMLIST_INSERT_TAIL(&event.operations, &read_op, entry);
		/* initialize operations and add them to operation list of ingress
		 * triggered stream stream */
		insert_op1.opcode = INSERT;
		insert_op1.length = 777;
		insert_op1.offset = 10;
		insert_op1.buffer_name = buffername_i1;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op1, entry);
		pad_op.opcode = PAD;
		pad_op.length = 777;
		pad_op.offset = 10;
		ACMLIST_INSERT_TAIL(&ingress.operations, &pad_op, entry);
		insert_op2.opcode = INSERT;
		insert_op2.length = 287;
		insert_op2.offset = 50;
		insert_op2.buffer_name = buffername_i2;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op2, entry);
		/* initialize item of type sysfs_buffer as it would be done by buffer_create*/
		new_msg_buf.msg_buff_name = strdup(read_op.buffer_name);
		new_msg_buf.msg_buff_index = 0;
		new_msg_buf.msg_buff_offset = 0;
		new_msg_buf.reset = false;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
		new_msg_buf.buff_size = read_op.length/4;
		new_msg_buf.timestamp = true;
		new_msg_buf.valid = true;

		/* execute testcase */
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
		buffer_create_ExpectAndReturn(&read_op, 0, 0, (read_op.length/4 + 1),
				&new_msg_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffername_check_IgnoreArg_reuse();
		buffer_add_list_Expect(&config.msg_buffs, &new_msg_buf);

		free(new_msg_buf.msg_buff_name);
		new_msg_buf.msg_buff_name = strdup(insert_op1.buffer_name);
		new_msg_buf.msg_buff_index = 1;
		new_msg_buf.msg_buff_offset = read_op.length/4 + 1;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg_buf.buff_size = insert_op1.length/4 + 1;
		buffer_create_ExpectAndReturn(&insert_op1, 1, read_op.length/4 + 1,
				(insert_op1.length/4 + 1), &new_msg_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffername_check_IgnoreArg_reuse();
		buffer_add_list_Expect(&config.msg_buffs, &new_msg_buf);

		new_msg2_buf.msg_buff_name = strdup(insert_op2.buffer_name);
		new_msg2_buf.msg_buff_index = 2;
		new_msg2_buf.msg_buff_offset = read_op.length/4 + 1 + insert_op1.length/4 + 1;
		new_msg2_buf.reset = false;
		new_msg2_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg2_buf.buff_size = insert_op2.length/4 + 1;
		new_msg2_buf.timestamp = true;
		new_msg2_buf.valid = true;
		reusable = &new_msg_buf;
		reuse_flag = true;
		buffer_create_ExpectAndReturn(&insert_op2, 2, new_msg2_buf.msg_buff_offset,
				(insert_op2.length/4 + 1), &new_msg2_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg2_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffername_check_ReturnMemThruPtr_reuse_msg_buff(&reusable, sizeof(reusable));
		buffername_check_IgnoreArg_reuse();
		buffername_check_ReturnMemThruPtr_reuse(&reuse_flag, sizeof(reuse_flag));
		buffer_destroy_Expect(&new_msg2_buf);

		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE),
				1000);
		logging_Expect(0, "Config: configured message buffers %d bigger than available %d");
		result = create_hw_msg_buf_list(&config);
		TEST_ASSERT_EQUAL(-EPERM, result);

 }

void test_create_hw_msg_buf_list_several_ops_change_reused(void){
		int result;
		struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
		struct acm_module module =
	            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
		struct acm_stream ingress, event;
		memset(&ingress, 0, sizeof(ingress));
		memset(&event, 0, sizeof(event));
		struct operation insert_op1, insert_op2, read_op, pad_op;
		memset(&insert_op1, 0, sizeof(insert_op1));
		memset(&insert_op2, 0, sizeof(insert_op2));
		memset(&read_op, 0, sizeof(read_op));
		memset(&pad_op, 0, sizeof(pad_op));
		char buffernameread[] = "special name";
		char buffername_i1[] = "name_insert";
		char buffername_i2[] = "name_insert";
		struct sysfs_buffer new_msg_buf, new_msg2_buf, *reusable;
		memset(&new_msg_buf, 0, sizeof(new_msg_buf));
		bool reuse_flag = false;

		/* prepare testcase */
		config.bypass[1] = &module;
		/*initialize msg buffer list of config */
		ACMLIST_INIT(&config.msg_buffs);
		/*initialize stream list of module */
        ACMLIST_INIT(&module.streams);
		/*initialize operation lists of streams */
		ACMLIST_INIT(&ingress.operations);
		ACMLIST_INIT(&event.operations);
		/* set stream types and references of streams */
		event.type = EVENT_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &event, entry);
		ingress.type = INGRESS_TRIGGERED_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &ingress, entry);
		event.reference = &ingress;
		ingress.reference_parent = &event;
		/* initialize operation and add it to operation list of event stream */
		read_op.opcode = READ;
		read_op.length = 400;
		read_op.offset = 10;
		read_op.buffer_name = buffernameread;
		ACMLIST_INSERT_TAIL(&event.operations, &read_op, entry);
		/* initialize operations and add them to operation list of ingress
		 * triggered stream stream */
		insert_op1.opcode = INSERT;
		insert_op1.length = 777;
		insert_op1.offset = 10;
		insert_op1.buffer_name = buffername_i1;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op1, entry);
		pad_op.opcode = PAD;
		pad_op.length = 777;
		pad_op.offset = 10;
		ACMLIST_INSERT_TAIL(&ingress.operations, &pad_op, entry);
		insert_op2.opcode = INSERT;
		insert_op2.length = 987;
		insert_op2.offset = 50;
		insert_op2.buffer_name = buffername_i2;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op2, entry);
		/* initialize item of type sysfs_buffer as it would be done by buffer_create*/
		new_msg_buf.msg_buff_name = strdup(read_op.buffer_name);
		new_msg_buf.msg_buff_index = 0;
		new_msg_buf.msg_buff_offset = 0;
		new_msg_buf.reset = false;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_RX;
		new_msg_buf.buff_size = read_op.length/4;
		new_msg_buf.timestamp = true;
		new_msg_buf.valid = true;

		/* execute testcase */
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
		buffer_create_ExpectAndReturn(&read_op, 0, 0, (read_op.length/4 + 1),
				&new_msg_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffername_check_IgnoreArg_reuse();
		buffer_add_list_Expect(&config.msg_buffs, &new_msg_buf);

		free(new_msg_buf.msg_buff_name);
		new_msg_buf.msg_buff_name = strdup(insert_op1.buffer_name);
		new_msg_buf.msg_buff_index = 1;
		new_msg_buf.msg_buff_offset = read_op.length/4 + 1;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg_buf.buff_size = insert_op1.length/4 + 1;
		buffer_create_ExpectAndReturn(&insert_op1, 1, read_op.length/4 + 1,
				(insert_op1.length/4 + 1), &new_msg_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffername_check_IgnoreArg_reuse();
		buffer_add_list_Expect(&config.msg_buffs, &new_msg_buf);

		new_msg2_buf.msg_buff_name = strdup(insert_op2.buffer_name);
		new_msg2_buf.msg_buff_index = 2;
		new_msg2_buf.msg_buff_offset = read_op.length/4 + 1 + insert_op1.length/4 + 1;
		new_msg2_buf.reset = false;
		new_msg2_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg2_buf.buff_size = insert_op2.length/4 + 1;
		new_msg2_buf.timestamp = true;
		new_msg2_buf.valid = true;
		reusable = &new_msg_buf;
		reuse_flag = true;
		buffer_create_ExpectAndReturn(&insert_op2, 2, new_msg2_buf.msg_buff_offset,
				(insert_op2.length/4 + 1), &new_msg2_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg2_buf,
				&reuse_flag, &reusable, 0);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffername_check_ReturnMemThruPtr_reuse_msg_buff(&reusable, sizeof(reusable));
		buffername_check_IgnoreArg_reuse();
		buffername_check_ReturnMemThruPtr_reuse(&reuse_flag, sizeof(reuse_flag));
		update_offset_after_buffer_Expect(&config.msg_buffs, reusable);
		buffer_destroy_Expect(&new_msg2_buf);

		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE),
				16384);
		result = create_hw_msg_buf_list(&config);
		TEST_ASSERT_EQUAL(0, result);

 }

void test_create_hw_msg_buf_list_neg_buffer_create(void){
		int result;
		struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
		struct acm_module module =
	            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
		struct acm_stream ingress;
		memset(&ingress, 0, sizeof(ingress));
		struct operation insert_op;
		memset(&insert_op, 0, sizeof(insert_op));
		char buffername[] = "special name";
		struct sysfs_buffer new_msg_buf;
		memset(&new_msg_buf, 0, sizeof(new_msg_buf));

		/* prepare testcase */
		config.bypass[1] = &module;
		/*initialize msg buffer list of config */
		ACMLIST_INIT(&config.msg_buffs);
		/*initialize stream list of module */
        ACMLIST_INIT(&module.streams);
		/*initialize operation lists of streams */
		ACMLIST_INIT(&ingress.operations);
		/* set stream types and references of streams */
		ingress.type = INGRESS_TRIGGERED_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &ingress, entry);
		/* initialize operation and add it to operation list of event stream */
		insert_op.opcode = INSERT;
		insert_op.length = 45;
		insert_op.offset = 10;
		insert_op.buffer_name = buffername;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op, entry);
		/* initialize item of type sysfs_buffer as it would be done by buffer_create*/
		new_msg_buf.msg_buff_name = strdup(insert_op.buffer_name);
		new_msg_buf.msg_buff_index = 0;
		new_msg_buf.msg_buff_offset = 0;
		new_msg_buf.reset = false;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg_buf.buff_size = insert_op.length/4 + 1;
		new_msg_buf.timestamp = true;
		new_msg_buf.valid = true;

		/* execute testcase */
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
		buffer_create_ExpectAndReturn(&insert_op, 0, 0, (insert_op.length/4 + 1),
				NULL);
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE),
				16384);
		result = create_hw_msg_buf_list(&config);
		TEST_ASSERT_EQUAL(-ENOMEM, result);
}

void test_create_hw_msg_buf_list_neg_check_buffername(void){
		int result;
		struct acm_config config = CONFIGURATION_INITIALIZER(config, false);
		struct acm_module module =
	            MODULE_INITIALIZER(module, CONN_MODE_SERIAL, SPEED_100MBps, MODULE_0, &config);
		struct acm_stream ingress;
		memset(&ingress, 0, sizeof(ingress));
		struct operation insert_op;
		memset(&insert_op, 0, sizeof(insert_op));
		char buffername[] = "special name";
		struct sysfs_buffer new_msg_buf, *reusable;
		memset(&new_msg_buf, 0, sizeof(new_msg_buf));
		bool reuse_flag = false;

		/* prepare testcase */
		config.bypass[1] = &module;
		/*initialize msg buffer list of config */
		ACMLIST_INIT(&config.msg_buffs);
		/*initialize stream list of module */
        ACMLIST_INIT(&module.streams);
		/*initialize operation lists of streams */
		ACMLIST_INIT(&ingress.operations);
		/* set stream types and references of streams */
		ingress.type = INGRESS_TRIGGERED_STREAM;
		ACMLIST_INSERT_TAIL(&module.streams, &ingress, entry);
		/* initialize operation and add it to operation list of event stream */
		insert_op.opcode = INSERT;
		insert_op.length = 45;
		insert_op.offset = 10;
		insert_op.buffer_name = buffername;
		ACMLIST_INSERT_TAIL(&ingress.operations, &insert_op, entry);
		/* initialize item of type sysfs_buffer as it would be done by buffer_create*/
		new_msg_buf.msg_buff_name = strdup(insert_op.buffer_name);
		new_msg_buf.msg_buff_index = 0;
		new_msg_buf.msg_buff_offset = 0;
		new_msg_buf.reset = false;
		new_msg_buf.stream_direction = ACMDRV_BUFF_DESC_BUFF_TYPE_TX;
		new_msg_buf.buff_size = insert_op.length/4 + 1;
		new_msg_buf.timestamp = true;
		new_msg_buf.valid = true;

		/* execute testcase */
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 4);
		buffer_create_ExpectAndReturn(&insert_op, 0, 0, (insert_op.length/4 + 1),
				&new_msg_buf);
		buffername_check_ExpectAndReturn(&config.msg_buffs, &new_msg_buf,
				&reuse_flag, &reusable, -EPERM);
		buffername_check_IgnoreArg_reuse_msg_buff();
		buffer_destroy_Expect(&new_msg_buf);
		get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_SIZE),
				16384);
		result = create_hw_msg_buf_list(&config);
		TEST_ASSERT_EQUAL(-EPERM, result);
}

void test_create_hw_msg_buf_list_wrong_granularity(void) {
    int result;

    get_int32_status_value_ExpectAndReturn(__stringify(ACM_SYSFS_MSGBUF_DATAWIDTH), 0);
    logging_Expect(0, "Config: read size of message buffer blocks is invalid: %d");
    result = create_hw_msg_buf_list(NULL);
    TEST_ASSERT_EQUAL(-ENODEV, result);
}
