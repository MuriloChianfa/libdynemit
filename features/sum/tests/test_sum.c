#include "unity.h"
#include <stdint.h>
#include <math.h>
#include <dynemit/sum.h>

void setUp(void) {}
void tearDown(void) {}

static double naive_sum_f64(const double *d, size_t n)
{ double s = 0; for (size_t i = 0; i < n; i++) s += d[i]; return s; }

void test_sum_f64_empty(void)
{
    double d[1];
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, sum_f64(d, 0));
}

void test_sum_f64_small(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, sum_f64(d, 5));
}

void test_sum_f64_large(void)
{
    double d[256];
    for (int i = 0; i < 256; i++) d[i] = (double)i * 0.5;
    TEST_ASSERT_DOUBLE_WITHIN(1e-6 * fabs(naive_sum_f64(d, 256)),
                              naive_sum_f64(d, 256), sum_f64(d, 256));
}

void test_sum_u64_basic(void)
{
    uint64_t d[] = {1, 2, 3, 4, 5};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, sum_u64(d, 5));
}

void test_sum_u32_basic(void)
{
    uint32_t d[] = {10, 20, 30};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 60.0, sum_u32(d, 3));
}

void test_sum_u16_basic(void)
{
    uint16_t d[] = {100, 200, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 600.0, sum_u16(d, 3));
}

void test_sum_f64_all_variants(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        sum_f64_fn_t fn = sum_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, fn(d, 5));
    }
}

void test_sum_u64_all_variants(void)
{
    uint64_t d[] = {1, 2, 3, 4, 5};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        sum_u64_fn_t fn = sum_u64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, fn(d, 5));
    }
}

void test_sum_u32_all_variants(void)
{
    uint32_t d[] = {10, 20, 30};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        sum_u32_fn_t fn = sum_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 60.0, fn(d, 3));
    }
}

void test_sum_u16_all_variants(void)
{
    uint16_t d[] = {100, 200, 300};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        sum_u16_fn_t fn = sum_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 600.0, fn(d, 3));
    }
}

void test_sum_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(sum_f64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(sum_u64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(sum_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(sum_u16_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sum_f64_empty);
    RUN_TEST(test_sum_f64_small);
    RUN_TEST(test_sum_f64_large);
    RUN_TEST(test_sum_u64_basic);
    RUN_TEST(test_sum_u32_basic);
    RUN_TEST(test_sum_u16_basic);
    RUN_TEST(test_sum_f64_all_variants);
    RUN_TEST(test_sum_u64_all_variants);
    RUN_TEST(test_sum_u32_all_variants);
    RUN_TEST(test_sum_u16_all_variants);
    RUN_TEST(test_sum_select_all_levels);
    return UNITY_END();
}
