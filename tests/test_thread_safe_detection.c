/**
 * @file test_thread_safe_detection.c
 * @brief Tests for detect_simd_level_ts() thread-safe SIMD detection
 */

#include <dynemit/core.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

#define NUM_THREADS 8
#define ITERATIONS_PER_THREAD 10000

// Shared results from all threads
static simd_level_t thread_results[NUM_THREADS];
static int thread_iteration_results[NUM_THREADS][ITERATIONS_PER_THREAD];

/**
 * Thread function that calls detect_simd_level_ts() many times
 * and stores all results for verification.
 */
static void* thread_func(void* arg)
{
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
        simd_level_t level = detect_simd_level_ts();
        thread_iteration_results[thread_id][i] = (int)level;
        
        // Store final result
        if (i == ITERATIONS_PER_THREAD - 1) {
            thread_results[thread_id] = level;
        }
    }
    
    return NULL;
}

/**
 * Test that detect_simd_level_ts() returns the same value as detect_simd_level()
 */
static int test_consistency_with_detect_simd_level(void)
{
    printf("  Testing consistency with detect_simd_level()... ");
    
    simd_level_t expected = detect_simd_level();
    simd_level_t cached = detect_simd_level_ts();
    
    if (expected != cached) {
        printf("FAIL\n");
        printf("    detect_simd_level() returned %d (%s)\n", 
               expected, simd_level_name(expected));
        printf("    detect_simd_level_ts() returned %d (%s)\n", 
               cached, simd_level_name(cached));
        return 1;
    }
    
    printf("OK (%s)\n", simd_level_name(cached));
    return 0;
}

/**
 * Test that multiple calls return the same cached value
 */
static int test_caching(void)
{
    printf("  Testing caching behavior... ");
    
    simd_level_t first = detect_simd_level_ts();
    simd_level_t second = detect_simd_level_ts();
    simd_level_t third = detect_simd_level_ts();
    
    if (first != second || second != third) {
        printf("FAIL\n");
        printf("    Results differ: %d, %d, %d\n", first, second, third);
        return 1;
    }
    
    printf("OK\n");
    return 0;
}

/**
 * Test thread safety with multiple concurrent threads
 */
static int test_thread_safety(void)
{
    printf("  Testing thread safety with %d threads... ", NUM_THREADS);
    
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    // Start all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            printf("FAIL (pthread_create)\n");
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify all threads got the same result
    simd_level_t expected = thread_results[0];
    for (int i = 1; i < NUM_THREADS; i++) {
        if (thread_results[i] != expected) {
            printf("FAIL\n");
            printf("    Thread 0 got %d (%s)\n", 
                   expected, simd_level_name(expected));
            printf("    Thread %d got %d (%s)\n", 
                   i, thread_results[i], simd_level_name(thread_results[i]));
            return 1;
        }
    }
    
    // Verify all iterations within each thread got the same result
    for (int t = 0; t < NUM_THREADS; t++) {
        for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
            if (thread_iteration_results[t][i] != (int)expected) {
                printf("FAIL\n");
                printf("    Thread %d iteration %d got %d, expected %d\n",
                       t, i, thread_iteration_results[t][i], (int)expected);
                return 1;
            }
        }
    }
    
    printf("OK (%d threads x %d iterations)\n", NUM_THREADS, ITERATIONS_PER_THREAD);
    return 0;
}

/**
 * Test that the cached value is valid
 */
static int test_valid_simd_level(void)
{
    printf("  Testing valid SIMD level range... ");
    
    simd_level_t level = detect_simd_level_ts();
    
    if (level < SIMD_SCALAR || level > SIMD_AVX512F) {
        printf("FAIL\n");
        printf("    Got invalid level: %d\n", level);
        return 1;
    }
    
    printf("OK (level=%d)\n", level);
    return 0;
}

int main(void)
{
    int failures = 0;
    
    printf("Testing detect_simd_level_ts():\n");
    printf("  Detected SIMD level: %s\n", simd_level_name(detect_simd_level()));
    printf("\n");
    
    failures += test_consistency_with_detect_simd_level();
    failures += test_caching();
    failures += test_thread_safety();
    failures += test_valid_simd_level();
    
    printf("\n");
    if (failures == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed!\n", failures);
        return 1;
    }
}
