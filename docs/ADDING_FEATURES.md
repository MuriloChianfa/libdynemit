# Adding New Features to Dynemit

This guide explains how to add new SIMD-optimized features to the dynemit library.

## Overview

The dynemit library uses a modular architecture where each feature:
- Has its own directory under `features/`
- Provides multiple SIMD implementations (scalar, SSE2, SSE4.2, AVX, AVX2, AVX-512F)
- Uses GCC's `ifunc` mechanism for runtime dispatch
- Can be built as both an individual library and part of the all-in-one bundle

## Quick Start

To add a new feature called `my_feature`:

1. Create feature directory and implementation
2. Create public header file
3. Create CMakeLists.txt for the feature
4. Update main build configuration
5. Update feature registry
6. Add tests
7. Update documentation

## Detailed Steps

### 1. Create Feature Directory Structure

```bash
mkdir -p features/my_feature
```

### 2. Create Implementation File

Create `features/my_feature/my_feature.c`:

```c
#include <immintrin.h>
#include <stddef.h>
#include <dynemit/core.h>

// Scalar version - disable auto-vectorization
__attribute__((target("default")))
__attribute__((optimize("no-tree-vectorize")))
static void
my_feature_f32_scalar(const float *a, const float *b, float *out, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = /* your operation here */;
}

__attribute__((target("sse2")))
static void
my_feature_f32_sse2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;  // SSE processes 4 floats
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = /* SSE operation */;
        _mm_storeu_ps(out + i, vc);
    }
    // Scalar tail for remaining elements
    for (; i < n; i++)
        out[i] = /* your operation here */;
}

__attribute__((target("sse4.2")))
static void
my_feature_f32_sse42(const float *a, const float *b, float *out, size_t n)
{
    // Same as SSE2 for basic operations, or use SSE4.2-specific intrinsics
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = /* SSE4.2 operation */;
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = /* your operation here */;
}

__attribute__((target("avx")))
static void
my_feature_f32_avx(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;  // AVX processes 8 floats
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = /* AVX operation */;
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = /* your operation here */;
}

__attribute__((target("avx2")))
static void
my_feature_f32_avx2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;  // AVX2 processes 8 floats
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = /* AVX2 operation */;
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = /* your operation here */;
}

__attribute__((target("avx512f")))
static void
my_feature_f32_avx512f(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 16;  // AVX-512F processes 16 floats
    for (; i + step <= n; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 vc = /* AVX-512F operation */;
        _mm512_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = /* your operation here */;
}

// Resolver function for ifunc
typedef void (*my_feature_f32_func_t)(const float *, const float *, float *, size_t);

static my_feature_f32_func_t
my_feature_f32_resolver(void)
{
    simd_level_t level = detect_simd_level();
    
    switch (level) {
    case SIMD_AVX512F: return my_feature_f32_avx512f;
    case SIMD_AVX2:    return my_feature_f32_avx2;
    case SIMD_AVX:     return my_feature_f32_avx;
    case SIMD_SSE4_2:  return my_feature_f32_sse42;
    case SIMD_SSE2:    return my_feature_f32_sse2;
    case SIMD_SCALAR:
    default:           return my_feature_f32_scalar;
    }
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
void my_feature_f32(const float *a, const float *b, float *out, size_t n)
    __attribute__((ifunc("my_feature_f32_resolver")));
```

### 3. Create Public Header

Create `include/dynemit/my_feature.h`:

```c
#ifndef DYNEMIT_MY_FEATURE_H
#define DYNEMIT_MY_FEATURE_H

#include <stddef.h>

/**
 * Brief description of what this feature does.
 * 
 * @param a First input vector
 * @param b Second input vector
 * @param out Output vector
 * @param n Number of elements to process
 */
void my_feature_f32(const float *a, const float *b, float *out, size_t n);

#endif // DYNEMIT_MY_FEATURE_H
```

### 4. Create Feature CMakeLists.txt

Create `features/my_feature/CMakeLists.txt`:

