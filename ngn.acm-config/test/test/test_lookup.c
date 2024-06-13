#include "unity.h"

#include <string.h>
#include <stdint.h>

#include "lookup.h"
#include "tracing.h"

#include "mock_logging.h"
#include "mock_memory.h"

void __attribute__((weak)) suite_setup(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_lookup_create(void) {
    struct lookup lookup_mem;
    memset(&lookup_mem, 0, sizeof (lookup_mem));
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t filter_mask[] = { 0xFF };
    uint8_t filter_pattern[] = { 0xA0 };
    struct lookup *result;

    acm_zalloc_ExpectAndReturn(sizeof(struct lookup), &lookup_mem);

    result = lookup_create(header,
            header_mask,
            filter_pattern,
            filter_mask,
            sizeof (filter_pattern));

    TEST_ASSERT_EQUAL_PTR(&lookup_mem, result);
    TEST_ASSERT_EQUAL_MEMORY(header, result->header, sizeof (header));
    TEST_ASSERT_EQUAL_MEMORY(header_mask, result->header_mask, sizeof (header_mask));
    TEST_ASSERT_EQUAL(sizeof (filter_pattern), result->filter_size);
    TEST_ASSERT_EQUAL_MEMORY(filter_mask, result->filter_mask, sizeof (filter_mask));
    TEST_ASSERT_EQUAL_MEMORY(filter_pattern, result->filter_pattern, sizeof (filter_pattern));
}

void test_lookup_create_invalid_filtersize(void) {
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t filter_mask[] = { 0xFF };
    uint8_t filter_pattern[] = { 0xA0 };
    struct lookup *result;

    logging_Expect(LOGLEVEL_ERR, "Lookup: Filter size too big: %u > %u");
    result = lookup_create(header, header_mask, filter_pattern, filter_mask, 113);

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_lookup_destroy(void) {
    struct lookup lookup_mem;
    memset(&lookup_mem, 0, sizeof (lookup_mem));

    acm_free_Expect(&lookup_mem);

    lookup_destroy(&lookup_mem);
}

void test_lookup_destroy_null(void) {
    lookup_destroy(NULL);
}

void test_lookup_create_filter_too_big(void) {
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t big_filter_mask[ACM_MAX_FILTER_SIZE + 1];
    uint8_t big_filter_pattern[ACM_MAX_FILTER_SIZE + 1];
    struct lookup *result;

    logging_Expect(LOGLEVEL_ERR, "Lookup: Filter size too big: %u > %u");
    result = lookup_create(header,
            header_mask,
            big_filter_pattern,
            big_filter_mask,
            sizeof (big_filter_pattern));

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_lookup_create_no_filter_mask(void) {
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t filter_pattern[] = { 0xA0 };
    struct lookup *result;

    logging_Expect(LOGLEVEL_ERR, "Lookup: Filter input error");
    result = lookup_create(header, header_mask, filter_pattern,
    NULL, sizeof (filter_pattern));

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_lookup_create_no_filter_pattern(void) {
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t filter_mask[] = { 0xFF };
    struct lookup *result;

    logging_Expect(LOGLEVEL_ERR, "Lookup: Filter input error");
    result = lookup_create(header, header_mask, NULL, filter_mask, sizeof (filter_mask));

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}

void test_lookup_create_null_filtersize(void) {
    struct lookup lookup_mem;
    memset(&lookup_mem, 0, sizeof (lookup_mem));
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t big_filter_mask[ACM_MAX_FILTER_SIZE + 1];
    uint8_t big_filter_pattern[ACM_MAX_FILTER_SIZE + 1];
    uint8_t zero_filter_pattern[ACM_MAX_FILTER_SIZE] = { 0x00 };
    uint8_t zero_filter_mask[ACM_MAX_FILTER_SIZE] = { 0x00 };
    struct lookup *result;

    acm_zalloc_ExpectAndReturn(sizeof(struct lookup), &lookup_mem);

    result = lookup_create(header, header_mask, big_filter_pattern, big_filter_mask, 0);

    TEST_ASSERT_EQUAL_PTR(&lookup_mem, result);
    TEST_ASSERT_EQUAL_MEMORY(header, result->header, sizeof (header));
    TEST_ASSERT_EQUAL_MEMORY(header_mask, result->header_mask, sizeof (header_mask));
    TEST_ASSERT_EQUAL(0, result->filter_size);
    TEST_ASSERT_EQUAL_MEMORY(zero_filter_mask, result->filter_mask, sizeof (zero_filter_mask));
    TEST_ASSERT_EQUAL_MEMORY(zero_filter_pattern,
            result->filter_pattern,
            sizeof (zero_filter_mask));
}

void test_lookup_create_no_memory(void) {
    uint8_t header[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
            0x81, 0x00, 0x3e, 0x08 };
    uint8_t header_mask[16] = { 0xFF };
    uint8_t filter_mask[] = { 0xFF };
    uint8_t filter_pattern[] = { 0xA0 };
    struct lookup *result;

    acm_zalloc_ExpectAndReturn(sizeof(struct lookup), NULL);
    logging_Expect(LOGLEVEL_ERR, "Lookup: Out of memory");
    result = lookup_create(header, header_mask, filter_mask, filter_pattern, sizeof (filter_mask));

    TEST_ASSERT_EQUAL_PTR(NULL, result);
}
