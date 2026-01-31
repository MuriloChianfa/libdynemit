# IFUNC Resolvers and Dynamic Loading

This document explains how to use libdynemit's IFUNC resolver utilities safely, especially when your library may be loaded via `dlopen()` (e.g., Python extensions, plugin systems).

## The Problem

GNU IFUNC (Indirect Functions) enables runtime function dispatch based on CPU features. This is how libdynemit automatically selects optimal SIMD implementations. However, IFUNC has a subtle issue with `dlopen()`:

**IFUNC resolvers run during ELF relocation**, which happens during `dlopen()`. In some contexts (especially Python's dynamic module loading with `RTLD_LOCAL`), resolvers may execute before the library is fully initialized, leading to:

- Segmentation faults
- Undefined behavior
- Crashes that are hard to debug

## The Solution

libdynemit provides two utilities to solve this problem:

### 1. `detect_simd_level_ts()` - Thread-Safe Detection

```c
#include <dynemit/core.h>

simd_level_t level = detect_simd_level_ts();
```

This function:
- Caches the SIMD detection result using atomic operations
- Is safe to call from multiple threads concurrently
- Avoids repeated CPUID calls
- Ensures consistent results across all calls

**When to use**: Always use `detect_simd_level_ts()` inside IFUNC resolvers instead of `detect_simd_level()`.

### 2. `EXPLICIT_RUNTIME_RESOLVER()` - Safe Resolver Wrapper

```c
#include <dynemit/err.h>

EXPLICIT_RUNTIME_RESOLVER(my_function_resolver)
{
    simd_level_t level = detect_simd_level_ts();
    
    switch (level) {
    case SIMD_AVX512F:
        return (void*)my_function_avx512;
    case SIMD_AVX2:
        return (void*)my_function_avx2;
    default:
        return (void*)my_function_scalar;
    }
}
```

This macro:
- Wraps resolver functions with a NULL-check
- Uses `__builtin_trap()` for immediate, debuggable failures
- Ensures resolvers never return NULL (which causes segfaults later)
- Generates both the wrapper and implementation functions

## Complete Example

Here's a complete example of a SIMD-dispatched function using safe resolvers:

```c
#include <dynemit/core.h>
#include <dynemit/err.h>
#include <stddef.h>

// Implementation variants
__attribute__((target("avx2")))
static void process_data_avx2(float *data, size_t n) {
    // AVX2 implementation
}

__attribute__((target("sse2")))
static void process_data_sse2(float *data, size_t n) {
    // SSE2 implementation
}

static void process_data_scalar(float *data, size_t n) {
    // Scalar fallback
}

// Function pointer type
typedef void (*process_data_func_t)(float *, size_t);

// Safe resolver using EXPLICIT_RUNTIME_RESOLVER
EXPLICIT_RUNTIME_RESOLVER(process_data_resolver)
{
    simd_level_t level = detect_simd_level_ts();
    
    switch (level) {
    case SIMD_AVX512F:
    case SIMD_AVX2:
        return (void*)process_data_avx2;
    case SIMD_SSE4_2:
    case SIMD_SSE2:
        return (void*)process_data_sse2;
    case SIMD_AVX:
    case SIMD_SCALAR:
    default:
        return (void*)process_data_scalar;
    }
}

// Public function using ifunc for runtime dispatch
void process_data(float *data, size_t n)
    __attribute__((ifunc("process_data_resolver")));
```

## Compiler Warnings

The `EXPLICIT_RUNTIME_RESOLVER` macro converts function pointers to `void*`, which generates `-Wpedantic` warnings. These warnings are expected and safe for IFUNC resolvers. You can suppress them:

```c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

EXPLICIT_RUNTIME_RESOLVER(my_resolver)
{
    // ... resolver logic
}

#pragma GCC diagnostic pop
```

Or project-wide in CMake:

```cmake
target_compile_options(my_target PRIVATE -Wno-pedantic)
```

## Best Practices

1. **Always use `detect_simd_level_ts()`** inside IFUNC resolvers
2. **Always use `EXPLICIT_RUNTIME_RESOLVER`** instead of writing resolvers directly
3. **Always have a default/fallback case** in your switch statement
4. **Test with different SIMD levels** using environment overrides if possible
5. **Document the `LD_PRELOAD` workaround** for users of your library

## Troubleshooting

### Segmentation fault on module load

**Symptom**: Crash when Python imports your extension module.

**Solution**: 
1. Ensure you're using `detect_simd_level_ts()` and `EXPLICIT_RUNTIME_RESOLVER`
2. Try `LD_PRELOAD` workaround
3. Check that all switch cases return valid function pointers

### Inconsistent SIMD level detection

**Symptom**: Different code paths being taken unexpectedly.

**Solution**: Use `detect_simd_level_ts()` which caches the result, ensuring consistency.

### Resolver returns NULL

**Symptom**: Program traps with `SIGILL` (illegal instruction).

**Solution**: The `EXPLICIT_RUNTIME_RESOLVER` macro traps immediately on NULL. Check your resolver logic to ensure all paths return a valid function pointer.

## Technical Details

### How `detect_simd_level_ts()` Works

Key properties:
- Uses `-1` as sentinel (valid SIMD levels are 0-5)
- Lock-free using C11 atomics
- Race-safe: multiple threads may detect simultaneously, but all converge to same value
- Zero overhead after first call (single atomic load)

### How `EXPLICIT_RUNTIME_RESOLVER` Works

Key properties:
- Generates two functions: wrapper (`name`) and implementation (`name_impl`)
- `__attribute__((used))` prevents dead code elimination
- `__builtin_trap()` causes immediate, debuggable crash if NULL
- You write the logic in `name_impl`, the macro provides the wrapper

## References

- [GNU IFUNC Documentation](https://sourceware.org/glibc/wiki/GNU_IFUNC)
- [dlopen Man Page](https://man7.org/linux/man-pages/man3/dlopen.3.html)
- [C11 Atomics](https://en.cppreference.com/w/c/atomic)
- [GCC Function Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
