/**
 * C++ Basic Compatibility Test
 * 
 * Tests that the library headers can be included from C++ and that
 * basic vector operations work correctly from C++ code.
 */

#include <dynemit.h>
#include <iostream>
#include <cmath>
#include <cstdio>

#define N 16

int main()
{
    std::cout << "C++ Basic Compatibility Test" << std::endl;
    std::cout << "=============================" << std::endl << std::endl;
    
    // Test SIMD detection from C++
    simd_level_t level = detect_simd_level();
    const char* level_name = simd_level_name(level);
    
    std::cout << "Detected SIMD level: " << level_name 
              << " (enum value: " << static_cast<int>(level) << ")" << std::endl << std::endl;
    
    // Test data
    float a[N], b[N], result[N];
    
    // Initialize test vectors
    for (int i = 0; i < N; i++) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i + 1);
    }
    
    // Test vector_add from C++
    vector_add_f32(a, b, result, N);
    std::cout << "vector_add_f32 test:" << std::endl;
    std::cout << "  a[0..3] = [" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << "]" << std::endl;
    std::cout << "  b[0..3] = [" << b[0] << ", " << b[1] << ", " << b[2] << ", " << b[3] << "]" << std::endl;
    std::cout << "  result  = [" << result[0] << ", " << result[1] << ", " << result[2] << ", " << result[3] << "]" << std::endl;
    
    // Verify correctness
    bool add_ok = true;
    for (int i = 0; i < N; i++) {
        if (std::fabs(result[i] - (a[i] + b[i])) > 1e-6f) {
            add_ok = false;
            break;
        }
    }
    std::cout << "  Status: " << (add_ok ? "OK" : "FAILED") << std::endl << std::endl;
    
    // Test vector_mul from C++
    vector_mul_f32(a, b, result, N);
    std::cout << "vector_mul_f32 test:" << std::endl;
    std::cout << "  a[0..3] = [" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << "]" << std::endl;
    std::cout << "  b[0..3] = [" << b[0] << ", " << b[1] << ", " << b[2] << ", " << b[3] << "]" << std::endl;
    std::cout << "  result  = [" << result[0] << ", " << result[1] << ", " << result[2] << ", " << result[3] << "]" << std::endl;
    
    // Verify correctness
    bool mul_ok = true;
    for (int i = 0; i < N; i++) {
        if (std::fabs(result[i] - (a[i] * b[i])) > 1e-6f) {
            mul_ok = false;
            break;
        }
    }
    std::cout << "  Status: " << (mul_ok ? "OK" : "FAILED") << std::endl << std::endl;
    
    // Test vector_sub from C++
    vector_sub_f32(a, b, result, N);
    std::cout << "vector_sub_f32 test:" << std::endl;
    std::cout << "  a[0..3] = [" << a[0] << ", " << a[1] << ", " << a[2] << ", " << a[3] << "]" << std::endl;
    std::cout << "  b[0..3] = [" << b[0] << ", " << b[1] << ", " << b[2] << ", " << b[3] << "]" << std::endl;
    std::cout << "  result  = [" << result[0] << ", " << result[1] << ", " << result[2] << ", " << result[3] << "]" << std::endl;
    
    // Verify correctness
    bool sub_ok = true;
    for (int i = 0; i < N; i++) {
        if (std::fabs(result[i] - (a[i] - b[i])) > 1e-6f) {
            sub_ok = false;
            break;
        }
    }
    std::cout << "  Status: " << (sub_ok ? "OK" : "FAILED") << std::endl << std::endl;
    
    // Final verdict
    if (add_ok && mul_ok && sub_ok) {
        std::cout << "All C++ compatibility tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cerr << "Some C++ compatibility tests FAILED!" << std::endl;
        return 1;
    }
}
