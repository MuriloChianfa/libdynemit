#!/bin/bash
# Build script for feature comparison benchmarks
# Builds separate binaries for each SIMD level with specific compiler flags

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Building SIMD Feature Comparison Benchmarks${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Define SIMD levels and their corresponding build flags
declare -A SIMD_LEVELS
SIMD_LEVELS=(
    ["scalar"]="-march=x86-64 -mno-sse2 -mno-avx"
    ["sse2"]="-march=x86-64 -msse2"
    ["sse4_2"]="-march=nehalem"
    ["avx"]="-march=sandybridge"
    ["avx2"]="-march=haswell"
    ["avx512f"]="-march=skylake-avx512"
)

# Build directory prefix
BUILD_PREFIX="build_feature_compare"

# Clean up old builds if they exist
echo -e "${YELLOW}Cleaning up old feature comparison builds...${NC}"
for level in "${!SIMD_LEVELS[@]}"; do
    if [ -d "${BUILD_PREFIX}_${level}" ]; then
        rm -rf "${BUILD_PREFIX}_${level}"
        echo "  Removed ${BUILD_PREFIX}_${level}"
    fi
done
echo ""

# Build each SIMD level
for level in scalar sse2 sse4_2 avx avx2 avx512f; do
    echo -e "${GREEN}Building for SIMD level: ${level}${NC}"
    BUILD_DIR="${BUILD_PREFIX}_${level}"
    FLAGS="${SIMD_LEVELS[$level]}"
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake
    echo "  Configuring..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="${FLAGS}" \
        -DCMAKE_C_FLAGS_RELEASE="-O3 ${FLAGS}" \
        > /dev/null 2>&1
    
    # Build only the feature comparison benchmark
    echo "  Compiling..."
    cmake --build . --target benchmark_vector_mul_feature_compare -j$(nproc) > /dev/null 2>&1
    
    # Copy binary to bench directory with level suffix
    if [ -f "bench/benchmark_vector_mul_feature_compare" ]; then
        cp bench/benchmark_vector_mul_feature_compare "../bench/benchmark_vector_mul_${level}"
        echo -e "  ${GREEN}✓${NC} Binary created: bench/benchmark_vector_mul_${level}"
    else
        echo -e "  ${RED}✗${NC} Failed to build binary for ${level}"
        exit 1
    fi
    
    cd ..
    echo ""
done

echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}All binaries built successfully!${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Built binaries:"
for level in scalar sse2 sse4_2 avx avx2 avx512f; do
    if [ -f "bench/benchmark_vector_mul_${level}" ]; then
        size=$(du -h "bench/benchmark_vector_mul_${level}" | cut -f1)
        echo "  - bench/benchmark_vector_mul_${level} (${size})"
    fi
done
echo ""
echo -e "Run ${YELLOW}./scripts/run_feature_comparison.sh${NC} to execute all benchmarks"
