#include "unity.h"

#include "buffer.h"
#include "tracing.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "mock_memory.h"
#include "mock_validate.h"
#include "mock_logging.h"

//TODO: update: static struct schedule_entry window_mem;
//TODO: update: static struct schedule_list winlist_mem;

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
	//TODO: update: 	memset(&window_mem, 0, sizeof(window_mem));
	//TODO: update: 	memset(&winlist_mem, 0, sizeof(winlist_mem));
}

void tearDown(void)
{
}

void test_buffer_create_rx(void){
	struct sysfs_buffer buffer_item;
	struct sysfs_buffer *result;
	memset(&buffer_item, 0, sizeof(buffer_item));
	struct operation operation;
	memset(&operation, 0, sizeof(operation));
	char buff_name[80];
	char string[] = "message_buffer_nr_1";
	uint8_t index = 7;
	uint16_t offset = 21;
	uint16_t size = 10;
	operation.opcode = READ;
	operation.buffer_name = buff_name;

	acm_zalloc_ExpectAndReturn(sizeof(buffer_item), &buffer_item);
	acm_strdup_ExpectAndReturn(buff_name, string);
	result = buffer_create(&operation, index, offset, size);
	TEST_ASSERT_EQUAL_PTR(&buffer_item, result);
	TEST_ASSERT_EQUAL(index, result->msg_buff_index);
	TEST_ASSERT_EQUAL(size, result->buff_size);
	TEST_ASSERT_EQUAL(offset, result->msg_buff_offset);
	TEST_ASSERT_EQUAL(ACMDRV_BUFF_DESC_BUFF_TYPE_RX, result->stream_direction);
	TEST_ASSERT_TRUE(result->valid);
	TEST_ASSERT_TRUE(result->timestamp);
	TEST_ASSERT_FALSE(result->reset);
	TEST_ASSERT_EQUAL_STRING(string, result->msg_buff_name);
}

void test_buffer_create_tx(void){
	struct sysfs_buffer buffer_item;
	struct sysfs_buffer *result;
	memset(&buffer_item, 0, sizeof(buffer_item));
	struct operation operation;
	memset(&operation, 0, sizeof(operation));
	char buff_name[] = "message_buffer_nr_1";
	char string[] = "message_buffer_nr_1";
	uint8_t index = 7;
	uint16_t offset = 21;
	uint16_t size = 10;
	operation.opcode = INSERT;
	operation.buffer_name = buff_name;

	acm_zalloc_ExpectAndReturn(sizeof(buffer_item), &buffer_item);
	acm_strdup_ExpectAndReturn(buff_name, string);
	result = buffer_create(&operation, index, offset, size);
	TEST_ASSERT_EQUAL_PTR(&buffer_item, result);
	TEST_ASSERT_EQUAL(index, result->msg_buff_index);
	TEST_ASSERT_EQUAL(size, result->buff_size);
	TEST_ASSERT_EQUAL(offset, result->msg_buff_offset);
	TEST_ASSERT_EQUAL(ACMDRV_BUFF_DESC_BUFF_TYPE_TX, result->stream_direction);
	TEST_ASSERT_TRUE(result->valid);
	TEST_ASSERT_TRUE(result->timestamp);
	TEST_ASSERT_FALSE(result->reset);
	TEST_ASSERT_EQUAL_STRING(buff_name, result->msg_buff_name);
}

