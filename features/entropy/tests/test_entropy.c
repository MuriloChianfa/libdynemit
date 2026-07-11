#include "unity.h"
#include "fault_alloc.h"
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <dynemit/entropy.h>

void setUp(void) {}
void tearDown(void)
{
    fault_alloc_reset();
}

void test_entropy_u16_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u16(NULL, 0));
}

void test_entropy_u16_constant(void)
{
    uint16_t d[] = {5, 5, 5, 5, 5, 5, 5, 5};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u16(d, 8));
}

void test_entropy_u16_two_values(void)
{
    uint16_t d[] = {0, 1, 0, 1, 0, 1, 0, 1};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, entropy_u16(d, 8));
}

void test_entropy_u32_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u32(NULL, 0));
}

void test_entropy_u32_constant(void)
{
    uint32_t d[] = {42, 42, 42, 42};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u32(d, 4));
}

void test_entropy_histogram_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_histogram(NULL, 0));
}

void test_entropy_histogram_equal(void)
{
    uint64_t c[] = {10, 10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, entropy_histogram(c, 2));
}

static void run_u16_variant_sizes(entropy_u16_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint16_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = 42;
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, n));
    }
}

static void run_u32_variant_sizes(entropy_u32_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint32_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = 42;
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, n));
    }
}

static void run_histogram_variant_sizes(entropy_histogram_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint64_t c[256];
        for (size_t i = 0; i < n; i++) c[i] = 10;
        double result = fn(c, n);
        TEST_ASSERT_TRUE(result >= 0.0);
    }
}

void test_entropy_u16_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u16_variant_sizes(fn);
    }
}

void test_entropy_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u32_fn_t fn = entropy_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u32_variant_sizes(fn);
    }
}

void test_entropy_histogram_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_histogram_fn_t fn = entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_histogram_variant_sizes(fn);
    }
}

void test_entropy_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(entropy_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

static double reference_entropy_u16(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint32_t hist[65536] = {0};
    for (size_t i = 0; i < n; i++) hist[data[i]]++;
    double inv_n = 1.0 / (double)n;
    double h = 0.0;
    for (size_t i = 0; i < 65536; i++) {
        if (hist[i] == 0) continue;
        double p = (double)hist[i] * inv_n;
        h -= p * log2(p);
    }
    return h;
}

void test_entropy_u16_precision_all_variants(void)
{
    static const size_t N = 1024;
    uint16_t *d = malloc(N * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++)
        d[i] = (uint16_t)(i % 64);

    double ref = reference_entropy_u16(d, N);
    TEST_ASSERT_TRUE(ref > 0.0);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
    free(d);
}

void test_entropy_u16_dense(void)
{
    static const size_t N = 65536;
    uint16_t *d = malloc(N * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++)
        d[i] = (uint16_t)i;

    double ref = reference_entropy_u16(d, N);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 16.0, ref);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
    free(d);
}

void test_entropy_histogram_precision_all_variants(void)
{
    static const size_t N = 128;
    uint64_t c[128];
    for (size_t i = 0; i < N; i++)
        c[i] = (i + 1) * 3;

    entropy_histogram_fn_t scalar_fn = entropy_histogram_select(SIMD_SCALAR);
    double ref = scalar_fn(c, N);
    TEST_ASSERT_TRUE(ref > 0.0);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_histogram_fn_t fn = entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(c, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
}

static int
cmp_u32_ref(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a;
    uint32_t y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

static double
reference_entropy_u32(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *sorted = malloc(n * sizeof(uint32_t));
    if (!sorted) {
        return 0.0;
    }
    for (size_t i = 0; i < n; i++) {
        sorted[i] = data[i];
    }
    qsort(sorted, n, sizeof(uint32_t), cmp_u32_ref);

    size_t cap = 256;
    uint64_t *cnts = malloc(cap * sizeof(uint64_t));
    if (!cnts) {
        free(sorted);
        return 0.0;
    }
    size_t num_cnts = 0;
    uint64_t run = 1;
    for (size_t i = 1; i < n; i++) {
        if (sorted[i] == sorted[i - 1]) {
            run++;
        } else {
            if (num_cnts >= cap) {
                cap *= 2;
                uint64_t *tmp = realloc(cnts, cap * sizeof(uint64_t));
                if (!tmp) {
                    free(cnts);
                    free(sorted);
                    return 0.0;
                }
                cnts = tmp;
            }
            cnts[num_cnts++] = run;
            run = 1;
        }
    }
    cnts[num_cnts++] = run;

    double h = entropy_histogram(cnts, num_cnts);
    free(cnts);
    free(sorted);
    return h;
}

void test_entropy_u32_precision_all_variants(void)
{
    static const size_t N = 1024;
    uint32_t *d = malloc(N * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) {
        d[i] = (uint32_t)(i % 64);
    }

    double ref = reference_entropy_u32(d, N);
    TEST_ASSERT_TRUE(ref > 0.0);

    entropy_u32_fn_t scalar_fn = entropy_u32_select(SIMD_SCALAR);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, scalar_fn(d, N));

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u32_fn_t fn = entropy_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }

    static const size_t N_UNIQUE = 512;
    for (size_t i = 0; i < N_UNIQUE; i++) {
        d[i] = (uint32_t)i;
    }
    ref = reference_entropy_u32(d, N_UNIQUE);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, scalar_fn(d, N_UNIQUE));
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u32_fn_t fn = entropy_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N_UNIQUE);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }

    free(d);
}

