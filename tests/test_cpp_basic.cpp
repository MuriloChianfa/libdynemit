/**
 * C++ Basic Compatibility Test (GTest)
 *
 * Tests that the library headers can be included from C++ and that
 * basic vector operations work correctly from C++ code.
 */

#include <gtest/gtest.h>
#include <dynemit.h>

static constexpr size_t N = 16;

class VectorOpsTest : public ::testing::Test {
protected:
    float a[N], b[N], result[N];

    void SetUp() override {
        for (size_t i = 0; i < N; i++) {
            a[i] = static_cast<float>(i);
            b[i] = static_cast<float>(i + 1);
        }
    }
};

TEST_F(VectorOpsTest, AddF32) {
    add_f32(a, b, result, N);
    for (size_t i = 0; i < N; i++) {
        EXPECT_NEAR(result[i], a[i] + b[i], 1e-6f)
            << "Mismatch at index " << i;
    }
}

TEST_F(VectorOpsTest, MulF32) {
    mul_f32(a, b, result, N);
    for (size_t i = 0; i < N; i++) {
        EXPECT_NEAR(result[i], a[i] * b[i], 1e-6f)
            << "Mismatch at index " << i;
    }
}

TEST_F(VectorOpsTest, SubF32) {
    sub_f32(a, b, result, N);
    for (size_t i = 0; i < N; i++) {
        EXPECT_NEAR(result[i], a[i] - b[i], 1e-6f)
            << "Mismatch at index " << i;
    }
}
