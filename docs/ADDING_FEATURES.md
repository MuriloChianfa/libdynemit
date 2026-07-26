# Adding New Features to Dynemit

This guide explains how to add new SIMD-optimized features to the dynemit library.
The running example is the existing `max` feature and its `max_u16` variant
(`features/max/max_u16.c`). Copy that layout when you add your own feature.

## Overview

The dynemit library uses a modular architecture where each feature:
- Has its own directory under `features/`
- Provides multiple SIMD implementations (scalar, SSE2, SSE4.2, AVX, AVX2, AVX-512F, AVX-512 VBMI2, and on aarch64 NEON/SVE/SVE2)
- Uses the `ifunc` mechanism (GCC and Clang) for runtime dispatch
- Can be built as both an individual library and part of the all-in-one bundle

## Quick Start

To add a new feature (mirror `max` / `max_u16`):

1. Create `features/<name>/` with implementation, tests, and benchmarks
2. Create public header `include/dynemit/<name>.h`
3. Create `features/<name>/CMakeLists.txt`
4. Register the feature in `src/dynemit_features.c` and `cmake/ListFeatures.cmake`
5. Include the header from `include/dynemit.h`
6. Build Debug and run `ctest`

The root `CMakeLists.txt` auto-discovers every directory under `features/`.

## Detailed Steps

### 1. Create Feature Directory Structure

For `max` the tree looks like:

```bash
features/max/
  ├── max_u16.c             # u16 reduction (also max_f64.c, max_u32.c, …)
  ├── CMakeLists.txt
  ├── tests/
  │   └── test_max.c
  └── benchmarks/
      ├── bench_max_f64.c
      └── bench_max_u32.c
```

For a new feature:

```bash
mkdir -p features/<name>/tests features/<name>/benchmarks
```

### 2. Create Implementation File

Create one `.c` file per type variant. Example: `features/max/max_u16.c`.
Sketch of the x86 path (see the real file for aarch64 and full SIMD bodies):

```c
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/max.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
max_u16_scalar(const uint16_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint16_t result = data[0];
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++) {
        if (data[i] > result) {
            result = data[i];
        }
    }
    return (double)result;
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse4.2")))
static double
max_u16_sse42(const uint16_t *data, size_t n)
{
    /* horizontal max with _mm_max_epu16 + scalar tail; see max_u16.c */
    ...
}

/* Also: max_u16_sse2, max_u16_avx, max_u16_avx2, max_u16_avx512f */
#endif

max_u16_fn_t
max_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return max_u16_avx512f;
    case SIMD_AVX2:    return max_u16_avx2;
    case SIMD_AVX:     return max_u16_avx;
    case SIMD_SSE4_2:  return max_u16_sse42;
    case SIMD_SSE2:    return max_u16_sse2;
#endif
    case SIMD_SCALAR:
    default:           return max_u16_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(max_u16_resolver, max_u16_fn_t)
{
    return max_u16_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(max_u16_fn_t, max_u16, max_u16_resolver)

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#endif
double max_u16(const uint16_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("max_u16_resolver");
```

Prefer the macros in `<dynemit/compiler.h>` / `<dynemit/err.h>`
(`DYNEMIT_IFUNC_SETUP`, `EXPLICIT_RUNTIME_RESOLVER`, …) rather than hand-rolled
`ifunc` attributes. Full reference: `features/max/max_u16.c`.

### 3. Create Public Header

`include/dynemit/max.h` declares each type variant and its `_select` API:

```c
#ifndef DYNEMIT_MAX_H
#define DYNEMIT_MAX_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*max_u16_fn_t)(const uint16_t *, size_t);

double max_u16(const uint16_t *data, size_t n);

/** Return the implementation for a specific SIMD level (tests/benchmarks). */
max_u16_fn_t max_u16_select(simd_level_t level);

/* Likewise max_f64 / max_u32 / max_u64 in the real header */

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_MAX_H */
```

### 4. Create Feature CMakeLists.txt

`features/max/CMakeLists.txt`:

```cmake
# Max Feature
# SIMD-optimized maximum reductions

file(GLOB MAX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_library(max_obj OBJECT ${MAX_SOURCES})
target_include_directories(max_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(max_obj PUBLIC dynemit_core)
set_target_properties(max_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(dynemit_max STATIC $<TARGET_OBJECTS:max_obj>)
target_include_directories(dynemit_max PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dynemit_max PUBLIC dynemit_core)

include(GNUInstallDirs)
install(TARGETS dynemit_max ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/max.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)

# Tests / benchmarks gated inside cmake/FeatureExtras.cmake
# (Debug: on by default; Release: -DDYNEMIT_BUILD_TESTS=ON / -DDYNEMIT_BUILD_BENCHMARKS=ON)
dynemit_add_feature_test(max)
dynemit_add_feature_bench(max f64)
dynemit_add_feature_bench(max u32)
```

