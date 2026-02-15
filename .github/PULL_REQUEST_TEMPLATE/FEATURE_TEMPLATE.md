## Feature Description

<!-- Provide a clear and concise description of the new feature -->

## Related Issue

<!-- Link to the feature request issue -->

Closes #

## Motivation

<!-- Why is this feature needed? What problem does it solve? -->

## Proposed Changes

<!-- Detailed description of the implementation -->

### API / Public Interface

<!-- New public functions, headers, or types -->

```c
// New API additions
#include <dynemit/your_feature.h>

// Example function signatures
void new_function(const float *input, float *output, size_t n);
```

### SIMD Implementations

<!-- List which SIMD levels are implemented -->

- [ ] Scalar (required)
- [ ] SSE2
- [ ] SSE4.2
- [ ] AVX
- [ ] AVX2
- [ ] AVX-512F

## Implementation Details

<!-- Technical details about the implementation -->

### Core Changes

-
-
-

### New Files Added

-
-
-

### Modified Files

-
-
-

## Design Decisions

<!-- Explain key design choices and trade-offs -->

### Alternatives Considered

1. **Alternative 1**:
   - Pros:
   - Cons:
   - Why not chosen:

2. **Alternative 2**:
   - Pros:
   - Cons:
   - Why not chosen:

### Why This Approach?

<!-- Justify your design choice -->

## Usage Examples

### Basic Usage

```c
#include <dynemit.h>

int main(void) {
    // Example showing how to use the new feature
    
    return 0;
}
```

### Advanced Usage

```c
// More complex usage example
```

## Test Environment

- **OS**: <!-- e.g., Ubuntu 24.04 -->
- **Compiler**: <!-- e.g., GCC 13.2.0 or Clang 16.0.6 -->
- **CMake version**: <!-- e.g., 3.25.1 -->
- **CPU**: <!-- e.g., AMD Ryzen 9 9950X -->
- **SIMD support**: <!-- e.g., AVX-512F, AVX2 -->

## Testing

### Test Coverage

- [ ] Unit tests for new C code
- [ ] C++ compatibility tests
- [ ] Edge case tests (zero-length, large arrays, etc.)
- [ ] Error handling tests
- [ ] All SIMD levels tested for correctness
- [ ] Thread safety tests (if applicable)

### Test Results

```
<!-- Test output from ctest -->
```

### Correctness Verification

<!-- How did you verify SIMD implementations produce correct results? -->

- [ ] Compared SIMD results against scalar implementation
- [ ] Tested with known inputs and expected outputs
- [ ] Verified across different array sizes
- [ ] Checked boundary conditions

## Performance

### Benchmarks

<!-- Required for performance-related features -->

```
<!-- Benchmark results showing performance across SIMD levels -->
```

### Performance Characteristics

- **Speedup vs scalar**: <!-- e.g., 8x on AVX-512, 4x on AVX2 -->
- **Memory bandwidth**: <!-- if relevant -->
- **Latency**: <!-- if relevant -->
- **Best use case**: <!-- what workloads benefit most -->

### Benchmark Methodology

<!-- Describe how you measured performance -->

- [ ] Used project's benchmark framework
- [ ] Tested multiple array sizes
- [ ] Multiple trials with statistical analysis
- [ ] Compared against other libraries (if applicable)

## Documentation

- [ ] Code comments for all public functions
- [ ] README.md updated with new feature
- [ ] docs/ADDING_FEATURES.md updated (if pattern changed)
- [ ] Usage examples provided
- [ ] Header file documentation complete

## Build System Integration

<!-- CMake integration -->

- [ ] Added to feature CMakeLists.txt
- [ ] Added to main CMakeLists.txt (all-in-one library)
- [ ] Header installed correctly
- [ ] Individual library target created
- [ ] Works with both modular and all-in-one builds

## Backward Compatibility

- [ ] Fully backward compatible
- [ ] Deprecates old behavior (migration guide provided)
- [ ] Breaking change (justified and documented)

### API Stability

<!-- Will this API remain stable? -->

## Compiler Compatibility

<!-- What compiler versions does this feature require? -->

- **Minimum GCC version**: <!-- e.g., 13.0 -->
- **Minimum Clang version**: <!-- e.g., 16.0 -->
- **New compiler features used**: <!-- if any -->
- [ ] Works with minimum required versions
- [ ] Tested with both GCC and Clang

## CPU Architecture Support

- [x] x86_64 (SSE2, SSE4.2, AVX, AVX2, AVX-512F)
- [ ] ARM (NEON) - planned for future
- [ ] Other: <!-- specify -->

## Security Considerations

<!-- Any security implications of this feature? -->

- [ ] No security implications
- [ ] Input validation added for all parameters
- [ ] Bounds checking implemented
- [ ] No integer overflow in size calculations
- [ ] Memory alignment requirements documented
- [ ] Safe for use in multi-threaded applications

## Dependencies

- [ ] No new dependencies
- [ ] New dependencies added (justified below)

### New Dependencies Justification

<!-- If new dependencies are required, explain why -->

## Future Work

<!-- Related features or improvements for future PRs -->

-
-

## Checklist

- [ ] Feature is complete and tested
- [ ] All SIMD levels implemented
- [ ] Code follows project standards (C23, tabs, 120 char lines)
- [ ] All tests pass (`ctest --output-on-failure`)
- [ ] Benchmarks show expected performance
- [ ] Documentation complete
- [ ] Usage examples provided
- [ ] Backward compatibility maintained (or justified)
- [ ] CI checks pass
- [ ] Code is formatted with clang-format
- [ ] Tested with both GCC and Clang
- [ ] No new compiler warnings

## Reviewer Notes

<!-- Areas you'd like reviewers to focus on -->

### Review Focus Areas

-
-

### Open Questions

-
-
