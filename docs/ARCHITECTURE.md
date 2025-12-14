# Dynemit Architecture

This document explains the internal architecture of the dynemit library.

## Overview

Dynemit uses a **hybrid library architecture** that supports both:
1. **All-in-one mode**: Single library containing all features
2. **Modular mode**: Separate libraries for each feature

This is achieved through CMake's **object library** pattern, which allows compiling code once and linking it into multiple targets.

## Directory Structure

```
dynemit/
├── src/                     # Core library (CPU detection)
│   ├── dynemit.c           # CPU feature detection
│   └── dynemit_features.c  # Feature list (all-in-one only)
├── features/                # Individual SIMD features
│   ├── vector_add/
│   ├── vector_mul/
│   └── vector_sub/
├── include/dynemit/         # Public headers
└── tests/                   # Test suite
```

## Build System Architecture

### Object Libraries

Each feature is built as an **object library** (`*_obj` target):

```cmake
add_library(vector_add_obj OBJECT vector_add.c)
```

Object libraries compile source files but don't create an archive. The compiled objects can then be:
1. Bundled into the all-in-one library
2. Packaged into individual static libraries

### All-in-One Library

The `libdynemit.a` library combines all object libraries:

```cmake
add_library(dynemit STATIC
    $<TARGET_OBJECTS:dynemit_core_obj>
    $<TARGET_OBJECTS:vector_add_obj>
    $<TARGET_OBJECTS:vector_mul_obj>
    $<TARGET_OBJECTS:vector_sub_obj>
    src/dynemit_features.c
)
```

Benefits:
- Single library to link against
- All features included
- Runtime feature discovery via `dynemit_features()`

### Individual Libraries

Each feature also creates its own static library:

```cmake
add_library(dynemit_vector_add STATIC 
    $<TARGET_OBJECTS:vector_add_obj>
)
```

Benefits:
- Minimal binary size (only link what you need)
- Fine-grained control
- No unused code in final binary

### Core Library

The core library (`libdynemit_core.a`) contains only CPU detection:

```cmake
add_library(dynemit_core STATIC 
    $<TARGET_OBJECTS:dynemit_core_obj>
)
```

This is required by all feature libraries and applications.

## Runtime Dispatch Mechanism

### GCC ifunc Attribute

Each feature uses GCC's `ifunc` (indirect function) attribute:

```c
// Resolver runs once at program load
static vector_add_f32_func_t
vector_add_f32_resolver(void)
{
    simd_level_t level = detect_simd_level();
    
    switch (level) {
    case SIMD_AVX512F: return vector_add_f32_avx512f;
    case SIMD_AVX2:    return vector_add_f32_avx2;
    // ... etc
    }
}

// Public function uses ifunc for dispatch
void vector_add_f32(const float *a, const float *b, float *out, size_t n)
    __attribute__((ifunc("vector_add_f32_resolver")));
```

**How it works:**
1. At program load time, the dynamic linker calls `vector_add_f32_resolver()`
2. Resolver detects CPU features and returns optimal implementation pointer
3. All subsequent calls to `vector_add_f32()` go directly to the selected implementation
4. **Zero runtime overhead** after initial resolution

### CPU Detection

The `detect_simd_level()` function in `src/dynemit.c`:

```c
simd_level_t detect_simd_level(void)
{
    // 1. Check CPU vendor and max CPUID leaf
    cpuid_x86(0, 0, &eax, &ebx, &ecx, &edx);
    
    // 2. Check basic features (SSE2, SSE4.2, AVX)
    cpuid_x86(1, 0, &eax, &ebx, &ecx, &edx);
    
    // 3. Check OS support for YMM/ZMM registers
    if (osxsave)
        xcr0 = xgetbv_x86(0);
    
    // 4. Check extended features (AVX2, AVX-512F)
    cpuid_x86(7, 0, &eax7, &ebx7, &ecx7, &edx7);
    
    // 5. Return highest supported level
    return simd_level;
}
```

**Detection criteria:**
- **SSE2**: CPUID.01H:EDX[26]
- **SSE4.2**: CPUID.01H:ECX[20]
- **AVX**: CPUID.01H:ECX[28] + XCR0[2:1] (OS support)
- **AVX2**: CPUID.07H:EBX[5] + XCR0[2:1]
- **AVX-512F**: CPUID.07H:EBX[16] + XCR0[7:5] (ZMM state)

## Feature Registry

### Weak Symbols

The `dynemit_features()` function uses weak symbols to provide different implementations:

**Default (in dynemit.c):**
```c
__attribute__((weak))
const char **
dynemit_features(void)
{
    static const char *features[] = { "core", NULL };
    return features;
}
```

**All-in-one override (in dynemit_features.c):**
```c
const char **
dynemit_features(void)
{
    static const char *features[] = {
        "core",
        "vector_add",
        "vector_mul",
        "vector_sub",
        NULL
    };
    return features;
}
```

