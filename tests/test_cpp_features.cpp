/**
 * C++ Features Detection Test (GTest)
 *
 * Tests that feature detection APIs work correctly from C++ code
 * and that all SIMD level enums are accessible.
 */

#include <gtest/gtest.h>
#include <dynemit.h>

TEST(CppFeatures, SimdLevelNames) {
    struct {
        simd_level_t level;
        const char* expected;
    } cases[] = {
        {SIMD_SCALAR,       "Scalar"},
        {SIMD_SSE2,         "SSE2"},
        {SIMD_SSE4_2,       "SSE4.2"},
        {SIMD_AVX,          "AVX"},
        {SIMD_AVX2,         "AVX2"},
        {SIMD_AVX512F,      "AVX-512F"},
        {SIMD_AVX512_VBMI2, "AVX-512VBMI2"},
    };

    for (const auto& c : cases) {
        EXPECT_STREQ(simd_level_name(c.level), c.expected)
            << "Mismatch for enum value " << static_cast<int>(c.level);
    }
}

TEST(CppFeatures, RuntimeDetection) {
    simd_level_t detected    = detect_simd_level();
    simd_level_t detected_ts = detect_simd_level_ts();

    EXPECT_EQ(detected, detected_ts)
        << "detect_simd_level() and detect_simd_level_ts() disagree";

    EXPECT_GE(static_cast<int>(detected), static_cast<int>(SIMD_SCALAR));
    EXPECT_LE(static_cast<int>(detected), static_cast<int>(SIMD_AVX512_VBMI2));
}

TEST(CppFeatures, FeatureList) {
#ifdef DYNEMIT_ALL_FEATURES
    const char** features = dynemit_features();
    ASSERT_NE(features, nullptr);

    int count = 0;
    for (int i = 0; features[i] != nullptr; i++) {
        count++;
    }
    EXPECT_GT(count, 0) << "Feature list should not be empty";
#else
    GTEST_SKIP() << "DYNEMIT_ALL_FEATURES not defined";
#endif
}
