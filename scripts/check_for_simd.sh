#!/bin/bash

# SIMD Instruction Verification Script
# Analyzes the compiled binary to verify SIMD instruction usage

set -e

# Colors for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Binary to analyze
BINARY="${1:-build/bench/benchmark_vector_mul}"

if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Binary '$BINARY' not found!${NC}"
    echo "Usage: $0 [path/to/binary]"
    echo "Example: $0 build/bench/benchmark_vector_mul"
    exit 1
fi

echo -e "${BOLD}${CYAN}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║         SIMD Dynamic Dispatch Verification Tool           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo -e "Binary: ${BLUE}$BINARY${NC}\n"

# Function to print section header
print_header() {
    echo -e "${BOLD}${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${YELLOW}$1${NC}"
    echo -e "${BOLD}${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

# Function to check if a function exists in binary
check_function_exists() {
    local func_name="$1"
    if objdump -t "$BINARY" 2>/dev/null | grep -q "$func_name"; then
        return 0
    else
        return 1
    fi
}

# Function to analyze SIMD instructions
analyze_simd() {
    local func_name="$1"
    local description="$2"
    local register_type="$3"
    local pattern="$4"
    
    echo -e "\n${BOLD}${GREEN}▶ $description${NC}"
    echo -e "  Function: ${CYAN}$func_name${NC}"
    
    if ! check_function_exists "$func_name"; then
        echo -e "  ${RED}✗ Function not found in binary${NC}"
        return
    fi
    
    local instructions=$(objdump -d -Mintel --disassemble="$func_name" "$BINARY" 2>/dev/null | grep -E "$pattern" | head -5)
    
    if [ -z "$instructions" ]; then
        echo -e "  ${RED}✗ No $register_type instructions found${NC}"
    else
        echo -e "  ${GREEN} $register_type instructions detected:${NC}"
        echo "$instructions" | while IFS= read -r line; do
            echo -e "    ${BLUE}$line${NC}"
        done
    fi
}

# 1. Symbol Table
print_header "1. Symbol Table - vector_mul Functions"
echo -e "\n${CYAN}Available vector_mul_f32 functions in binary:${NC}\n"
objdump -t "$BINARY" | grep vector_mul_f32 | while IFS= read -r line; do
    echo -e "  ${GREEN}•${NC} $line"
done

# 2. SIMD Instruction Analysis
print_header "2. SIMD Instruction Verification"

analyze_simd \
    "vector_mul_f32_avx512f" \
    "AVX-512F (512-bit ZMM registers)" \
    "ZMM" \
    "(zmm|vmovups.*zmm|vmulps.*zmm)"

analyze_simd \
    "vector_mul_f32_avx2" \
    "AVX2 (256-bit YMM registers)" \
    "YMM" \
    "(ymm|vmovups.*ymm|vmulps.*ymm)"

analyze_simd \
    "vector_mul_f32_avx" \
    "AVX (256-bit YMM registers)" \
    "YMM" \
    "(ymm|vmovups.*ymm|vmulps.*ymm)"

analyze_simd \
    "vector_mul_f32_sse42" \
    "SSE4.2 (128-bit XMM registers)" \
    "XMM" \
    "(xmm|movups.*xmm|mulps.*xmm)"

analyze_simd \
    "vector_mul_f32_sse2" \
    "SSE2 (128-bit XMM registers)" \
    "XMM" \
    "(xmm|movups.*xmm|mulps.*xmm)"

# 3. Scalar version
echo -e "\n${BOLD}${GREEN}▶ Scalar (No SIMD, regular operations)${NC}"
echo -e "  Function: ${CYAN}vector_mul_f32_scalar${NC}"
if check_function_exists "vector_mul_f32_scalar"; then
    echo -e "  ${GREEN} Function found${NC}"
    echo -e "  ${BLUE}First 10 instructions:${NC}"
    objdump -d -Mintel --disassemble=vector_mul_f32_scalar "$BINARY" 2>/dev/null | grep -E "^\s+[0-9a-f]+:" | head -10 | while IFS= read -r line; do
        echo -e "    ${BLUE}$line${NC}"
    done
else
    echo -e "  ${RED}✗ Function not found in binary${NC}"
fi

# 4. Resolver (ifunc dispatcher)
print_header "3. Runtime Dispatcher (ifunc resolver)"
echo -e "\n${BOLD}${GREEN}▶ vector_mul_f32_resolver (selects optimal implementation at runtime)${NC}"
if check_function_exists "vector_mul_f32_resolver"; then
    echo -e "  ${GREEN} Resolver function found${NC}"
    echo -e "  ${BLUE}First 15 instructions:${NC}"
    objdump -d -Mintel --disassemble=vector_mul_f32_resolver "$BINARY" 2>/dev/null | grep -E "^\s+[0-9a-f]+:" | head -15 | while IFS= read -r line; do
        echo -e "    ${BLUE}$line${NC}"
    done
else
    echo -e "  ${RED}✗ Resolver function not found${NC}"
fi

# Summary
print_header "4. Summary"
echo ""
total_funcs=$(objdump -t "$BINARY" 2>/dev/null | grep -c "vector_mul_f32" || echo "0")
echo -e "${GREEN}${NC} Total vector_mul_f32 variants found: ${BOLD}$total_funcs${NC}"
echo -e "${GREEN}${NC} Binary analysis complete!"
echo ""
echo -e "${CYAN}Tip: Run the benchmark to see which SIMD level is selected at runtime:${NC}"
echo -e "  ${BLUE}./build/bench/benchmark_vector_mul${NC}"
echo ""