When linking:
- `libdynemit.a`: Uses the all-in-one version (includes dynemit_features.c)
- `libdynemit_core.a`: Uses the weak default version

## Header Organization

### Umbrella Header

`include/dynemit.h` conditionally includes feature headers:

```c
#include <dynemit/core.h>

#ifdef DYNEMIT_ALL_FEATURES
#include <dynemit/vector_add.h>
#include <dynemit/vector_mul.h>
#include <dynemit/vector_sub.h>
#endif
```

The all-in-one library defines `DYNEMIT_ALL_FEATURES`:

```cmake
target_compile_definitions(dynemit PUBLIC DYNEMIT_ALL_FEATURES)
```

### Feature Headers

Each feature has a minimal header in `include/dynemit/`:

```c
#ifndef DYNEMIT_VECTOR_ADD_H
#define DYNEMIT_VECTOR_ADD_H

#include <stddef.h>

void vector_add_f32(const float *a, const float *b, float *out, size_t n);

#endif
```

**Design principles:**
- Self-contained (only depends on standard headers and core.h)
- No implementation details leaked
- Can be included independently

## SIMD Implementation Pattern

### Standard Structure

Every SIMD feature follows this pattern:

```c
// 1. Include intrinsics
#include <immintrin.h>
#include <stddef.h>
#include <dynemit/core.h>

// 2. Scalar fallback
__attribute__((target("default")))
__attribute__((optimize("no-tree-vectorize")))
static void feature_scalar(...) { /* ... */ }

// 3. SIMD implementations (SSE2, SSE4.2, AVX, AVX2, AVX-512F)
__attribute__((target("sse2")))
static void feature_sse2(...) { /* ... */ }

// ... more SIMD levels ...

// 4. Resolver function
typedef void (*feature_func_t)(...);

static feature_func_t
feature_resolver(void)
{
    simd_level_t level = detect_simd_level();
    switch (level) { /* ... */ }
}

// 5. Public ifunc
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
void feature(...) __attribute__((ifunc("feature_resolver")));
```

### Why This Pattern?

1. **Scalar fallback**: Ensures portability to non-x86 platforms
2. **Static implementations**: Prevents symbol conflicts
3. **Target attributes**: Enables specific CPU instructions
4. **Resolver pattern**: Centralizes dispatch logic
5. **ifunc attribute**: Achieves zero-overhead dispatch

## Testing Strategy

### Unit Tests

Each feature should have a correctness test:

```c
// Verify SIMD result matches expected scalar computation
for (int i = 0; i < N; i++) {
    if (fabsf(result[i] - expected[i]) > epsilon) {
        return FAIL;
    }
}
```

### Feature Discovery Test

`tests/test_features.c` verifies:
- Runtime feature enumeration works
- All expected features are present
- SIMD level detection succeeds

### Integration Tests

Benchmarks serve as integration tests:
- Verify end-to-end functionality
- Measure performance
- Detect regressions

## Performance Considerations

### Cache Efficiency

- Use unaligned loads (`_mm_loadu_ps`) to avoid alignment overhead
- Process data sequentially to maximize cache hits
- Consider prefetching for large datasets

### Loop Structure

```c
size_t i = 0;
const size_t step = 16;  // SIMD width

// Main SIMD loop
for (; i + step <= n; i += step) {
    // SIMD operations
}

// Scalar tail for remaining elements
for (; i < n; i++) {
    // Scalar operation
}
```

### Compiler Optimization

- Build with `-O3` for maximum performance
- Use `__restrict__` for pointer arguments if applicable
- Let compiler auto-vectorize scalar tail

## Portability

### Non-x86 Platforms

On non-x86 systems:
- CPUID returns 0 for all features
- `detect_simd_level()` returns `SIMD_SCALAR`
- Resolver always selects scalar implementation
- No SIMD intrinsics are used

### Compiler Requirements

**Required:**
- GCC with `ifunc` support
- x86-64 target for SIMD optimizations

**Not supported:**
- Clang (no `ifunc` support)
- MSVC (different intrinsics model)

## Future Enhancements

### Potential Improvements

1. **ARM NEON support**: Add ARM-specific implementations
2. **Multi-architecture**: Support both x86 and ARM in same library
3. **AVX-512 subsets**: Use AVX-512BW, AVX-512DQ, etc.
4. **Runtime benchmarking**: Choose implementation based on actual performance
5. **Compile-time options**: Allow disabling specific SIMD levels

### Scalability

The current architecture scales well up to ~10-20 features. Beyond that, consider:
- Feature categorization (`features/math/`, `features/crypto/`, etc.)
- Automated CMake feature discovery
- Plugin system for dynamic feature loading

## References

- [GCC Function Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [CMake Object Libraries](https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries)
- [CPUID Specification](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html)
