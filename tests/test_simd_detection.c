#include "unity.h"
#include "cpuid_mock.h"
#include <dynemit/core.h>

void setUp(void)
{
    cpuid_mock_reset();
}

void tearDown(void)
{
    cpuid_mock_reset();
}

void test_detect_cpuid_eax0_zero(void)
{
#if defined(__x86_64__) || defined(__i386__)
    cpuid_mock_set(CPUID_MOCK_EAX0_ZERO);
    TEST_ASSERT_EQUAL_INT(SIMD_SCALAR, detect_simd_level());
#else
    TEST_PASS();
#endif
}

void test_detect_avx512_without_zmm(void)
{
#if defined(__x86_64__) || defined(__i386__)
    simd_level_t baseline = detect_simd_level();
    cpuid_mock_set(CPUID_MOCK_AVX512_NO_ZMM);
    simd_level_t mocked = detect_simd_level();
    if (baseline >= SIMD_AVX512F) {
        TEST_ASSERT_TRUE(mocked <= SIMD_AVX2);
        TEST_ASSERT_TRUE(mocked >= SIMD_AVX2);
    } else {
        TEST_ASSERT_EQUAL_INT(baseline, mocked);
    }
#else
    TEST_PASS();
#endif
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_detect_cpuid_eax0_zero);
    RUN_TEST(test_detect_avx512_without_zmm);
    return UNITY_END();
}
