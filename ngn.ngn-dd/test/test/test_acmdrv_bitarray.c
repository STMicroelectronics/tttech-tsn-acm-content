#include "unity.h"
#include <stdlib.h>

#include "acmdrv.h"

#define BITARRAY_SIZE 7
typedef struct acmdrv_bitarray_test {
	acmdrv_bitarray_elem_t bits[BITARRAY_SIZE];
} acmdrv_bitarray_test_t;

static acmdrv_bitarray_test_t ary, ary2, ary3;
static void *null_ary_mem;

void setUp(void)
{
	memset(&ary, 0xab, sizeof(ary));
	memset(&ary2, 0x0, sizeof(ary2));
	memset(&ary3, 0x0, sizeof(ary3));
	null_ary_mem = calloc(1, sizeof(ary));
}

void tearDown(void)
{
	free(null_ary_mem);
}

void test_bitarray_zero(void)
{

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_MEMORY(null_ary_mem, &ary, sizeof(ary));
}

void test_bitarray_set(void)
{
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	acmdrv_bitarray_set(0, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	/* test all bits */
	TEST_ASSERT_EQUAL_UINT32(1 << 0, ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(0, ary.bits[1]);

	acmdrv_bitarray_set(sizeof(acmdrv_bitarray_test_t) * NBBY - 1,
		sizeof(acmdrv_bitarray_test_t), (struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32(1 << 0, ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(1 << 31, ary.bits[BITARRAY_SIZE - 1]);

	acmdrv_bitarray_set(23, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32((1 << 0) | (1 << 23), ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32((1 << 31), ary.bits[BITARRAY_SIZE - 1]);

	acmdrv_bitarray_set(38, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32((1 << 0) | (1 << 23), ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32((1 << 6), ary.bits[1]);
	TEST_ASSERT_EQUAL_UINT32((1 << 31), ary.bits[BITARRAY_SIZE - 1]);
}

void test_bitarray_clr(void)
{
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0xFFFFFFFF;

	acmdrv_bitarray_clr(0, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32(~1, ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFF, ary.bits[1]);

	acmdrv_bitarray_clr(ACMDRV_MSGBUF_LOCK_CTRL_MAXSIZE - 1,
		sizeof(acmdrv_bitarray_test_t), (struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32(~1, ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(~(1 << 31), ary.bits[1]);

	acmdrv_bitarray_clr(23, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32(~(1 | (1 << 23)), ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(~(1 << 31), ary.bits[1]);

	acmdrv_bitarray_clr(38, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	TEST_ASSERT_EQUAL_UINT32(~(1 | (1 << 23)), ary.bits[0]);
	TEST_ASSERT_EQUAL_UINT32(~((1 << 31) | (1 << 6)), ary.bits[1]);
}

void test_bitarray_isset(void)
{

	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[0] = (1 << 17);
	ary.bits[1] = (1 << 21);

	TEST_ASSERT_FALSE(acmdrv_bitarray_isset(14,
		sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary));
	TEST_ASSERT_TRUE(acmdrv_bitarray_isset(17,
		sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary));
	TEST_ASSERT_FALSE(acmdrv_bitarray_isset(32,
		sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary));
	TEST_ASSERT_TRUE(acmdrv_bitarray_isset(53,
		sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary));
}

void test_bitarray_equal(void)
{
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[0] = (1 << 17);
	ary.bits[1] = (1 << 21);

	ary2.bits[0] = (1 << 17);
	ary2.bits[1] = (1 << 21);

	ary3.bits[0] = (1 << 17);
	ary3.bits[1] = (1 << 22);

	TEST_ASSERT_TRUE(acmdrv_bitarray_equal(sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary,
		(const struct acmdrv_bitarray *)&ary2));
	TEST_ASSERT_FALSE(acmdrv_bitarray_equal(sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary,
		(const struct acmdrv_bitarray *)&ary3));
}

void test_bitarray_or(void)
{
	acmdrv_bitarray_test_t res;
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[0] = (1 << 17);
	ary.bits[1] = (1 << 21);

	ary2.bits[0] = (1 << 13);
	ary2.bits[1] = (1 << 16);

	ary3.bits[0] = ary.bits[0] | ary2.bits[0];
	ary3.bits[1] = ary.bits[1] | ary2.bits[1];

	acmdrv_bitarray_or(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&res,
		(const struct acmdrv_bitarray *)&ary,
		(const struct acmdrv_bitarray *)&ary2);
	TEST_ASSERT_EQUAL_MEMORY(&ary3, &res, sizeof(ary3));
}

void test_bitarray_and(void)
{
	acmdrv_bitarray_test_t res;
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[0] = (1 << 17) | (1 << 13);
	ary.bits[1] = (1 << 21);

	ary2.bits[0] = (1 << 17)  | (1 << 2);
	ary2.bits[1] = (1 << 16);

	ary3.bits[0] = ary.bits[0] & ary2.bits[0];
	ary3.bits[1] = ary.bits[1] & ary2.bits[1];

	acmdrv_bitarray_and(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&res,
		(const struct acmdrv_bitarray *)&ary,
		(const struct acmdrv_bitarray *)&ary2);
	TEST_ASSERT_EQUAL_MEMORY(&ary3, &res, sizeof(ary3));
}

void test_bitarray_xor(void)
{
	acmdrv_bitarray_test_t res;
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[0] = (1 << 17) | (1 << 13);
	ary.bits[1] = (1 << 21);

	ary2.bits[0] = (1 << 17)  | (1 << 2);
	ary2.bits[1] = (1 << 16);

	ary3.bits[0] = ary.bits[0] ^ ary2.bits[0];
	ary3.bits[1] = ary.bits[1] ^ ary2.bits[1];

	acmdrv_bitarray_xor(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&res,
		(const struct acmdrv_bitarray *)&ary,
		(const struct acmdrv_bitarray *)&ary2);
	TEST_ASSERT_EQUAL_MEMORY(&ary3, &res, sizeof(ary3));
}

void test_bitarray_not(void)
{
	acmdrv_bitarray_test_t res;
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i) {
		ary.bits[i] = 0x0;
		res.bits[i] = 0x0;
	}

	ary.bits[0] = (1 << 17) | (1 << 13);
	ary.bits[1] = (1 << 21);

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary2.bits[i] = ~ary.bits[i];

	acmdrv_bitarray_not(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&res,
		(const struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &res, sizeof(ary2));
}


void test_bitarray_count(void)
{
	size_t count;
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[0] = (1 << 17) | (1 << 13);
	ary.bits[1] = (1 << 21);

	count = acmdrv_bitarray_count(sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL(3, count);
}

void test_bitarray_ffs(void)
{
	int i;
	int ffs;

	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ary.bits[BITARRAY_SIZE - 1] = (1 << 13);
	ffs = acmdrv_bitarray_ffs(sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL((BITARRAY_SIZE - 1) * 32 + 13 + 1, ffs);
}

void test_bitarray_ffs_of_0(void)
{
	int i;
	int ffs;

	/* no bit set */
	for (i = 0; i < BITARRAY_SIZE; ++i)
		ary.bits[i] = 0x0;

	ffs = acmdrv_bitarray_ffs(sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL(0, ffs);
}

void test_bitarray_shiftr_64(void)
{
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i) {
		ary.bits[i] = 0x0;
		ary2.bits[i] = 0x0;
	}

	ary.bits[BITARRAY_SIZE - 1] = (1 << 13);
	ary2.bits[BITARRAY_SIZE - 3] = (1 << 13);
	acmdrv_bitarray_shr(sizeof(acmdrv_bitarray_test_t), 64,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}

void test_bitarray_shiftr_65(void)
{
	int i;

	for (i = 0; i < BITARRAY_SIZE; ++i) {
		ary.bits[i] = 0x0;
		ary2.bits[i] = 0x0;
	}

	ary.bits[BITARRAY_SIZE - 1] = (1 << 13);
	ary2.bits[BITARRAY_SIZE - 3] = (1 << 12);
	acmdrv_bitarray_shr(sizeof(acmdrv_bitarray_test_t), 65,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}

void test_bitarray_shr_16(void)
{
	int i;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	ary.bits[1] = (1 << 23);
	ary.bits[2] = 0xaffedead;
	ary2.bits[1] = 0xdead0000 | (1 << 7);
	ary2.bits[2] = 0x0000affe;
	acmdrv_bitarray_shr(sizeof(acmdrv_bitarray_test_t), 16,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}

void test_bitarray_shr_64(void)
{
	int i;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	ary.bits[BITARRAY_SIZE - 1] = 0xaffe0000;
	ary2.bits[BITARRAY_SIZE - 3] = 0xaffe0000;
	acmdrv_bitarray_shr(sizeof(acmdrv_bitarray_test_t), 64,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}


void test_bitarray_shr_2_border(void)
{
	int i;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	ary.bits[0] = (1 << 1) | (1 << 2);
	ary2.bits[0] = (1 << 0);
	acmdrv_bitarray_shr(sizeof(acmdrv_bitarray_test_t), 2,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}


void test_bitarray_shl_16(void)
{
	int i;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	ary.bits[1] = (1 << 23);
	ary.bits[2] = 0xaffedead;
	ary2.bits[2] = 0xdead0000 | (1 << 7);
	ary2.bits[3] = 0x0000affe;
	acmdrv_bitarray_shl(sizeof(acmdrv_bitarray_test_t), 16,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}

void test_bitarray_shl_64(void)
{
	int i;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	ary.bits[0] = 0xdead;
	ary2.bits[2] = 0xdead;
	acmdrv_bitarray_shl(sizeof(acmdrv_bitarray_test_t), 64,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}


void test_bitarray_shl_2_border(void)
{
	int i;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	ary.bits[BITARRAY_SIZE - 1] = (1 << 30) | (1 << 29);
	ary2.bits[BITARRAY_SIZE - 1] = (1 << 31);
	acmdrv_bitarray_shl(sizeof(acmdrv_bitarray_test_t), 2,
		(struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, &ary, sizeof(ary2));
}

void test_bitarray_shl_on_stack(void)
{
	char buffer[3 * sizeof(acmdrv_bitarray_test_t)];
	acmdrv_bitarray_test_t *arytest =
		(acmdrv_bitarray_test_t *)
		&buffer[sizeof(acmdrv_bitarray_test_t)];

	memset(buffer, 0xaa, sizeof(buffer));

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)arytest);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	arytest->bits[BITARRAY_SIZE - 1] = (1 << 30) | (1 << 29);
	ary2.bits[BITARRAY_SIZE - 1] = (1 << 31);
	acmdrv_bitarray_shl(sizeof(acmdrv_bitarray_test_t), 2,
		(struct acmdrv_bitarray *)arytest);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, arytest, sizeof(ary2));
}

void test_bitarray_shr_on_stack(void)
{
	char buffer[3 * sizeof(acmdrv_bitarray_test_t)];
	acmdrv_bitarray_test_t *arytest =
		(acmdrv_bitarray_test_t *)
		&buffer[sizeof(acmdrv_bitarray_test_t)];

	memset(buffer, 0xaa, sizeof(buffer));

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)arytest);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary2);

	arytest->bits[0] = (1 << 2) | (1 << 1);
	ary2.bits[0] = (1 << 0);
	acmdrv_bitarray_shr(sizeof(acmdrv_bitarray_test_t), 2,
		(struct acmdrv_bitarray *)arytest);
	TEST_ASSERT_EQUAL_MEMORY(&ary2, arytest, sizeof(ary2));
}


void test_bitarray_field_get(void)
{
	acmdrv_bitarray_test_t mask;
	acmdrv_bitarray_elem_t value;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);

	ary.bits[2] = 0xaffedead;
	mask.bits[2] = 0xffff0000;

	value = acmdrv_bitmask_field_get(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask,
		(const struct acmdrv_bitarray *)&ary);
	TEST_ASSERT_EQUAL(0xaffe, value);
}

void test_bitarray_field_set(void)
{
	acmdrv_bitarray_test_t mask, result;
	acmdrv_bitarray_elem_t value = 0xdead;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&ary);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&result);

	ary.bits[2] = 0xaffe0bad;
	mask.bits[2] = 0x0000ffff;
	result.bits[2] = 0xaffedead;

	acmdrv_bitmask_field_set(sizeof(acmdrv_bitarray_test_t),
		(const struct acmdrv_bitarray *)&mask,
		(struct acmdrv_bitarray *)&ary, value);
	TEST_ASSERT_EQUAL_MEMORY(&result, &ary, sizeof(result));
}

void test_acmdrv_bitmask_assign(void)
{
	acmdrv_bitarray_test_t t1, t2;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&t1);
	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&t2);

	acmdrv_bitarray_set(3, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&t1);

	acmdrv_bitmask_assign(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&t2,
		(const struct acmdrv_bitarray *)&t1);

	TEST_ASSERT_EQUAL_MEMORY(&t1, &t2, sizeof(t1));

	acmdrv_bitarray_set(67, sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&t1);

	acmdrv_bitmask_assign(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&t2,
		(const struct acmdrv_bitarray *)&t1);

	TEST_ASSERT_EQUAL_MEMORY(&t1, &t2, sizeof(t1));
}

void test_acmdrv_bitmask_genmask(void)
{
	int i;
	int lo = 29;
	int hi = 38;

	acmdrv_bitarray_test_t mask;
	acmdrv_bitarray_test_t *result;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask);

	result = (acmdrv_bitarray_test_t *)acmdrv_bitmask_genmask(
		sizeof(acmdrv_bitarray_test_t), (struct acmdrv_bitarray *)&mask,
		hi, lo);

	TEST_ASSERT_EQUAL_PTR(&mask, result);

	for (i = 0; i < sizeof(acmdrv_bitarray_test_t) * NBBY; ++i) {
		bool bit = ((lo <= i) && (i <= hi));

		TEST_ASSERT(bit == acmdrv_bitarray_isset(i,
			sizeof(acmdrv_bitarray_test_t),
			(const struct acmdrv_bitarray *)&mask));
	}
}

void test_acmdrv_bitmask_genmask_wrong(void)
{
	int i;
	int lo = 48;
	int hi = 43;

	acmdrv_bitarray_test_t mask;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask);

	acmdrv_bitmask_genmask(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask, hi, lo);

	for (i = 0; i < sizeof(acmdrv_bitarray_test_t) * NBBY; ++i) {
		TEST_ASSERT_FALSE(acmdrv_bitarray_isset(i,
			sizeof(acmdrv_bitarray_test_t),
			(const struct acmdrv_bitarray *)&mask));
	}
}

void test_acmdrv_bitmask_genmask_single(void)
{
	int i;
	int lo = 48;
	int hi = lo;

	acmdrv_bitarray_test_t mask;

	acmdrv_bitarray_zero(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask);

	acmdrv_bitmask_genmask(sizeof(acmdrv_bitarray_test_t),
		(struct acmdrv_bitarray *)&mask, hi, lo);

	for (i = 0; i < sizeof(acmdrv_bitarray_test_t) * NBBY; ++i) {
		bool bit = (i == lo);

		TEST_ASSERT(bit == acmdrv_bitarray_isset(i,
			sizeof(acmdrv_bitarray_test_t),
			(const struct acmdrv_bitarray *)&mask));
	}
}

