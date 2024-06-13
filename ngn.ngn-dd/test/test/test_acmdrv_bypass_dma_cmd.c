// SPDX-License-Identifier: GPL-2.0

#include "unity.h"
#include <stdlib.h>

#include "acmdrv.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_bypass_dma_cmd_p_mov_msg_buff_create(void)
{
	uint32_t value;
	bool last = false;
	bool irq = false;
	uint16_t length = 5;
	uint8_t msg_buf_id = 3;

	value = acmdrv_bypass_dma_cmd_p_mov_msg_buff_create(last, irq, length,
		msg_buf_id);

	/*
	 * b'0000000000 000011 00000000101 0 0 10 0  ->
	 * b'0000 0000 0000 0011 0000 0000 1010 0100 ->
	 * 0x000300A4
	 */
	TEST_ASSERT_EQUAL(0x000300A4, value);

	last = true;
	value = acmdrv_bypass_dma_cmd_p_mov_msg_buff_create(last, irq, length,
		msg_buf_id);

	TEST_ASSERT_EQUAL(0x000300A5, value);
}