void test_entropy_u16_dirty_cleanup(void)
{
    static const size_t N = 600;
    uint16_t *d = malloc(N * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) {
        d[i] = (uint16_t)i;
    }

    double ref = reference_entropy_u16(d, N);
    TEST_ASSERT_TRUE(ref > 0.0);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
    free(d);
}

void test_entropy_u16_buf_alloc_fail(void)
{
    uint16_t d[] = {0, 1, 2, 3, 4, 5};
    fault_alloc_fail_next_aligned_alloc();
    entropy_u16_fn_t fn = entropy_u16_select(SIMD_SCALAR);
    double got = fn(d, 6);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, got);
}

void test_entropy_u32_malloc_fail(void)
{
    uint32_t d[64];
    for (size_t i = 0; i < 64; i++) {
        d[i] = (uint32_t)(i % 16);
    }

    entropy_u32_fn_t scalar_fn = entropy_u32_select(SIMD_SCALAR);

    fault_alloc_fail_next_malloc();
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, scalar_fn(d, 64));

    fault_alloc_fail_nth_malloc(2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, scalar_fn(d, 64));
    fault_alloc_reset();
}

void test_entropy_u32_calloc_fail(void)
{
#if defined(__x86_64__) || defined(__i386__)
    uint32_t d[64];
    for (size_t i = 0; i < 64; i++) {
        d[i] = (uint32_t)(i % 16);
    }

    entropy_u32_fn_t fn = entropy_u32_select(SIMD_AVX2);
    if (detect_simd_level() < SIMD_AVX2) {
        fn = entropy_u32_select(SIMD_SSE2);
    }

    fault_alloc_fail_nth_calloc(1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, 64));

    fault_alloc_fail_nth_calloc(2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, 64));
    fault_alloc_reset();
#else
    TEST_PASS();
#endif
}

void test_entropy_u32_realloc_fail(void)
{
    static const size_t N = 512;
    uint32_t *d = malloc(N * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) {
        d[i] = (uint32_t)i;
    }

    entropy_u32_fn_t scalar_fn = entropy_u32_select(SIMD_SCALAR);
    fault_alloc_fail_nth_realloc(1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, scalar_fn(d, N));
    fault_alloc_reset();
    free(d);
}

void test_entropy_histogram_all_zero(void)
{
    uint64_t c[17] = {0};
    entropy_histogram_fn_t scalar_fn = entropy_histogram_select(SIMD_SCALAR);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, scalar_fn(c, 17));

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_histogram_fn_t fn = entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(c, 17));
    }
}

void test_entropy_histogram_precision_small(void)
{
    uint64_t c[17];
    for (size_t i = 0; i < 17; i++) {
        c[i] = (i + 1) * 2;
    }

    entropy_histogram_fn_t scalar_fn = entropy_histogram_select(SIMD_SCALAR);
    double ref = scalar_fn(c, 17);
    TEST_ASSERT_TRUE(ref > 0.0);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_histogram_fn_t fn = entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, fn(c, 17));
    }
}

static void *
entropy_u16_worker(void *arg)
{
    (void)arg;
    static const size_t N = 256;
    uint16_t d[N];
    for (size_t i = 0; i < N; i++) {
        d[i] = (uint16_t)(i % 48);
    }
    entropy_u16_fn_t fn = entropy_u16_select(SIMD_SCALAR);
    double got = fn(d, N);
    TEST_ASSERT_TRUE(got > 0.0);
    return NULL;
}

void test_entropy_u16_threaded(void)
{
    pthread_t threads[4];
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, entropy_u16_worker, NULL));
    }
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_entropy_u16_buf_alloc_fail);

    RUN_TEST(test_entropy_u16_empty);
    RUN_TEST(test_entropy_u16_constant);
    RUN_TEST(test_entropy_u16_two_values);

    RUN_TEST(test_entropy_u32_empty);
    RUN_TEST(test_entropy_u32_constant);

    RUN_TEST(test_entropy_histogram_empty);
    RUN_TEST(test_entropy_histogram_equal);

    RUN_TEST(test_entropy_u16_all_variants);
    RUN_TEST(test_entropy_u32_all_variants);
    RUN_TEST(test_entropy_histogram_all_variants);

    RUN_TEST(test_entropy_select_all_levels);

    RUN_TEST(test_entropy_u16_precision_all_variants);
    RUN_TEST(test_entropy_u16_dense);
    RUN_TEST(test_entropy_u16_dirty_cleanup);
    RUN_TEST(test_entropy_histogram_precision_all_variants);

    RUN_TEST(test_entropy_u32_precision_all_variants);

    RUN_TEST(test_entropy_u32_malloc_fail);
    RUN_TEST(test_entropy_u32_calloc_fail);
    RUN_TEST(test_entropy_u32_realloc_fail);

    RUN_TEST(test_entropy_histogram_all_zero);
    RUN_TEST(test_entropy_histogram_precision_small);
    RUN_TEST(test_entropy_u16_threaded);

    return UNITY_END();
}
