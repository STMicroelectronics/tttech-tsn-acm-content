#include "unity.h"
#include <stdlib.h>

#include "acmdrv.h"

static struct acmdrv_msgbuf_lock_ctrl lock, lock2, lock3;
static void *null_lock_mem;

void setUp(void)
{
	memset(&lock, 0xab, sizeof(lock));
	memset(&lock2, 0x0, sizeof(lock2));
	memset(&lock3, 0x0, sizeof(lock3));
	null_lock_mem = calloc(1, sizeof(lock));
}

void tearDown(void)
{
	free(null_lock_mem);
}

void test_acmdrv_msgbuf_lock_control_zero(void)
{

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&lock);

	TEST_ASSERT_EQUAL_MEMORY(null_lock_mem, &lock, sizeof(lock));
}

void test_acmdrv_msgbuf_lock_control_set(void)
{
	int i;

	lock.bits[0] = lock.bits[1] = 0x0;

	ACMDRV_MSGBUF_LOCK_CTRL_SET(0, &lock);

	/* test all bits */
	TEST_ASSERT_EQUAL_UINT32(1 << 0, lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(0, lock.bits[1]);

	ACMDRV_MSGBUF_LOCK_CTRL_SET(ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE - 1, &lock);

	TEST_ASSERT_EQUAL_UINT32(1 << 0, lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(1 << 31, lock.bits[1]);

	ACMDRV_MSGBUF_LOCK_CTRL_SET(23, &lock);

	TEST_ASSERT_EQUAL_UINT32((1 << 0) | (1 << 23), lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32((1 << 31), lock.bits[1]);

	ACMDRV_MSGBUF_LOCK_CTRL_SET(38, &lock);

	TEST_ASSERT_EQUAL_UINT32((1 << 0) | (1 << 23), lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32((1 << 31) | (1 << 6), lock.bits[1]);
}

void test_acmdrv_msgbuf_lock_control_set_out_of_range(void)
{
	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&lock);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE, &lock);

	TEST_ASSERT_EQUAL_MEMORY(null_lock_mem, &lock, sizeof(lock));
}

void test_acmdrv_msgbuf_lock_control_clr(void)
{
	lock.bits[0] = lock.bits[1] = 0xFFFFFFFF;

	ACMDRV_MSGBUF_LOCK_CTRL_CLR(0, &lock);

	TEST_ASSERT_EQUAL_UINT32(~1, lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, lock.bits[1]);

	ACMDRV_MSGBUF_LOCK_CTRL_CLR(ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE - 1, &lock);

	TEST_ASSERT_EQUAL_UINT32(~1, lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(~(1 << 31), lock.bits[1]);

	ACMDRV_MSGBUF_LOCK_CTRL_CLR(23, &lock);

	TEST_ASSERT_EQUAL_UINT32(~(1 | (1 << 23)), lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(~(1 << 31), lock.bits[1]);

	ACMDRV_MSGBUF_LOCK_CTRL_CLR(38, &lock);

	TEST_ASSERT_EQUAL_UINT32(~(1 | (1 << 23)), lock.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(~((1 << 31) | (1 << 6)), lock.bits[1]);
}

void test_acmdrv_msgbuf_lock_control_clr_out_of_range(void)
{
	lock.bits[0] = lock.bits[1] = 0xFFFFFFFF;
	lock2.bits[0] = lock2.bits[1] = 0xFFFFFFFF;
	ACMDRV_MSGBUF_LOCK_CTRL_CLR(ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE, &lock);

	TEST_ASSERT_EQUAL_MEMORY(&lock2, &lock, sizeof(lock));
}


void test_acmdrv_msgbuf_lock_control_isset(void)
{
	lock.bits[0] = (1 << 17);
	lock.bits[1] = (1 << 21);

	TEST_ASSERT_FALSE(ACMDRV_MSGBUF_LOCK_CTRL_ISSET(
		ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE, &lock));
}

void test_acmdrv_msgbuf_lock_control_isset_out_of_range(void)
{
	lock.bits[0] = (1 << 17);
	lock.bits[1] = (1 << 21);

	TEST_ASSERT_FALSE(ACMDRV_MSGBUF_LOCK_CTRL_ISSET(14, &lock));
	TEST_ASSERT_TRUE(ACMDRV_MSGBUF_LOCK_CTRL_ISSET(17, &lock));
	TEST_ASSERT_FALSE(ACMDRV_MSGBUF_LOCK_CTRL_ISSET(32, &lock));
	TEST_ASSERT_TRUE(ACMDRV_MSGBUF_LOCK_CTRL_ISSET(53, &lock));
}

void test_acmdrv_msgbuf_lock_control_equal(void)
{
	lock.bits[0] = (1 << 17);
	lock.bits[1] = (1 << 21);

	lock2.bits[0] = (1 << 17);
	lock2.bits[1] = (1 << 21);

	lock3.bits[0] = (1 << 17);
	lock3.bits[1] = (1 << 22);

	TEST_ASSERT_EQUAL(1, ACMDRV_MSGBUF_LOCK_CTRL_EQUAL(&lock, &lock2));
	TEST_ASSERT_EQUAL(0, ACMDRV_MSGBUF_LOCK_CTRL_EQUAL(&lock, &lock3));
}

void test_acmdrv_msgbuf_lock_control_or(void)
{
	struct acmdrv_msgbuf_lock_ctrl res;

	lock.bits[0] = (1 << 17);
	lock.bits[1] = (1 << 21);

	lock2.bits[0] = (1 << 13);
	lock2.bits[1] = (1 << 16);

	lock3.bits[0] = lock.bits[0] | lock2.bits[0];
	lock3.bits[1] = lock.bits[1] | lock2.bits[1];

	ACMDRV_MSGBUF_LOCK_CTRL_OR(&res, &lock, &lock2);
	TEST_ASSERT_EQUAL_MEMORY(&lock3, &res, sizeof(lock3));
}

void test_acmdrv_msgbuf_lock_control_and(void)
{
	struct acmdrv_msgbuf_lock_ctrl res;

	lock.bits[0] = (1 << 17) | (1 << 13);
	lock.bits[1] = (1 << 21);

	lock2.bits[0] = (1 << 17)  | (1 << 2);
	lock2.bits[1] = (1 << 16);

	lock3.bits[0] = lock.bits[0] & lock2.bits[0];
	lock3.bits[1] = lock.bits[1] & lock2.bits[1];

	ACMDRV_MSGBUF_LOCK_CTRL_AND(&res, &lock, &lock2);
	TEST_ASSERT_EQUAL_MEMORY(&lock3, &res, sizeof(lock3));
}

void test_acmdrv_msgbuf_lock_control_xor(void)
{
	struct acmdrv_msgbuf_lock_ctrl res;

	lock.bits[0] = (1 << 17) | (1 << 13);
	lock.bits[1] = (1 << 21);

	lock2.bits[0] = (1 << 17)  | (1 << 2);
	lock2.bits[1] = (1 << 16);

	lock3.bits[0] = lock.bits[0] ^ lock2.bits[0];
	lock3.bits[1] = lock.bits[1] ^ lock2.bits[1];

	ACMDRV_MSGBUF_LOCK_CTRL_XOR(&res, &lock, &lock2);
	TEST_ASSERT_EQUAL_MEMORY(&lock3, &res, sizeof(lock3));
}

void test_acmdrv_msgbuf_lock_control_not(void)
{
	struct acmdrv_msgbuf_lock_ctrl res;

	lock.bits[0] = (1 << 17) | (1 << 13);
	lock.bits[1] = (1 << 21);

	lock2.bits[0] = ~lock.bits[0];
	lock2.bits[1] = ~lock.bits[1];

	ACMDRV_MSGBUF_LOCK_CTRL_NOT(&res, &lock);
	TEST_ASSERT_EQUAL_MEMORY(&lock2, &res, sizeof(lock2));
}


void test_acmdrv_msgbuf_lock_control_count(void)
{
	size_t count;

	lock.bits[0] = (1 << 17) | (1 << 13);
	lock.bits[1] = (1 << 21);

	count = ACMDRV_MSGBUF_LOCK_CTRL_COUNT(&lock);
	TEST_ASSERT_EQUAL(3, count);
}

void test_acmdrv_msgbuf_lock_control_field_get(void)
{
	unsigned int result;
	struct acmdrv_msgbuf_lock_ctrl mask;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&mask);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(51, &mask);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(52, &mask);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(53, &mask);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(54, &mask);

	lock.bits[0] = (1 << 17) | (1 << 13);
	lock.bits[1] = (1 << 21);

	result = ACMDRV_MSGBUF_LOCK_CTRL_FIELD_GET(&mask, &lock);
	TEST_ASSERT_EQUAL(4, result);
}

void test_acmdrv_msgbuf_lock_control_field_set(void)
{
	struct acmdrv_msgbuf_lock_ctrl result;
	struct acmdrv_msgbuf_lock_ctrl mask;
	int i;

	/* prepare mask from bit 32-47 */
	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&mask);
	for (i = 0; i < 16; ++i)
		ACMDRV_MSGBUF_LOCK_CTRL_SET(32 + i, &mask);

	lock.bits[0] = 0x0badcab1;
	lock.bits[1] = 0xaffe0bad;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&result);
	result.bits[0] = 0x0badcab1;
	result.bits[1] = 0xaffedead;

	ACMDRV_MSGBUF_LOCK_CTRL_FIELD_SET(&mask, &lock, 0xdead);
	TEST_ASSERT_EQUAL_MEMORY(&result, &lock, sizeof(result));
}

void test_acmdrv_msgbuf_lock_control_assign(void)
{
	struct acmdrv_msgbuf_lock_ctrl lh, rh, *res;

	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&rh);
	ACMDRV_MSGBUF_LOCK_CTRL_ZERO(&lh);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(55, &lh);
	ACMDRV_MSGBUF_LOCK_CTRL_SET(43, &rh);

	res = ACMDRV_MSGBUF_LOCK_CTRL_ASSIGN(&lh, &rh);

	TEST_ASSERT_EQUAL_PTR(&lh, res);
	TEST_ASSERT(ACMDRV_MSGBUF_LOCK_CTRL_EQUAL(&lh, &rh))
}

void test_acmdrv_msgbuf_lock_control_genmask(void)
{
	struct acmdrv_msgbuf_lock_ctrl mask, *res;
	int i;
	int lo = 28;
	int hi = 33;

	res = ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(&mask, hi, lo);

	TEST_ASSERT_EQUAL_PTR(&mask, res);
	for (i = 0; i < ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE; ++i) {
		bool bit = ((lo <= i) && (i <= hi));

		TEST_ASSERT(bit == ACMDRV_MSGBUF_LOCK_CTRL_ISSET(i, &mask));
	}
}

void test_acmdrv_msgbuf_lock_control_genmask_0_15(void)
{
	struct acmdrv_msgbuf_lock_ctrl mask, *res;
	int i;
	int lo = 0;
	int hi = 15;

	res = ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(&mask, hi, lo);

	TEST_ASSERT_EQUAL_PTR(&mask, res);
	for (i = 0; i < ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE; ++i) {
		bool bit = ((lo <= i) && (i <= hi));

		TEST_ASSERT(bit == ACMDRV_MSGBUF_LOCK_CTRL_ISSET(i, &mask));
	}
}

void test_acmdrv_msgbuf_lock_control_genmask_16_31(void)
{
	struct acmdrv_msgbuf_lock_ctrl mask, *res;
	int i;
	int lo = 16;
	int hi = 31;

	res = ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(&mask, hi, lo);

	TEST_ASSERT_EQUAL_PTR(&mask, res);
	for (i = 0; i < ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE; ++i) {
		bool bit = ((lo <= i) && (i <= hi));

		TEST_ASSERT(bit == ACMDRV_MSGBUF_LOCK_CTRL_ISSET(i, &mask));
	}
}

void test_acmdrv_msgbuf_lock_control_genmask_32_47(void)
{
	struct acmdrv_msgbuf_lock_ctrl mask, *res;
	int i;
	int lo = 32;
	int hi = 47;

	res = ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(&mask, hi, lo);

	TEST_ASSERT_EQUAL_PTR(&mask, res);
	for (i = 0; i < ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE; ++i) {
		bool bit = ((lo <= i) && (i <= hi));

		TEST_ASSERT(bit == ACMDRV_MSGBUF_LOCK_CTRL_ISSET(i, &mask));
	}
}

void test_acmdrv_msgbuf_lock_control_genmask_48_63(void)
{
	struct acmdrv_msgbuf_lock_ctrl mask, *res;
	int i;
	int lo = 48;
	int hi = 63;

	res = ACMDRV_MSGBUF_LOCK_CTRL_GENMASK(&mask, hi, lo);

	TEST_ASSERT_EQUAL_PTR(&mask, res);
	for (i = 0; i < ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE; ++i) {
		bool bit = ((lo <= i) && (i <= hi));

		TEST_ASSERT(bit == ACMDRV_MSGBUF_LOCK_CTRL_ISSET(i, &mask));
	}
}
