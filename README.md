# libdynemit

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C23](https://img.shields.io/badge/std-C23-blue.svg)](https://en.cppreference.com/w/c/23)
[![GCC 13+](https://img.shields.io/badge/GCC-13%2B-green.svg)](https://gcc.gnu.org/)

> **High-performance C23 library for runtime CPU feature detection and automatic SIMD dispatch**

Write once, run optimally everywhere. **libdynemit** automatically selects the best SIMD implementation for your CPU at program startup—achieving **up to 16× speedup** with zero runtime overhead.

## Performance First

```c
#include <dynemit.h>

// Automatically uses AVX-512, AVX2, AVX, SSE4.2, SSE2, or scalar
// based on your CPU's capabilities—decided once at program load
vector_mul_f32(a, b, result, n);
```

## Why libdynemit?

| Feature | Benefit |
|---------|---------|
| **Zero Runtime Overhead** | Dispatch happens once at program load using GCC's `ifunc` |
| **Automatic Detection** | CPUID & XGETBV intrinsics detect CPU capabilities |
| **Modular** | Link only what you need—core + individual features |
| **Modern C23** | Clean, type-safe code using latest C standard |

## Quick Start

```bash
# Clone and build
git clone https://github.com/murilo/libdynemit.git
cd libdynemit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# Run benchmark to see the magic
./bench/benchmark_vector_mul
```

**Example output:**
```
Detected SIMD level: AVX2
n = 1048576, iters = 2000
elapsed = 4.129774 s
throughput ~= 6.09 GB/s
GFLOP/s   ~= 0.51
correctness check: OK ✓
```

## Project Structure

```
libdynemit/
├── CMakeLists.txt          # Main CMake configuration
├── include/
│   ├── dynemit.h           # Umbrella header (includes all features)
│   └── dynemit/
│       ├── core.h          # CPU detection API
│       ├── vector_add.h    # Vector addition feature
│       ├── vector_mul.h    # Vector multiplication feature
│       └── ... and more
├── src/
│   ├── CMakeLists.txt      # Core library build config
│   ├── dynemit.c           # CPU feature detection implementation
│   └── dynemit_features.c  # Feature list for all-in-one library
├── features/
│   ├── vector_add/
│   │   ├── CMakeLists.txt
│   │   └── vector_add.c    # SIMD add implementations
│   ├── vector_mul/
│   │   ├── CMakeLists.txt
│   │   └── vector_mul.c    # SIMD multiply implementations
│   └── ... and more
├── bench/
│   ├── CMakeLists.txt      # Benchmark CMake config
│   └── benchmark_vector_mul.c  # Benchmark program
├── tests/
│   ├── CMakeLists.txt      # Tests CMake config
│   ├── test_features.c     # Feature discovery test
│   └── test_vector_ops.c   # Vector operations correctness test
├── docs/
│   ├── ADDING_FEATURES.md  # Guide for adding new features
│   └── ARCHITECTURE.md     # Internal architecture documentation
├── scripts/
│   └── check_for_simd.sh   # Verify SIMD instructions in binary
└── README.md
```

## Building

### Requirements

- **GCC 13+** (required for C23 support, ifunc, and SIMD intrinsics)
- **CMake** 3.16 or higher
- **x86_64 architecture** (for SIMD optimizations)

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run the benchmark
./bench/benchmark_vector_mul
```

<details>
<summary><b>Build Options</b></summary>

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (default, -O3 optimization)
cmake .. -DCMAKE_BUILD_TYPE=Release

# List available features
cmake .. -DLIST_FEATURES=ON
```

</details>

<details>
<summary><b>Running Tests</b></summary>

```bash
# Build and run tests
cd build
make
ctest --verbose

# Or run individual test
./tests/test_features
```

</details>

## Usage

### Running the Benchmark

The benchmark program detects your CPU's SIMD capabilities and runs a performance test:

```bash
./build/bench/benchmark_vector_mul
```

Example output:
```
Detected SIMD level: AVX
n = 1048576, iters = 2000
elapsed = 4.129774 s
throughput ~= 6.09 GB/s (counting a+b+out)
GFLOP/s   ~= 0.51
correctness check: OK (first 16 elements)
```

<details>
<summary><b>Verifying SIMD Instructions</b></summary>

Use the included verification script to inspect which SIMD instructions were compiled into the binary:

```bash
./scripts/check_for_simd.sh
```

This will show:
- All `vector_mul_f32` function variants in the symbol table
- Actual SIMD instructions used in each implementation
- The ifunc resolver function that performs runtime dispatch

</details>

<details>
<summary><b>How It Works</b> (Technical Details)</summary>

### 1. CPU Feature Detection

The `detect_simd_level()` function uses CPUID and XGETBV instructions to query:
- Available instruction set extensions (SSE2, SSE4.2, AVX, AVX2, AVX-512F)
- OS support for saving/restoring SIMD register state (XCR0)

```c
simd_level_t level = detect_simd_level();
// Returns highest supported SIMD level
```

### 2. Multiple SIMD Implementations

Each SIMD level has its own implementation compiled with appropriate GCC target attributes:

```c
__attribute__((target("avx2")))
static void vector_mul_f32_avx2(const float *a, const float *b, float *out, size_t n)
{
    // AVX2 implementation using 256-bit YMM registers
}
```

### 3. Runtime Dispatch with ifunc

The `vector_mul_f32()` function uses GCC's ifunc attribute to resolve to the optimal implementation:

```c
vector_mul_f32_func_t vector_mul_f32_resolver(void)
{
    simd_level_t level = detect_simd_level();
    switch (level) {
        case SIMD_AVX512F: return vector_mul_f32_avx512f;
        case SIMD_AVX2:    return vector_mul_f32_avx2;
        // ... other cases
    }
}

void vector_mul_f32(const float *, const float *, float *, size_t)
    __attribute__((ifunc("vector_mul_f32_resolver")));
```

This happens **once** at program load time, making subsequent calls as fast as direct function calls.

</details>

<details>
<summary><b>Performance Considerations</b></summary>

- **Alignment**: The benchmark uses 64-byte aligned buffers for optimal memory access
- **Loop unrolling**: Each SIMD implementation processes multiple elements per iteration
- **Scalar tail**: Handles remaining elements that don't fit in SIMD registers
- **Compiler optimization**: Built with `-O3` in release mode

### Expected Performance Gains

Compared to scalar code, approximate speedups:
- **AVX-512F**: ~16x (processes 16 floats per instruction)
- **AVX/AVX2**: ~8x (processes 8 floats per instruction)
- **SSE2/SSE4.2**: ~4x (processes 4 floats per instruction)

Actual performance depends on memory bandwidth, CPU architecture, and data access patterns.

</details>

## Installation

```bash
# Install library and headers
cd build
sudo make install
```

<details>
<summary>View installed files</summary>

**Libraries** (6 options available):
- `/usr/local/lib/libdynemit.a` (all-in-one, includes all features)
- `/usr/local/lib/libdynemit_core.a` (just CPU detection)
- `/usr/local/lib/libdynemit_vector_add.a` (single feature)
- `/usr/local/lib/libdynemit_vector_mul.a` (single feature)
- ... and more

**Headers:**
- `/usr/local/include/dynemit.h` (umbrella header)
- `/usr/local/include/dynemit/core.h`
- `/usr/local/include/dynemit/vector_add.h`
- `/usr/local/include/dynemit/vector_mul.h`
- ... and more

</details>

## Library Usage Options

The library provides flexible usage options depending on your needs:

<details open>
<summary><b>Option 1: All-in-One Library</b> (Recommended for Simplicity)</summary>

Use the bundled library that includes all features:

```c
#include <dynemit.h>  // Includes core + all features

int main(void) {
    // Query available features at runtime
    const char **features = dynemit_features();
    printf("Available features:\n");
    for (int i = 0; features[i] != NULL; i++) {
        printf("  - %s\n", features[i]);
    }
    
    simd_level_t level = detect_simd_level();
    printf("SIMD level: %s\n", simd_level_name(level));
    
    // Use any of the vector operations
    float a[1024], b[1024], result[1024];
    vector_add_f32(a, b, result, 1024);
    vector_mul_f32(a, b, result, 1024);
    vector_sub_f32(a, b, result, 1024);
    
    return 0;
}
```

Compile and link:
```bash
gcc -O3 myprogram.c -ldynemit -lm -o myprogram
```

</details>

<details>
<summary><b>Option 2: Modular Libraries</b> (For Minimal Binary Size)</summary>

Include only the features you need:

```c
#include <dynemit/core.h>
#include <dynemit/vector_add.h>
#include <dynemit/vector_mul.h>

int main(void) {
    simd_level_t level = detect_simd_level();
    float a[1024], b[1024], result[1024];
    
    vector_add_f32(a, b, result, 1024);
    vector_mul_f32(a, b, result, 1024);
    
    return 0;
}
```

Compile and link:
```bash
gcc -O3 myprogram.c -ldynemit_core -ldynemit_vector_add -ldynemit_vector_mul -lm -o myprogram
```

</details>

<details>
<summary><b>Option 3: Core Only</b></summary>

If you only need CPU detection:

```c
#include <dynemit/core.h>

int main(void) {
    simd_level_t level = detect_simd_level();
    printf("CPU supports: %s\n", simd_level_name(level));
    return 0;
}
```

Compile and link:
```bash
gcc -O3 myprogram.c -ldynemit_core -lm -o myprogram
```

</details>

## Technical Details

### C23 Standard

**libdynemit** is written in **C23** (ISO/IEC 9899:2023), taking advantage of modern C features.

The build system enforces C23 compliance and requires **GCC 13 or newer** for proper C23 support.

### GCC Target Attributes

Each function variant is compiled with specific `-march` flags:
- `target("default")`: Scalar implementation, no special instructions
- `target("sse2")`: SSE2 instructions + 128-bit XMM registers
- `target("sse4.2")`: SSE4.2 instructions + 128-bit XMM registers
- `target("avx")`: AVX instructions + 256-bit YMM registers
- `target("avx2")`: AVX2 instructions + 256-bit YMM registers
- `target("avx512f")`: AVX-512 Foundation instructions + 512-bit ZMM registers

### CPUID Detection

The implementation checks:
- **Leaf 0x01**: SSE2, SSE4.2, OSXSAVE, AVX support bits
- **Leaf 0x07**: AVX2, AVX-512F support bits
- **XCR0**: OS support for YMM (AVX) and ZMM (AVX-512) state saving

For more details on the internal architecture, see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

### Adding New Features

For detailed instructions on how to add new SIMD-optimized features, see [docs/ADDING_FEATURES.md](docs/ADDING_FEATURES.md).

Quick summary:

1. **Create feature directory**: `features/your_feature/`
2. **Add source file**: `features/your_feature/your_feature.c`
3. **Create header**: `include/dynemit/your_feature.h`
4. **Add CMakeLists.txt** following the pattern:
   ```cmake
   # Object library for bundling
   add_library(your_feature_obj OBJECT your_feature.c)
   target_include_directories(your_feature_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
   target_link_libraries(your_feature_obj PUBLIC dynemit_core)
   
   # Individual static library
   add_library(dynemit_your_feature STATIC $<TARGET_OBJECTS:your_feature_obj>)
   target_include_directories(dynemit_your_feature PUBLIC ${PROJECT_SOURCE_DIR}/include)
   target_link_libraries(dynemit_your_feature PUBLIC dynemit_core)
   
   # Installation
   install(TARGETS dynemit_your_feature ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
   install(FILES ${PROJECT_SOURCE_DIR}/include/dynemit/your_feature.h 
           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dynemit)
   ```
5. **Update main CMakeLists.txt**: Add to `dynemit` all-in-one library
6. **Update umbrella header**: Add `#include <dynemit/your_feature.h>` in `include/dynemit.h`

## License

See [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Areas for improvement:
- Additional SIMD operations (add, subtract, fused multiply-add, etc.)
- ARM NEON support
- AMD-specific optimizations (FMA4, XOP)
- Additional benchmarks and test cases

## References

- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [GCC Function Multi-versioning](https://gcc.gnu.org/onlinedocs/gcc/Function-Multiversioning.html)
- [GCC ifunc Attribute](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-ifunc-function-attribute)
- [x86 CPUID Instruction](https://en.wikipedia.org/wiki/CPUID)
