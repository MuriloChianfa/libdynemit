#include "unity.h"
#include "fault_alloc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dynemit/radixs.h>

void setUp(void) {}
void tearDown(void)
{
    fault_alloc_reset();
}

/* ---- Reference comparators for qsort ---- */

static int cmp_u16(const void *a, const void *b)
{
    uint16_t x = *(const uint16_t *)a, y = *(const uint16_t *)b;
    return (x > y) - (x < y);
}
static int cmp_u32(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

/* ---- xorshift64 PRNG (deterministic, no <stdlib> rand state) ---- */

static uint64_t prng_state = 0x9e3779b97f4a7c15ULL;
static uint64_t prng_u64(void)
{
    uint64_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    prng_state = x;
    return x;
}
static void prng_seed(uint64_t s) { prng_state = s ? s : 0x9e3779b97f4a7c15ULL; }

/* ---- Basic edge cases (default-dispatched IFUNC) ---- */

void test_u16_empty(void)
{
    uint16_t out[1] = {0xdead};
    radixs_u16(NULL, out, 0);
    TEST_ASSERT_EQUAL_UINT16(0xdead, out[0]);
}

void test_u32_empty(void)
{
    uint32_t out[1] = {0xdeadbeef};
    radixs_u32(NULL, out, 0);
    TEST_ASSERT_EQUAL_UINT32(0xdeadbeefu, out[0]);
}

void test_u64_empty(void)
{
    uint64_t out[1] = {0xdeadbeefcafebabeULL};
    radixs_u64(NULL, out, 0);
    TEST_ASSERT_EQUAL_UINT64(0xdeadbeefcafebabeULL, out[0]);
}

void test_u16_single(void)
{
    uint16_t in[] = {42}, out[1] = {0};
    radixs_u16(in, out, 1);
    TEST_ASSERT_EQUAL_UINT16(42, out[0]);
}

void test_u32_single(void)
{
    uint32_t in[] = {12345u}, out[1] = {0};
    radixs_u32(in, out, 1);
    TEST_ASSERT_EQUAL_UINT32(12345u, out[0]);
}

void test_u64_single(void)
{
    uint64_t in[] = {0x123456789abcdef0ULL}, out[1] = {0};
    radixs_u64(in, out, 1);
    TEST_ASSERT_EQUAL_UINT64(0x123456789abcdef0ULL, out[0]);
}

void test_u16_basic(void)
{
    uint16_t in[]  = {5, 3, 8, 1, 4};
    uint16_t exp[] = {1, 3, 4, 5, 8};
    uint16_t out[5];
    radixs_u16(in, out, 5);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(exp, out, 5);
}

void test_u32_basic(void)
{
    uint32_t in[]  = {0xff, 0x10000, 0x1, 0xff00, 0x100};
    uint32_t exp[] = {0x1, 0xff, 0x100, 0xff00, 0x10000};
    uint32_t out[5];
    radixs_u32(in, out, 5);
    TEST_ASSERT_EQUAL_UINT32_ARRAY(exp, out, 5);
}

void test_u64_basic(void)
{
    uint64_t in[]  = {0xffffffffffffffffULL, 1ULL, 0x100000000ULL, 0, 7};
    uint64_t exp[] = {0, 1, 7, 0x100000000ULL, 0xffffffffffffffffULL};
    uint64_t out[5];
    radixs_u64(in, out, 5);
    TEST_ASSERT_EQUAL_UINT64_ARRAY(exp, out, 5);
}

/* ---- Per-variant generic harness ---- */

static const size_t TEST_SIZES[] = {
    0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 33, 64, 100, 256, 257, 1023, 1024, 4096
};
#define N_TEST_SIZES ((int)(sizeof(TEST_SIZES) / sizeof(TEST_SIZES[0])))

static void run_variant_u16(radixs_u16_fn_t fn)
{
    for (int s = 0; s < N_TEST_SIZES; s++) {
        size_t n = TEST_SIZES[s];
        if (n == 0) {
            uint16_t guard = 0xbeef;
            fn(NULL, &guard, 0);
            TEST_ASSERT_EQUAL_UINT16(0xbeef, guard);
            continue;
        }

        uint16_t *in  = malloc(n * sizeof(uint16_t));
        uint16_t *out = malloc(n * sizeof(uint16_t));
        uint16_t *ref = malloc(n * sizeof(uint16_t));
        TEST_ASSERT_NOT_NULL(in);
        TEST_ASSERT_NOT_NULL(out);
        TEST_ASSERT_NOT_NULL(ref);

        prng_seed(0xC0FFEEULL ^ (uint64_t)n);
        for (size_t i = 0; i < n; i++)
            in[i] = (uint16_t)(prng_u64() & 0xffff);
        memcpy(ref, in, n * sizeof(uint16_t));
        qsort(ref, n, sizeof(uint16_t), cmp_u16);
        memset(out, 0xa5, n * sizeof(uint16_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT16_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = (uint16_t)i;
        memcpy(ref, in, n * sizeof(uint16_t));
        qsort(ref, n, sizeof(uint16_t), cmp_u16);
        memset(out, 0xa5, n * sizeof(uint16_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT16_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = (uint16_t)(n - 1 - i);
        memcpy(ref, in, n * sizeof(uint16_t));
        qsort(ref, n, sizeof(uint16_t), cmp_u16);
        memset(out, 0xa5, n * sizeof(uint16_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT16_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = 7;
        memset(out, 0xa5, n * sizeof(uint16_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT16(7, out[i]);

        free(in); free(out); free(ref);
    }
}

static void run_variant_u32(radixs_u32_fn_t fn)
{
    for (int s = 0; s < N_TEST_SIZES; s++) {
        size_t n = TEST_SIZES[s];
        if (n == 0) {
            uint32_t guard = 0xbeef;
            fn(NULL, &guard, 0);
            TEST_ASSERT_EQUAL_UINT32(0xbeefu, guard);
            continue;
        }

        uint32_t *in  = malloc(n * sizeof(uint32_t));
        uint32_t *out = malloc(n * sizeof(uint32_t));
        uint32_t *ref = malloc(n * sizeof(uint32_t));
        TEST_ASSERT_NOT_NULL(in);
        TEST_ASSERT_NOT_NULL(out);
        TEST_ASSERT_NOT_NULL(ref);

        prng_seed(0xCAFEBABEULL ^ (uint64_t)n);
        for (size_t i = 0; i < n; i++)
            in[i] = (uint32_t)prng_u64();
        memcpy(ref, in, n * sizeof(uint32_t));
        qsort(ref, n, sizeof(uint32_t), cmp_u32);
        memset(out, 0xa5, n * sizeof(uint32_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT32_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = (uint32_t)i;
        memcpy(ref, in, n * sizeof(uint32_t));
        qsort(ref, n, sizeof(uint32_t), cmp_u32);
        memset(out, 0xa5, n * sizeof(uint32_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT32_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = (uint32_t)(n - 1 - i);
        memcpy(ref, in, n * sizeof(uint32_t));
        qsort(ref, n, sizeof(uint32_t), cmp_u32);
        memset(out, 0xa5, n * sizeof(uint32_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT32_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = 0xdeadbeefu;
        memset(out, 0xa5, n * sizeof(uint32_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT32(0xdeadbeefu, out[i]);

        for (size_t i = 0; i < n; i++) in[i] = 0u;
        memset(out, 0xa5, n * sizeof(uint32_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT32(0u, out[i]);

        for (size_t i = 0; i < n; i++) in[i] = UINT32_MAX;
        memset(out, 0xa5, n * sizeof(uint32_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, out[i]);

        free(in); free(out); free(ref);
    }
}

static void run_variant_u64(radixs_u64_fn_t fn)
{
    for (int s = 0; s < N_TEST_SIZES; s++) {
        size_t n = TEST_SIZES[s];
        if (n == 0) {
            uint64_t guard = 0xbeefULL;
            fn(NULL, &guard, 0);
            TEST_ASSERT_EQUAL_UINT64(0xbeefULL, guard);
            continue;
        }

        uint64_t *in  = malloc(n * sizeof(uint64_t));
        uint64_t *out = malloc(n * sizeof(uint64_t));
        uint64_t *ref = malloc(n * sizeof(uint64_t));
        TEST_ASSERT_NOT_NULL(in);
        TEST_ASSERT_NOT_NULL(out);
        TEST_ASSERT_NOT_NULL(ref);

        prng_seed(0xDEADBEEFULL ^ (uint64_t)n);
        for (size_t i = 0; i < n; i++)
            in[i] = prng_u64();
        memcpy(ref, in, n * sizeof(uint64_t));
        qsort(ref, n, sizeof(uint64_t), cmp_u64);
        memset(out, 0xa5, n * sizeof(uint64_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT64_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = (uint64_t)i;
        memcpy(ref, in, n * sizeof(uint64_t));
        qsort(ref, n, sizeof(uint64_t), cmp_u64);
        memset(out, 0xa5, n * sizeof(uint64_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT64_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = (uint64_t)(n - 1 - i);
        memcpy(ref, in, n * sizeof(uint64_t));
        qsort(ref, n, sizeof(uint64_t), cmp_u64);
        memset(out, 0xa5, n * sizeof(uint64_t));
        fn(in, out, n);
        TEST_ASSERT_EQUAL_UINT64_ARRAY(ref, out, n);

        for (size_t i = 0; i < n; i++) in[i] = UINT64_MAX;
        memset(out, 0xa5, n * sizeof(uint64_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, out[i]);

        for (size_t i = 0; i < n; i++) in[i] = 0u;
        memset(out, 0xa5, n * sizeof(uint64_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT64(0u, out[i]);

        for (size_t i = 0; i < n; i++) in[i] = 0xdeadbeefcafebabeULL;
        memset(out, 0xa5, n * sizeof(uint64_t));
        fn(in, out, n);
        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_EQUAL_UINT64(0xdeadbeefcafebabeULL, out[i]);

        free(in); free(out); free(ref);
    }
}

/* ---- All-variant tests via *_select() ---- */

void test_radixs_u16_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        radixs_u16_fn_t fn = radixs_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_u16(fn);
    }
}

void test_radixs_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        radixs_u32_fn_t fn = radixs_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_u32(fn);
    }
}

void test_radixs_u64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        radixs_u64_fn_t fn = radixs_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_u64(fn);
    }
}

void test_radixs_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(radixs_u16_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(radixs_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(radixs_u64_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

/*
 * Ensure SIMD_AVX512_VBMI2 dispatches to a non-null function pointer
 * even on hardware that doesn't advertise VBMI2; the IFUNC machinery
 * will just never pick it on those CPUs but _select must remain total.
 */
void test_radixs_vbmi2_select_total(void)
{
    TEST_ASSERT_NOT_NULL(radixs_u16_select(SIMD_AVX512_VBMI2));
    TEST_ASSERT_NOT_NULL(radixs_u32_select(SIMD_AVX512_VBMI2));
    TEST_ASSERT_NOT_NULL(radixs_u64_select(SIMD_AVX512_VBMI2));
}

#if defined(__x86_64__) || defined(__i386__)
static void
run_avx512_conflict_u32(radixs_u32_fn_t fn)
{
    uint32_t in[16];
    uint32_t out[16];
    uint32_t ref[16];
    for (int i = 0; i < 16; i++) {
        in[i] = (uint32_t)(i << 8);
    }
    memcpy(ref, in, sizeof(in));
    qsort(ref, 16, sizeof(uint32_t), cmp_u32);
    fn(in, out, 16);
    TEST_ASSERT_EQUAL_UINT32_ARRAY(ref, out, 16);
}

static void
run_avx512_conflict_u64(radixs_u64_fn_t fn)
{
    uint64_t in[8];
    uint64_t out[8];
    uint64_t ref[8];
    for (int i = 0; i < 8; i++) {
        in[i] = (uint64_t)(i << 8);
    }
    memcpy(ref, in, sizeof(in));
    qsort(ref, 8, sizeof(uint64_t), cmp_u64);
    fn(in, out, 8);
    TEST_ASSERT_EQUAL_UINT64_ARRAY(ref, out, 8);
}
#endif

void test_radixs_avx512_conflict(void)
{
#if defined(__x86_64__) || defined(__i386__)
    if (detect_simd_level() < SIMD_AVX512F) {
        TEST_PASS();
    }
    run_avx512_conflict_u32(radixs_u32_select(SIMD_AVX512F));
    run_avx512_conflict_u64(radixs_u64_select(SIMD_AVX512F));
    if (detect_simd_level() >= SIMD_AVX512_VBMI2) {
        run_avx512_conflict_u32(radixs_u32_select(SIMD_AVX512_VBMI2));
        run_avx512_conflict_u64(radixs_u64_select(SIMD_AVX512_VBMI2));
    }
#else
    TEST_PASS();
#endif
}

static void
verify_sorted_u32(const uint32_t *in, const uint32_t *out, size_t n)
{
    uint32_t *ref = malloc(n * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(ref);
    memcpy(ref, in, n * sizeof(uint32_t));
    qsort(ref, n, sizeof(uint32_t), cmp_u32);
    TEST_ASSERT_EQUAL_UINT32_ARRAY(ref, out, n);
    free(ref);
}

static void
verify_sorted_u64(const uint64_t *in, const uint64_t *out, size_t n)
{
    uint64_t *ref = malloc(n * sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(ref);
    memcpy(ref, in, n * sizeof(uint64_t));
    qsort(ref, n, sizeof(uint64_t), cmp_u64);
    TEST_ASSERT_EQUAL_UINT64_ARRAY(ref, out, n);
    free(ref);
}

void test_radixs_u32_alloc_fail_fallback(void)
{
    uint32_t in[] = {5, 3, 8, 1, 4, 7, 2, 6};
    uint32_t out[8];
    radixs_u32_fn_t fn = radixs_u32_select(SIMD_AVX2);
    if (detect_simd_level() < SIMD_AVX2) {
        fn = radixs_u32_select(SIMD_SCALAR);
    }

    fault_alloc_fail_nth_aligned_alloc(1);
    fn(in, out, 8);
    verify_sorted_u32(in, out, 8);

    fault_alloc_fail_nth_aligned_alloc(2);
    fn(in, out, 8);
    verify_sorted_u32(in, out, 8);
}

void test_radixs_u64_alloc_fail_fallback(void)
{
    uint64_t in[] = {5, 3, 8, 1, 4, 7, 2, 6};
    uint64_t out[8];
    radixs_u64_fn_t fn = radixs_u64_select(SIMD_AVX2);
    if (detect_simd_level() < SIMD_AVX2) {
        fn = radixs_u64_select(SIMD_SCALAR);
    }

    fault_alloc_fail_nth_aligned_alloc(1);
    fn(in, out, 8);
    verify_sorted_u64(in, out, 8);

    fault_alloc_fail_nth_aligned_alloc(2);
    fn(in, out, 8);
    verify_sorted_u64(in, out, 8);
}

void test_radixs_avx512_alloc_fail_fallback(void)
{
#if defined(__x86_64__) || defined(__i386__)
    if (detect_simd_level() < SIMD_AVX512F) {
        TEST_PASS();
    }
    uint32_t in32[] = {5, 3, 8, 1, 4, 7, 2, 6};
    uint32_t out32[8];
    uint64_t in64[] = {5, 3, 8, 1, 4, 7, 2, 6};
    uint64_t out64[8];

    radixs_u32_fn_t fn32 = radixs_u32_select(SIMD_AVX512F);
    fault_alloc_fail_nth_aligned_alloc(1);
    fn32(in32, out32, 8);
    verify_sorted_u32(in32, out32, 8);

    radixs_u64_fn_t fn64 = radixs_u64_select(SIMD_AVX512F);
    fault_alloc_fail_nth_aligned_alloc(1);
    fn64(in64, out64, 8);
    verify_sorted_u64(in64, out64, 8);
#else
    TEST_PASS();
#endif
}

static void
verify_sorted_u16(const uint16_t *in, const uint16_t *out, size_t n)
{
    uint16_t *ref = malloc(n * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(ref);
    memcpy(ref, in, n * sizeof(uint16_t));
    qsort(ref, n, sizeof(uint16_t), cmp_u16);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ref, out, n);
    free(ref);
}

void test_radixs_u16_alloc_fail_fallback(void)
{
    uint16_t in[] = {5, 3, 8, 1, 4, 7, 2, 6};
    uint16_t out[8];
    radixs_u16_fn_t fn = radixs_u16_select(SIMD_AVX2);
    if (detect_simd_level() < SIMD_AVX2) {
        fn = radixs_u16_select(SIMD_SCALAR);
    }

    fault_alloc_fail_nth_aligned_alloc(1);
    fn(in, out, 8);
    verify_sorted_u16(in, out, 8);
}

void test_radixs_u16_large_dense(void)
{
    static const size_t N = 4096;
    uint16_t *in = malloc(N * sizeof(uint16_t));
    uint16_t *out = malloc(N * sizeof(uint16_t));
    uint16_t *ref = malloc(N * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(in);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(ref);

    for (size_t i = 0; i < N; i++) {
        in[i] = (uint16_t)(i % 1024);
    }
    memcpy(ref, in, N * sizeof(uint16_t));
    qsort(ref, N, sizeof(uint16_t), cmp_u16);

    radixs_u16_fn_t fn = radixs_u16_select(SIMD_SCALAR);
    if (detect_simd_level() >= SIMD_AVX2) {
        fn = radixs_u16_select(SIMD_AVX2);
    }
    fn(in, out, N);
    TEST_ASSERT_EQUAL_UINT16_ARRAY(ref, out, N);

    free(in);
    free(out);
    free(ref);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_u16_empty);
    RUN_TEST(test_u32_empty);
    RUN_TEST(test_u64_empty);
    RUN_TEST(test_u16_single);
    RUN_TEST(test_u32_single);
    RUN_TEST(test_u64_single);
    RUN_TEST(test_u16_basic);
    RUN_TEST(test_u32_basic);
    RUN_TEST(test_u64_basic);

    RUN_TEST(test_radixs_u16_all_variants);
    RUN_TEST(test_radixs_u32_all_variants);
    RUN_TEST(test_radixs_u64_all_variants);

    RUN_TEST(test_radixs_select_all_levels);
    RUN_TEST(test_radixs_vbmi2_select_total);
    RUN_TEST(test_radixs_avx512_conflict);

    RUN_TEST(test_radixs_u32_alloc_fail_fallback);
    RUN_TEST(test_radixs_u64_alloc_fail_fallback);
    RUN_TEST(test_radixs_u16_alloc_fail_fallback);
    RUN_TEST(test_radixs_avx512_alloc_fail_fallback);
    RUN_TEST(test_radixs_u16_large_dense);

    return UNITY_END();
}
