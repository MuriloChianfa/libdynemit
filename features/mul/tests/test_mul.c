#include "unity.h"
#include <dynemit/mul.h>

void setUp(void) {}
void tearDown(void) {}

static void verify_mul_f32(size_t n)
{
    if (n == 0) {
        mul_f32(nullptr, nullptr, nullptr, 0);
        return;
    }
    float a[n];
    float b[n];
    float out[n];
    for (size_t i = 0; i < n; i++) {
        a[i] = (float)i * 0.5f;
        b[i] = (float)(i + 1) * 0.25f;
    }
    mul_f32(a, b, out, n);
    for (size_t i = 0; i < n; i++)
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, a[i] * b[i], out[i]);
}

void test_mul_f32_empty(void)  { verify_mul_f32(0); }
void test_mul_f32_n1(void)     { verify_mul_f32(1); }
void test_mul_f32_n7(void)     { verify_mul_f32(7); }
void test_mul_f32_n16(void)    { verify_mul_f32(16); }
void test_mul_f32_n31(void)    { verify_mul_f32(31); }
void test_mul_f32_n1024(void)  { verify_mul_f32(1024); }

void test_mul_f32_all_variants(void)
{
    float a[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    float b[] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    float out[16];
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        mul_f32_fn_t fn = mul_f32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        fn(a, b, out, 16);
        for (int i = 0; i < 16; i++)
            TEST_ASSERT_FLOAT_WITHIN(1e-6f, a[i] * b[i], out[i]);
    }
}

void test_mul_f32_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        mul_f32_fn_t fn = mul_f32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mul_f32_empty);
    RUN_TEST(test_mul_f32_n1);
    RUN_TEST(test_mul_f32_n7);
    RUN_TEST(test_mul_f32_n16);
    RUN_TEST(test_mul_f32_n31);
    RUN_TEST(test_mul_f32_n1024);
    RUN_TEST(test_mul_f32_all_variants);
    RUN_TEST(test_mul_f32_select_all_levels);
    return UNITY_END();
}