Helpers live in `cmake/FeatureExtras.cmake`. Optional flags include `FAULT_ALLOC`,
`PTHREAD`, `LIBS`, and `INCLUDES`; see `entropy` or `histogram` for examples.

### 5. Register the Feature (no root CMakeLists edit)

Features under `features/*/` are discovered automatically. You still need to
register the feature name in two places:

**a) Runtime registry.** Edit `src/dynemit_features.c`:

```c
const char **
dynemit_features(void)
{
    static const char *features[] = {
        "core",
        "add",
        /* ... */
        "max",          /* already present for the max feature */
        nullptr
    };
    return features;
}
```

**b) Configure-time list.** Add a line in `cmake/ListFeatures.cmake`
(`LIST_FEATURES=ON` output). `max` is already listed there.

### 6. Update Umbrella Header

Edit `include/dynemit.h` to include your feature header (headers are always
listed here; modular users can also `#include <dynemit/max.h>` directly):

```c
#include <dynemit/max.h>   /* already present for max */
```

### 7. Add Tests and Benchmarks

Tests live next to the feature. `features/max/tests/test_max.c` covers `max_u16`
with Unity, including `_select` across SIMD levels:

```c
void test_max_u16_basic(void)
{
    uint16_t d[] = {500, 100, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 500.0, max_u16(d, 3));
}

void test_max_u16_all_variants(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        max_u16_fn_t fn = max_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        /* exercise fn across sizes / tails */
    }
}
```

Benchmarks use `bench/bench_utils.h`. For `max`, see
`features/max/benchmarks/bench_max_f64.c` and `bench_max_u32.c`. A u16 bench
would be registered as `dynemit_add_feature_bench(max u16)` and live at
`features/max/benchmarks/bench_max_u16.c`.

The `dynemit_add_feature_test` / `dynemit_add_feature_bench` calls in step 4
register these targets. Do **not** add them to top-level `tests/CMakeLists.txt`
(that directory is for core-only tests).

### 8. Build and Test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Run the max suite or binary directly:

```bash
ctest --test-dir build -R test_max --output-on-failure
./build/features/max/test_max
```

Verify the library:

```bash
find build -name "libdynemit_max.a"
```

## Common SIMD Intrinsics

### Integer max (u16-oriented, as in `max_u16`)

| Operation | SSE4.2 | AVX2 | AVX-512F |
|-----------|--------|------|----------|
| Max epu16 | `_mm_max_epu16` | `_mm256_max_epu16` | `_mm512_max_epu16` |
| Load | `_mm_loadu_si128` | `_mm256_loadu_si256` | `_mm512_loadu_si512` |

### Float arithmetic (other features)

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

1. **Always provide a scalar fallback.** Mark it with `DYNEMIT_TARGET_DEFAULT` / `__attribute__((target("default")))`.

2. **Handle tail elements.** Process remaining elements that don't fit in SIMD registers (see `max_u16_sse42` / `max_u16_avx2`).

3. **Use unaligned loads/stores** unless you can guarantee alignment.

4. **Test correctness** via `_select` across all SIMD levels and awkward sizes (including tails).

5. **Disable auto-vectorization for scalar.** Use `DYNEMIT_NO_AUTOVECTORIZE` and `DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN` from `<dynemit/compiler.h>`.

6. **Use appropriate SIMD levels.** Lower levels may delegate (e.g. `max_u16_sse2` calls scalar; `max_u16_avx` reuses SSE4.2).

7. **Include necessary headers.** `<immintrin.h>` on x86; `<arm_neon.h>` / `<arm_sve.h>` on aarch64.

8. **Follow naming conventions** (from `max` / `max_u16`):
   - Feature directory / CMake: `max`, `max_obj`, `dynemit_max`
   - Function: `max_u16` (also `max_f64`, `max_u32`, …)
   - Select API: `max_u16_select`
   - Static implementations: `max_u16_scalar`, `max_u16_sse42`, `max_u16_avx2`, …
   - Resolver: `max_u16_resolver`
   - Test / bench: `tests/test_max.c`, `benchmarks/bench_max_<type>.c`

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
./build/tests/test_features
```

## Further Reading

- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [GCC documentation on ifunc](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [Clang Attributes Reference](https://clang.llvm.org/docs/AttributeReference.html)
- [Agner Fog's optimization manuals](https://www.agner.org/optimize/)
