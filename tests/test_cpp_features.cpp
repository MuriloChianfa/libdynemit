/**
 * C++ Features Detection Test
 * 
 * Tests that feature detection APIs work correctly from C++ code
 * and that all SIMD level enums are accessible.
 */

#include <dynemit.h>
#include <iostream>
#include <string>

int main()
{
    std::cout << "C++ Features Detection Test" << std::endl;
    std::cout << "===========================" << std::endl << std::endl;
    
    // Test all SIMD level enums are accessible from C++
    std::cout << "Testing SIMD level enums:" << std::endl;
    
    simd_level_t levels[] = {
        SIMD_SCALAR,
        SIMD_SSE2,
        SIMD_SSE4_2,
        SIMD_AVX,
        SIMD_AVX2,
        SIMD_AVX512F
    };
    
    const char* expected_names[] = {
        "Scalar",
        "SSE2",
        "SSE4.2",
        "AVX",
        "AVX2",
        "AVX-512F"
    };
    
    bool enum_test_ok = true;
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        const char* name = simd_level_name(levels[i]);
        std::cout << "  " << expected_names[i] << " -> " << name;
        
        if (std::string(name) == std::string(expected_names[i])) {
            std::cout << " [OK]" << std::endl;
        } else {
            std::cout << " [FAILED - expected " << expected_names[i] << "]" << std::endl;
            enum_test_ok = false;
        }
    }
    std::cout << std::endl;
    
    // Test runtime SIMD detection
    std::cout << "Testing runtime SIMD detection:" << std::endl;
    
    simd_level_t detected = detect_simd_level();
    const char* detected_name = simd_level_name(detected);
    std::cout << "  detect_simd_level(): " << detected_name << std::endl;
    
    simd_level_t detected_ts = detect_simd_level_ts();
    const char* detected_ts_name = simd_level_name(detected_ts);
    std::cout << "  detect_simd_level_ts(): " << detected_ts_name << std::endl;
    
    bool detection_ok = (detected == detected_ts);
    std::cout << "  Both detection methods agree: " 
              << (detection_ok ? "OK" : "FAILED") << std::endl << std::endl;
    
    // Test feature list function
    std::cout << "Testing feature list function:" << std::endl;
    
#ifdef DYNEMIT_ALL_FEATURES
    const char** features = dynemit_features();
    if (features != nullptr) {
        std::cout << "  Available features:" << std::endl;
        for (int i = 0; features[i] != nullptr; i++) {
            std::cout << "    - " << features[i] << std::endl;
        }
        std::cout << "  Feature list: OK" << std::endl;
    } else {
        std::cout << "  Feature list: FAILED (returned nullptr)" << std::endl;
        enum_test_ok = false;
    }
#else
    std::cout << "  Feature list not available (DYNEMIT_ALL_FEATURES not defined)" << std::endl;
#endif
    std::cout << std::endl;
    
    // Final verdict
    if (enum_test_ok && detection_ok) {
        std::cout << "All C++ feature detection tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cerr << "Some C++ feature detection tests FAILED!" << std::endl;
        return 1;
    }
}
