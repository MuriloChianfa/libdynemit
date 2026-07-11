#include "unity.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mem.h"

void setUp(void) {}
void tearDown(void) {}

void test_mem_align_up_exact(void)
{
    TEST_ASSERT_EQUAL_size_t(64, mem_align_up(64, 64));
    TEST_ASSERT_EQUAL_size_t(128, mem_align_up(128, 64));
}

void test_mem_align_up_rounds_up(void)
{
    TEST_ASSERT_EQUAL_size_t(64, mem_align_up(1, 64));
    TEST_ASSERT_EQUAL_size_t(64, mem_align_up(63, 64));
    TEST_ASSERT_EQUAL_size_t(128, mem_align_up(65, 64));
}

void test_mem_aligned_bytes_u32_scratch(void)
{
    static const size_t counts[] = {0, 1, 2, 7, 8, 9, 15, 16, 17, 100, 256};
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
        size_t n = counts[i];
        size_t raw = n * sizeof(uint32_t);
        size_t aligned = mem_aligned_count(n, uint32_t);
        TEST_ASSERT_GREATER_OR_EQUAL(raw, aligned);
        TEST_ASSERT_EQUAL_size_t(0, aligned % DYNEMIT_MEM_ALIGN);
        TEST_ASSERT_LESS_OR_EQUAL(DYNEMIT_MEM_ALIGN - 1, aligned - raw);
    }
}

void test_mem_aligned_bytes_double_matches_log2_pattern(void)
{
    TEST_ASSERT_EQUAL_size_t(64, mem_aligned_count(1, double));
    TEST_ASSERT_EQUAL_size_t(64, mem_aligned_count(8, double));
    TEST_ASSERT_EQUAL_size_t(128, mem_aligned_count(9, double));
}

void test_mem_aligned_bytes_u64_scratch(void)
{
    TEST_ASSERT_EQUAL_size_t(0, mem_aligned_count(0, uint64_t));
    TEST_ASSERT_EQUAL_size_t(64, mem_aligned_count(1, uint64_t));
    TEST_ASSERT_EQUAL_size_t(64, mem_aligned_count(8, uint64_t));
}

void test_memcpys_success(void)
{
    uint8_t src[] = {1, 2, 3, 4};
    uint8_t dst[4] = {0};
    TEST_ASSERT_EQUAL_INT(0, memcpys(dst, sizeof(dst), src, sizeof(src)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(src, dst, 4);
}

void test_memcpys_zero_count(void)
{
    uint8_t buf[4] = {0xAA, 0xAA, 0xAA, 0xAA};
    TEST_ASSERT_EQUAL_INT(0, memcpys(buf, sizeof(buf), buf, 0));
    TEST_ASSERT_EQUAL_UINT8(0xAA, buf[0]);
}

void test_memcpys_null_dest(void)
{
    TEST_ASSERT_EQUAL_INT(EINVAL, memcpys(NULL, 0, "ab", 2));
    TEST_ASSERT_EQUAL_INT(0, memcpys(NULL, 0, "ab", 0));
}

void test_memcpys_null_src(void)
{
    uint8_t dst[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    TEST_ASSERT_EQUAL_INT(EINVAL, memcpys(dst, sizeof(dst), NULL, 2));
    TEST_ASSERT_EQUAL_UINT8(0, dst[0]);
    TEST_ASSERT_EQUAL_UINT8(0, dst[1]);
    TEST_ASSERT_EQUAL_UINT8(0, dst[2]);
    TEST_ASSERT_EQUAL_UINT8(0, dst[3]);
}

void test_memcpys_count_exceeds_destsz(void)
{
    uint8_t dst[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    TEST_ASSERT_EQUAL_INT(EOVERFLOW, memcpys(dst, sizeof(dst), src, 8));
    for (size_t i = 0; i < sizeof(dst); i++)
        TEST_ASSERT_EQUAL_UINT8(0, dst[i]);
}

void test_memsets_success(void)
{
    uint8_t buf[8];
    TEST_ASSERT_EQUAL_INT(0, memsets(buf, sizeof(buf), 0xA5, sizeof(buf)));
    for (size_t i = 0; i < sizeof(buf); i++)
        TEST_ASSERT_EQUAL_UINT8(0xA5, buf[i]);
}

void test_memsets_null_dest(void)
{
    TEST_ASSERT_EQUAL_INT(EINVAL, memsets(NULL, 0, 0, 4));
    TEST_ASSERT_EQUAL_INT(0, memsets(NULL, 0, 0, 0));
}

void test_memsets_count_exceeds_destsz(void)
{
    uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    TEST_ASSERT_EQUAL_INT(EOVERFLOW, memsets(buf, sizeof(buf), 0xA5, 8));
    for (size_t i = 0; i < sizeof(buf); i++)
        TEST_ASSERT_EQUAL_UINT8(0, buf[i]);
}

void test_mem_aligned_alloc_roundtrip(void)
{
    size_t n = 17;
    size_t bytes = mem_aligned_count(n, uint32_t);
    uint32_t *p = aligned_alloc(DYNEMIT_MEM_ALIGN, bytes);
    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQUAL_INT(0, memsets(p, bytes, 0, bytes));
    free(p);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mem_align_up_exact);
    RUN_TEST(test_mem_align_up_rounds_up);
    RUN_TEST(test_mem_aligned_bytes_u32_scratch);
    RUN_TEST(test_mem_aligned_bytes_double_matches_log2_pattern);
    RUN_TEST(test_mem_aligned_bytes_u64_scratch);
    RUN_TEST(test_memcpys_success);
    RUN_TEST(test_memcpys_zero_count);
    RUN_TEST(test_memcpys_null_dest);
    RUN_TEST(test_memcpys_null_src);
    RUN_TEST(test_memcpys_count_exceeds_destsz);
    RUN_TEST(test_memsets_success);
    RUN_TEST(test_memsets_null_dest);
    RUN_TEST(test_memsets_count_exceeds_destsz);
    RUN_TEST(test_mem_aligned_alloc_roundtrip);
    return UNITY_END();
}
