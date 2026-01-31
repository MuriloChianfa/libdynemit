/**
 * C++ IFUNC Resolver Macro Test
 * 
 * Tests that the EXPLICIT_RUNTIME_RESOLVER macro works correctly
 * when used in C++ code. This ensures C++ projects can create
 * their own IFUNC resolvers using the library's helpers.
 */

#include <dynemit/core.h>
#include <dynemit/err.h>
#include <iostream>
#include <cmath>

// Define different implementations
static void test_func_scalar(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i] + 1.0f;  // Slightly different than vector_add
    }
}

static void test_func_sse2(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i] + 2.0f;  // Different constant to verify dispatch
    }
}

static void test_func_avx(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i] + 3.0f;
    }
}

static void test_func_avx2(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i] + 4.0f;
    }
}

static void test_func_avx512(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i] + 5.0f;
    }
}

// Function pointer type
typedef void (*test_func_t)(float*, const float*, const float*, size_t);

// Use EXPLICIT_RUNTIME_RESOLVER macro in C++
// Note: The resolver and implementation need C linkage for IFUNC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

extern "C" {
EXPLICIT_RUNTIME_RESOLVER(test_func_resolver)
{
    simd_level_t level = detect_simd_level_ts();
    
    switch (level) {
    case SIMD_AVX512F:
        return reinterpret_cast<void*>(test_func_avx512);
    case SIMD_AVX2:
        return reinterpret_cast<void*>(test_func_avx2);
    case SIMD_AVX:
        return reinterpret_cast<void*>(test_func_avx);
    case SIMD_SSE4_2:
    case SIMD_SSE2:
        return reinterpret_cast<void*>(test_func_sse2);
    default:
        return reinterpret_cast<void*>(test_func_scalar);
    }
}

// Declare the IFUNC-dispatched function
void test_func_cpp(float* out, const float* a, const float* b, size_t n)
    __attribute__((ifunc("test_func_resolver")));
}

#pragma GCC diagnostic pop

int main()
{
    std::cout << "C++ IFUNC Resolver Macro Test" << std::endl;
    std::cout << "==============================" << std::endl << std::endl;
    
    // Detect SIMD level
    simd_level_t level = detect_simd_level_ts();
    const char* level_name = simd_level_name(level);
    
    std::cout << "Detected SIMD level: " << level_name << std::endl;
    std::cout << "Testing EXPLICIT_RUNTIME_RESOLVER macro in C++..." << std::endl << std::endl;
    
    // Test the dispatched function
    constexpr size_t N = 8;
    float a[N], b[N], result[N];
    
    for (size_t i = 0; i < N; i++) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i + 1);
    }
    
    // Call the IFUNC-dispatched function
    test_func_cpp(result, a, b, N);
    
    std::cout << "Function executed successfully!" << std::endl;
    std::cout << "Sample results:" << std::endl;
    std::cout << "  a[0] = " << a[0] << ", b[0] = " << b[0] 
              << " -> result[0] = " << result[0] << std::endl;
    std::cout << "  a[1] = " << a[1] << ", b[1] = " << b[1] 
              << " -> result[1] = " << result[1] << std::endl;
    
    // Verify the correct implementation was dispatched
    // Each implementation adds a different constant
    float expected_constant = 0.0f;
    switch (level) {
    case SIMD_AVX512F:
        expected_constant = 5.0f;
        break;
    case SIMD_AVX2:
        expected_constant = 4.0f;
        break;
    case SIMD_AVX:
        expected_constant = 3.0f;
        break;
    case SIMD_SSE4_2:
    case SIMD_SSE2:
        expected_constant = 2.0f;
        break;
    default:
        expected_constant = 1.0f;
        break;
    }
    
    bool test_ok = true;
    for (size_t i = 0; i < N; i++) {
        float expected = a[i] + b[i] + expected_constant;
        if (std::fabs(result[i] - expected) > 1e-6f) {
            test_ok = false;
            std::cerr << "  Error at index " << i << ": expected " 
                      << expected << ", got " << result[i] << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Dispatch verification: " << (test_ok ? "OK" : "FAILED") << std::endl;
    std::cout << "Expected constant offset: " << expected_constant << std::endl;
    
    if (test_ok) {
        std::cout << std::endl << "C++ IFUNC resolver macro test PASSED!" << std::endl;
        return 0;
    } else {
        std::cerr << std::endl << "C++ IFUNC resolver macro test FAILED!" << std::endl;
        return 1;
    }
}