```cmake
# My Feature
# SIMD-optimized <description>

# Object library for bundling into all-in-one library
add_library(my_feature_obj OBJECT 
    my_feature.c
)

target_include_directories(my_feature_obj 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(my_feature_obj PUBLIC dynemit_core)

# Individual static library
add_library(dynemit_my_feature STATIC 
    $<TARGET_OBJECTS:my_feature_obj>
)

target_include_directories(dynemit_my_feature 
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dynemit_my_feature PUBLIC dynemit_core)

# Installation
include(GNUInstallDirs)

install(TARGETS dynemit_my_feature
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/my_feature.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit
)
```

### 5. Update Main CMakeLists.txt

Edit `CMakeLists.txt` to add your feature:

**a) Add subdirectory:**
```cmake
# Add subdirectories for core and features
add_subdirectory(src)
add_subdirectory(features/vector_add)
add_subdirectory(features/vector_mul)
add_subdirectory(features/vector_sub)
add_subdirectory(features/my_feature)  # <-- Add this
add_subdirectory(bench)
```

**b) Add object library to all-in-one library:**
```cmake
# Create all-in-one library combining core + all features
add_library(dynemit STATIC
    $<TARGET_OBJECTS:dynemit_core_obj>
    $<TARGET_OBJECTS:vector_add_obj>
    $<TARGET_OBJECTS:vector_mul_obj>
    $<TARGET_OBJECTS:vector_sub_obj>
    $<TARGET_OBJECTS:my_feature_obj>  # <-- Add this
    src/dynemit_features.c
)
```

**c) Update LIST_FEATURES option:**
```cmake
if(LIST_FEATURES)
    message(STATUS "")
    message(STATUS "=== Available Dynemit Features ===")
    message(STATUS "  - core              (CPU detection and SIMD level API)")
    message(STATUS "  - vector_add        (SIMD-optimized vector addition)")
    message(STATUS "  - vector_mul   (SIMD-optimized vector multiplication)")
    message(STATUS "  - vector_sub        (SIMD-optimized vector subtraction)")
    message(STATUS "  - my_feature        (SIMD-optimized <description>)")  # <-- Add this
    # ... rest of the message
```

### 6. Update Feature Registry

Edit `src/dynemit_features.c` to include your feature in the all-in-one build:

```c
const char **
dynemit_features(void)
{
    static const char *features[] = {
        "core",
        "my_feature",         // <-- Add this (alphabetically)
        "vector_add",
        "vector_mul",
        "vector_sub",
        NULL  // NULL-terminated
    };
    return features;
}
```

### 7. Update Umbrella Header

Edit `include/dynemit.h` to include your feature header:

```c
// Features - automatically included when using the all-in-one library
#ifdef DYNEMIT_ALL_FEATURES
#include <dynemit/my_feature.h>      // <-- Add this
#include <dynemit/vector_add.h>
#include <dynemit/vector_mul.h>
#include <dynemit/vector_sub.h>
#endif

// Alternatively, users can include individual feature headers:
// #include <dynemit/my_feature.h>    // <-- Add this
// #include <dynemit/vector_add.h>
// etc.
```

### 8. Add Tests

Create a test in `tests/` to verify correctness:

```c
#include <stdio.h>
#include <math.h>
#include <dynemit/core.h>
#include <dynemit/my_feature.h>

int main(void) {
    float a[16], b[16], result[16];
    
    // Initialize test data
    for (int i = 0; i < 16; i++) {
        a[i] = (float)i;
        b[i] = (float)(i + 1);
    }
    
    // Run your feature
    my_feature_f32(a, b, result, 16);
    
    // Verify correctness
    for (int i = 0; i < 16; i++) {
        float expected = /* compute expected value */;
        if (fabsf(result[i] - expected) > 1e-6f) {
            printf("FAILED at index %d\n", i);
            return 1;
        }
    }
    
    printf("PASSED\n");
    return 0;
}
```

Add the test to `tests/CMakeLists.txt`:

```cmake
add_executable(test_my_feature test_my_feature.c)
target_include_directories(test_my_feature PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_my_feature PRIVATE dynemit_core dynemit_my_feature m)
add_test(NAME test_my_feature COMMAND test_my_feature)
```

### 9. Build and Test

```bash
cd build
cmake ..
make
ctest
```

