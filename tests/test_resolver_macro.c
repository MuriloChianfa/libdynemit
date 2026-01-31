/**
 * @file test_resolver_macro.c
 * @brief Tests for EXPLICIT_RUNTIME_RESOLVER macro
 */

#include <dynemit/core.h>
#include <dynemit/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Sample implementations for testing
// ============================================================================

static int sample_func_avx2(int x) { return x * 4; }
static int sample_func_sse2(int x) { return x * 2; }
static int sample_func_scalar(int x) { return x * 1; }

typedef int (*sample_func_t)(int);

// ============================================================================
// Test resolver using EXPLICIT_RUNTIME_RESOLVER
// ============================================================================

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

EXPLICIT_RUNTIME_RESOLVER(sample_resolver)
{
    simd_level_t level = detect_simd_level_ts();
    
    switch (level) {
    case SIMD_AVX512F:
    case SIMD_AVX2:
        return (void*)sample_func_avx2;
    case SIMD_AVX:
    case SIMD_SSE4_2:
    case SIMD_SSE2:
        return (void*)sample_func_sse2;
    case SIMD_SCALAR:
    default:
        return (void*)sample_func_scalar;
    }
}

// Resolver that always returns a specific implementation (for testing)
EXPLICIT_RUNTIME_RESOLVER(scalar_only_resolver)
{
    (void)detect_simd_level_ts(); // Still use cached detection
    return (void*)sample_func_scalar;
}

#pragma GCC diagnostic pop

// ============================================================================
// Tests
// ============================================================================

/**
 * Test that the resolver returns a valid function pointer
 */
static int test_resolver_returns_valid_pointer(void)
{
    printf("  Testing resolver returns valid pointer... ");
    
    void* ptr = sample_resolver();
    
    if (ptr == NULL) {
        printf("FAIL (returned NULL)\n");
        return 1;
    }
    
    // Cast to function pointer and call it
    sample_func_t func = (sample_func_t)ptr;
    int result = func(10);
    
    // Result should be 10, 20, or 40 depending on SIMD level
    if (result != 10 && result != 20 && result != 40) {
        printf("FAIL (unexpected result: %d)\n", result);
        return 1;
    }
    
    printf("OK (result=%d)\n", result);
    return 0;
}

/**
 * Test that the resolver is consistent with SIMD level
 */
static int test_resolver_matches_simd_level(void)
{
    printf("  Testing resolver matches SIMD level... ");
    
    simd_level_t level = detect_simd_level_ts();
    void* ptr = sample_resolver();
    sample_func_t func = (sample_func_t)ptr;
    int result = func(10);
    
    int expected;
    switch (level) {
    case SIMD_AVX512F:
    case SIMD_AVX2:
        expected = 40;  // sample_func_avx2
        break;
    case SIMD_AVX:
    case SIMD_SSE4_2:
    case SIMD_SSE2:
        expected = 20;  // sample_func_sse2
        break;
    case SIMD_SCALAR:
    default:
        expected = 10;  // sample_func_scalar
        break;
    }
    
    if (result != expected) {
        printf("FAIL\n");
        printf("    SIMD level: %s\n", simd_level_name(level));
        printf("    Expected result: %d, got: %d\n", expected, result);
        return 1;
    }
    
    printf("OK (level=%s, result=%d)\n", simd_level_name(level), result);
    return 0;
}

/**
 * Test that repeated calls return the same pointer
 */
static int test_resolver_caching(void)
{
    printf("  Testing resolver returns consistent pointer... ");
    
    void* ptr1 = sample_resolver();
    void* ptr2 = sample_resolver();
    void* ptr3 = sample_resolver();
    
    if (ptr1 != ptr2 || ptr2 != ptr3) {
        printf("FAIL\n");
        printf("    Pointers differ: %p, %p, %p\n", ptr1, ptr2, ptr3);
        return 1;
    }
    
    printf("OK\n");
    return 0;
}

/**
 * Test that the scalar-only resolver works
 */
static int test_scalar_only_resolver(void)
{
    printf("  Testing scalar-only resolver... ");
    
    void* ptr = scalar_only_resolver();
    
    if (ptr == NULL) {
        printf("FAIL (returned NULL)\n");
        return 1;
    }
    
    sample_func_t func = (sample_func_t)ptr;
    int result = func(10);
    
    if (result != 10) {
        printf("FAIL (expected 10, got %d)\n", result);
        return 1;
    }
    
    printf("OK\n");
    return 0;
}

/**
 * Test macro generates expected function names
 */
static int test_macro_generates_impl_function(void)
{
    printf("  Testing macro generates _impl function... ");
    
    // The macro should generate sample_resolver_impl
    // We can't call it directly as it's static, but we verify
    // the resolver works (which means _impl exists)
    
    void* ptr = sample_resolver();
    if (ptr == NULL) {
        printf("FAIL\n");
        return 1;
    }
    
    printf("OK (resolver works, so _impl exists)\n");
    return 0;
}

int main(void)
{
    int failures = 0;
    
    printf("Testing EXPLICIT_RUNTIME_RESOLVER macro:\n");
    printf("  Current SIMD level: %s\n", simd_level_name(detect_simd_level_ts()));
    printf("\n");
    
    failures += test_resolver_returns_valid_pointer();
    failures += test_resolver_matches_simd_level();
    failures += test_resolver_caching();
    failures += test_scalar_only_resolver();
    failures += test_macro_generates_impl_function();
    
    printf("\n");
    if (failures == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed!\n", failures);
        return 1;
    }
}