void test_buffer_create_null_operation(void){
	struct sysfs_buffer *result;
	uint8_t index = 7;
	uint16_t offset = 21;
	uint16_t size = 10;

	logging_Expect(0, "Buffer: pointer to operation is null");
	result = buffer_create(NULL, index, offset, size);
	TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_buffer_create_wrong_opcode(void){
	struct sysfs_buffer buffer_item;
	struct sysfs_buffer *result;
	memset(&buffer_item, 0, sizeof(buffer_item));
	struct operation operation;
	memset(&operation, 0, sizeof(operation));
	uint8_t index = 7;
	uint16_t offset = 21;
	uint16_t size = 10;
	operation.opcode = FORWARD_ALL;

	logging_Expect(0, "Buffer: Wrong operation code");
	result = buffer_create(&operation, index, offset, size);
	TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_buffer_create_no_memory(void){
	struct sysfs_buffer buffer_item;
	struct sysfs_buffer *result;
	memset(&buffer_item, 0, sizeof(buffer_item));
	struct operation operation;
	memset(&operation, 0, sizeof(operation));
	uint8_t index = 7;
	uint16_t offset = 21;
	uint16_t size = 10;
	operation.opcode = READ;

	acm_zalloc_ExpectAndReturn(sizeof(buffer_item), NULL);
	logging_Expect(0, "Buffer: Out of memory");
	result = buffer_create(&operation, index, offset, size);
	TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_buffer_create_neg_copy_buffername(void){
	struct sysfs_buffer buffer_item;
	struct sysfs_buffer *result;
	memset(&buffer_item, 0, sizeof(buffer_item));
	struct operation operation;
	memset(&operation, 0, sizeof(operation));
	char buff_name[] = "message_buffer_nr_1";
	uint8_t index = 7;
	uint16_t offset = 21;
	uint16_t size = 10;
	operation.opcode = INSERT;
	operation.buffer_name = buff_name;

	acm_zalloc_ExpectAndReturn(sizeof(buffer_item), &buffer_item);
	acm_strdup_ExpectAndReturn(buff_name, NULL);
	logging_Expect(0, "Buffer: Problem when copying buffer name. errno: %d");
	acm_free_Expect(&buffer_item);
	result = buffer_create(&operation, index, offset, size);
	TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_buffer_destroy(void) {
	struct sysfs_buffer sysfs_msg_buffer;
	memset(&sysfs_msg_buffer, 0, sizeof(sysfs_msg_buffer));
	char buffername[] = "buffername";

	sysfs_msg_buffer.msg_buff_name = buffername;

	acm_free_Expect(sysfs_msg_buffer.msg_buff_name);
	acm_free_Expect(&sysfs_msg_buffer);
	buffer_destroy(&sysfs_msg_buffer);
}

void test_buffer_destroy_null(void) {

	buffer_destroy(NULL);
}

void test_buffer_init_list(void) {
	int result;
	struct acm_config config;
	memset(&config, 0, sizeof(config));

	result = buffer_init_list(&config.msg_buffs);
	TEST_ASSERT_EQUAL(0, result);
	TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&config.msg_buffs));
	TEST_ASSERT_EQUAL_PTR(NULL, ACMLIST_FIRST(&config.msg_buffs));
	TEST_ASSERT_EQUAL_PTR(ACMLIST_FIRST(&config.msg_buffs),
	        ACMLIST_LAST(&config.msg_buffs, buffer_list));
}

void test_buffer_init_list_null(void) {
	int result;

	result = buffer_init_list(NULL);
	TEST_ASSERT_EQUAL(-EINVAL, result);
}

void test_buffer_add_list_null_list(void) {
	struct sysfs_buffer buffer;
	memset(&buffer, 0, sizeof(buffer));

	logging_Expect(0, "Buffer: Pointer to buffer_list or buffer is null");
	buffer_add_list(NULL, &buffer);
}
void test_buffer_add_list_null_buffer(void) {
	struct buffer_list msg_bufferlist;
	memset(&msg_bufferlist, 0, sizeof(msg_bufferlist));

	ACMLIST_INIT(&msg_bufferlist);
	logging_Expect(0, "Buffer: Pointer to buffer_list or buffer is null");
	buffer_add_list(&msg_bufferlist, NULL);
	TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&msg_bufferlist));
	TEST_ASSERT_NULL(ACMLIST_FIRST(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(ACMLIST_FIRST(&msg_bufferlist),
	        ACMLIST_LAST(&msg_bufferlist, buffer_list));
}
void test_buffer_add_list(void) {
	struct buffer_list msg_bufferlist;
	memset(&msg_bufferlist, 0, sizeof(msg_bufferlist));
	struct sysfs_buffer buffer;
	memset(&buffer, 0, sizeof(buffer));

	ACMLIST_INIT(&msg_bufferlist);

	buffer_add_list(&msg_bufferlist, &buffer);
	TEST_ASSERT_EQUAL(1, ACMLIST_COUNT(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer, ACMLIST_FIRST(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(ACMLIST_FIRST(&msg_bufferlist),
	        ACMLIST_LAST(&msg_bufferlist, buffer_list));
}

void test_buffer_empty_list(void) {
	struct buffer_list msg_bufferlist;
	memset(&msg_bufferlist, 0, sizeof(msg_bufferlist));
	struct sysfs_buffer buffer[3];
	memset(&buffer, 0, sizeof(buffer));

	/* prepare testcase */
	ACMLIST_INIT(&msg_bufferlist);
	buffer_add_list(&msg_bufferlist, &buffer[0]);
	buffer_add_list(&msg_bufferlist, &buffer[1]);
	buffer_add_list(&msg_bufferlist, &buffer[2]);
	TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer[0], ACMLIST_FIRST(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer[1], ACMLIST_NEXT(&buffer[0], entry));
	TEST_ASSERT_EQUAL_PTR(&buffer[2], ACMLIST_NEXT(&buffer[1], entry));
	TEST_ASSERT_EQUAL_PTR(&buffer[2], ACMLIST_LAST(&msg_bufferlist, buffer_list));

	/* execute testcase */
	acm_free_Expect(buffer[0].msg_buff_name);
	acm_free_Expect(&buffer[0]);
	acm_free_Expect(buffer[1].msg_buff_name);
	acm_free_Expect(&buffer[1]);
	acm_free_Expect(buffer[2].msg_buff_name);
	acm_free_Expect(&buffer[2]);
	buffer_empty_list(&msg_bufferlist);

	TEST_ASSERT_EQUAL(0, ACMLIST_COUNT(&msg_bufferlist));
	TEST_ASSERT_NULL(ACMLIST_FIRST(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(ACMLIST_FIRST(&msg_bufferlist), ACMLIST_LAST(&msg_bufferlist, buffer_list));
}

void test_buffer_empty_list_null(void) {

	buffer_empty_list(NULL);
}

void test_update_offset_after_buffer_1(void){
	struct buffer_list msg_bufferlist;
	memset(&msg_bufferlist, 0, sizeof(msg_bufferlist));
	struct sysfs_buffer buffer[3];
	memset(&buffer, 0, sizeof(buffer));

	/* prepare testcase */
	ACMLIST_INIT(&msg_bufferlist);
	buffer_add_list(&msg_bufferlist, &buffer[0]);
	buffer_add_list(&msg_bufferlist, &buffer[1]);
	buffer_add_list(&msg_bufferlist, &buffer[2]);
	TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer[0], ACMLIST_FIRST(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer[1], ACMLIST_NEXT(&buffer[0], entry));
	TEST_ASSERT_EQUAL_PTR(&buffer[2], ACMLIST_NEXT(&buffer[1], entry));
	TEST_ASSERT_EQUAL_PTR(&buffer[2], ACMLIST_LAST(&msg_bufferlist, buffer_list));
	/* set sizes of msgbuffers */
	buffer[0].msg_buff_index = 0;
	buffer[0].buff_size = 10;
	buffer[0].msg_buff_offset = 0;
	buffer[1].msg_buff_index = 1;
	buffer[1].buff_size = 20;
	buffer[1].msg_buff_offset = 10;
	buffer[2].msg_buff_index = 2;
	buffer[2].buff_size = 30;
	buffer[2].msg_buff_offset = 30;

	buffer[0].buff_size = 20;

	/* execute testcase */
	update_offset_after_buffer(&msg_bufferlist, &buffer[0]);
	/* check new offsets */
	TEST_ASSERT_EQUAL(20, buffer[1].msg_buff_offset);
	TEST_ASSERT_EQUAL(40, buffer[2].msg_buff_offset);
}

void test_update_offset_after_buffer_2(void){
	struct buffer_list msg_bufferlist;
	memset(&msg_bufferlist, 0, sizeof(msg_bufferlist));
	struct sysfs_buffer buffer[3];
	memset(&buffer, 0, sizeof(buffer));

	/* prepare testcase */
	ACMLIST_INIT(&msg_bufferlist);
	buffer_add_list(&msg_bufferlist, &buffer[0]);
	buffer_add_list(&msg_bufferlist, &buffer[1]);
	buffer_add_list(&msg_bufferlist, &buffer[2]);
	TEST_ASSERT_EQUAL(3, ACMLIST_COUNT(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer[0], ACMLIST_FIRST(&msg_bufferlist));
	TEST_ASSERT_EQUAL_PTR(&buffer[1], ACMLIST_NEXT(&buffer[0], entry));
	TEST_ASSERT_EQUAL_PTR(&buffer[2], ACMLIST_NEXT(&buffer[1], entry));
	TEST_ASSERT_EQUAL_PTR(&buffer[2], ACMLIST_LAST(&msg_bufferlist, buffer_list));
	/* set sizes of msgbuffers */
	buffer[0].msg_buff_index = 0;
	buffer[0].buff_size = 10;
	buffer[0].msg_buff_offset = 0;
	buffer[1].msg_buff_index = 1;
	buffer[1].buff_size = 20;
	buffer[1].msg_buff_offset = 10;
	buffer[2].msg_buff_index = 2;
	buffer[2].buff_size = 30;
	buffer[2].msg_buff_offset = 30;

	buffer[1].buff_size = 400;

	/* execute testcase */
	update_offset_after_buffer(&msg_bufferlist, &buffer[1]);
	/* check new offsets */
	TEST_ASSERT_EQUAL(10, buffer[1].msg_buff_offset);
	TEST_ASSERT_EQUAL(410, buffer[2].msg_buff_offset);
}

void test_update_offset_after_buffer_null_list(void) {
	struct sysfs_buffer buffer;

	logging_Expect(0, "Buffer: Pointer to buffer_list %d or message_buffer %d is null");
	update_offset_after_buffer(NULL, &buffer);
}

void test_update_offset_after_buffer_null_buffer(void) {
	struct buffer_list bufferlist;

	logging_Expect(0, "Buffer: Pointer to buffer_list %d or message_buffer %d is null");
	update_offset_after_buffer(&bufferlist, NULL);
}