Verify your library was built:
```bash
find . -name "libdynemit_my_feature.a"
```

## Common SIMD Intrinsics

### Arithmetic Operations

| Operation | SSE/SSE4.2 | AVX/AVX2 | AVX-512F |
|-----------|------------|----------|----------|
| Add | `_mm_add_ps` | `_mm256_add_ps` | `_mm512_add_ps` |
| Subtract | `_mm_sub_ps` | `_mm256_sub_ps` | `_mm512_sub_ps` |
| Multiply | `_mm_mul_ps` | `_mm256_mul_ps` | `_mm512_mul_ps` |
| Divide | `_mm_div_ps` | `_mm256_div_ps` | `_mm512_div_ps` |
| FMA | `_mm_fmadd_ps` | `_mm256_fmadd_ps` | `_mm512_fmadd_ps` |

### Memory Operations

| Operation | SSE/SSE4.2 | AVX/AVX2 | AVX-512F |
|-----------|------------|----------|----------|
| Load (unaligned) | `_mm_loadu_ps` | `_mm256_loadu_ps` | `_mm512_loadu_ps` |
| Store (unaligned) | `_mm_storeu_ps` | `_mm256_storeu_ps` | `_mm512_storeu_ps` |
| Load (aligned) | `_mm_load_ps` | `_mm256_load_ps` | `_mm512_load_ps` |
| Store (aligned) | `_mm_store_ps` | `_mm256_store_ps` | `_mm512_store_ps` |

### Other Operations

| Operation | SSE/SSE4.2 | AVX/AVX2 | AVX-512F |
|-----------|------------|----------|----------|
| Min | `_mm_min_ps` | `_mm256_min_ps` | `_mm512_min_ps` |
| Max | `_mm_max_ps` | `_mm256_max_ps` | `_mm512_max_ps` |
| Sqrt | `_mm_sqrt_ps` | `_mm256_sqrt_ps` | `_mm512_sqrt_ps` |
| And | `_mm_and_ps` | `_mm256_and_ps` | `_mm512_and_ps` |
| Or | `_mm_or_ps` | `_mm256_or_ps` | `_mm512_or_ps` |

## Best Practices

1. **Always provide a scalar fallback** - Mark it with `__attribute__((target("default")))`

2. **Handle tail elements** - Process remaining elements that don't fit in SIMD registers

3. **Use unaligned loads/stores** - Unless you can guarantee alignment, use `_loadu_ps` and `_storeu_ps`

4. **Test correctness** - Verify your SIMD implementation matches scalar behavior

5. **Disable auto-vectorization for scalar** - Use `__attribute__((optimize("no-tree-vectorize")))` on scalar version

6. **Use appropriate SIMD levels** - AVX2 may not provide benefits over AVX for all operations

7. **Include necessary headers** - `<immintrin.h>` provides all x86 intrinsics

8. **Follow naming conventions**:
   - Feature name: `my_feature`
   - Function: `my_feature_f32` (or `_f64` for double precision)
   - Static implementations: `my_feature_f32_sse2`, `my_feature_f32_avx`, etc.
   - Resolver: `my_feature_f32_resolver`

## Example: Existing Features

Study these existing features as reference:
- `features/vector_add/` - Simple element-wise addition
- `features/vector_mul/` - Element-wise multiplication
- `features/vector_sub/` - Element-wise subtraction

## Troubleshooting

### Build Errors

- **Multiple definition errors**: Make sure static functions are marked `static`
- **Implicit declaration**: Include `<dynemit/core.h>` for `simd_level_t`
- **Undefined symbols**: Check that all SIMD variants are defined

### Runtime Issues

- **Wrong results**: Verify tail handling for non-multiple-of-step sizes
- **Crashes**: Check for alignment issues or out-of-bounds access
- **Performance**: Profile to ensure SIMD version is actually faster

### Testing

Run the verification script to check SIMD instructions:
```bash
./scripts/check_for_simd.sh
```

Verify feature is registered:
```bash
./tests/test_features
```

## Further Reading

- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [GCC documentation on ifunc](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [Agner Fog's optimization manuals](https://www.agner.org/optimize/)
