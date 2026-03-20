#!/bin/bash

# SIMD Instruction Verification Script
# Analyzes compiled binaries to verify SIMD instruction usage across x86 and AArch64

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

BINARY="${1:-}"
FILTER="${2:-}"  # optional function group filter, e.g. "max_u32"

if [ -z "$BINARY" ] || [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Binary '${BINARY:-<none>}' not found!${NC}"
    echo "Usage: $0 <path/to/binary> [function_filter]"
    echo "Examples:"
    echo "  $0 build-arm/features/max/bench_max"
    echo "  $0 build/features/max/test_max max_u32"
    exit 1
fi

# Detect binary architecture
BINARY_INFO=$(file "$BINARY")
if echo "$BINARY_INFO" | grep -qiE 'aarch64|ARM aarch64'; then
    ARCH="aarch64"
elif echo "$BINARY_INFO" | grep -qi 'x86-64'; then
    ARCH="x86_64"
elif echo "$BINARY_INFO" | grep -qiE 'Intel 80386|i386'; then
    ARCH="i386"
else
    echo -e "${RED}Error: Unsupported architecture in binary${NC}"
    echo "$BINARY_INFO"
    exit 1
fi

# Select the right objdump for the binary's architecture
OBJDUMP=objdump
HOST_ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ] && [ "$HOST_ARCH" != "aarch64" ]; then
    if command -v aarch64-linux-gnu-objdump &>/dev/null; then
        OBJDUMP=aarch64-linux-gnu-objdump
    else
        echo -e "${RED}Error: Need aarch64-linux-gnu-objdump for cross-architecture analysis${NC}"
        exit 1
    fi
fi

DISASM_EXTRA=""
[ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i386" ] && DISASM_EXTRA="-Mintel"

print_header() {
    echo -e "\n${BOLD}${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}${YELLOW}$1${NC}"
    echo -e "${BOLD}${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

check_function_exists() {
    $OBJDUMP -t "$BINARY" 2>/dev/null | grep -q "$1"
}

analyze_simd() {
    local func_name="$1"
    local description="$2"
    local isa_label="$3"
    local pattern="$4"

    echo -e "\n${BOLD}${GREEN}▶ $description${NC}"
    echo -e "  Function: ${CYAN}$func_name${NC}"

    if ! check_function_exists "$func_name"; then
        echo -e "  ${RED}✗ Function not found in binary${NC}"
        return
    fi

    local disasm
    disasm=$($OBJDUMP -d $DISASM_EXTRA --disassemble="$func_name" "$BINARY" 2>/dev/null)

    local instructions
    instructions=$(echo "$disasm" | grep -E "$pattern" | head -5)

    if [ -n "$instructions" ]; then
        echo -e "  ${GREEN}✓ $isa_label instructions detected:${NC}"
        echo "$instructions" | while IFS= read -r line; do
            echo -e "    ${BLUE}$line${NC}"
        done
        return
    fi

    # No SIMD instructions -- check if it delegates to another variant
    local delegate=""
    if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i386" ]; then
        delegate=$(echo "$disasm" | grep -oP '(?:call|jmp)\s+[0-9a-f]+\s+<\K[^>]+' | head -1)
    else
        delegate=$(echo "$disasm" | grep -oP '\b(?:b|bl)\s+[0-9a-f]+\s+<\K[^>]+' | head -1)
    fi

    if [ -n "$delegate" ]; then
        echo -e "  ${YELLOW}→ Delegates to ${BOLD}$delegate${NC}"
        return
    fi

    # No delegation call either. If the function has substantial code, it was
    # inlined from a lower-level variant (e.g. AVX inlining SSE4.2 code).
    local instr_count
    instr_count=$(echo "$disasm" | grep -cE '^\s+[0-9a-f]+:' || echo "0")

    if [ "$instr_count" -gt 10 ]; then
        echo -e "  ${YELLOW}→ Inlined from lower-level variant (no $isa_label-specific instructions)${NC}"
    else
        echo -e "  ${RED}✗ No $isa_label instructions found${NC}"
    fi
}

analyze_scalar() {
    local func_name="$1"

    echo -e "\n${BOLD}${GREEN}▶ Scalar (no SIMD)${NC}"
    echo -e "  Function: ${CYAN}$func_name${NC}"

    if ! check_function_exists "$func_name"; then
        echo -e "  ${RED}✗ Function not found in binary${NC}"
        return
    fi

    echo -e "  ${GREEN}✓ Function found${NC}"
    echo -e "  ${BLUE}First 10 instructions:${NC}"
    $OBJDUMP -d $DISASM_EXTRA --disassemble="$func_name" "$BINARY" 2>/dev/null \
        | grep -E '^\s+[0-9a-f]+:' | head -10 | while IFS= read -r line; do
        echo -e "    ${BLUE}$line${NC}"
    done
}

analyze_resolver() {
    local func_name="$1"

    echo -e "\n${BOLD}${GREEN}▶ $func_name (selects optimal implementation at runtime)${NC}"

    if ! check_function_exists "$func_name"; then
        echo -e "  ${RED}✗ Resolver function not found${NC}"
        return
    fi

    echo -e "  ${GREEN}✓ Resolver function found${NC}"
    echo -e "  ${BLUE}First 15 instructions:${NC}"
    $OBJDUMP -d $DISASM_EXTRA --disassemble="$func_name" "$BINARY" 2>/dev/null \
        | grep -E '^\s+[0-9a-f]+:' | head -15 | while IFS= read -r line; do
        echo -e "    ${BLUE}$line${NC}"
    done
}

# --- Banner ---
echo -e "${BOLD}${CYAN}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║         SIMD Dynamic Dispatch Verification Tool           ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo -e "Binary:       ${BLUE}$BINARY${NC}"
echo -e "Architecture: ${BLUE}$ARCH${NC}"
echo -e "Objdump:      ${BLUE}$OBJDUMP${NC}"
[ -n "$FILTER" ] && echo -e "Filter:       ${BLUE}$FILTER${NC}"

# --- Discover dispatch groups ---
# A dispatch group is identified by a *_scalar symbol; the base name is everything before _scalar.
ALL_SYMBOLS=$($OBJDUMP -t "$BINARY" 2>/dev/null)
BASES=$(echo "$ALL_SYMBOLS" | grep -oP '\b\S+(?=_scalar\b)' | sort -u)

if [ -n "$FILTER" ]; then
    BASES=$(echo "$BASES" | grep "$FILTER" || true)
fi

if [ -z "$BASES" ]; then
    echo -e "\n${RED}No dispatch function groups found${NC}"
    [ -n "$FILTER" ] && echo -e "${YELLOW}Try without a filter, or check the binary has *_scalar symbols${NC}"
    exit 1
fi

GROUP_COUNT=$(echo "$BASES" | wc -w)
echo -e "Groups found: ${BOLD}$GROUP_COUNT${NC}"

# --- Analyze each group ---
for base in $BASES; do
    print_header "Dispatch group: $base"

    echo -e "\n${CYAN}Symbols matching ${base}_*:${NC}"
    echo "$ALL_SYMBOLS" | grep "${base}_" | while IFS= read -r line; do
        echo -e "  ${GREEN}•${NC} $line"
    done

    # SIMD variant analysis (architecture-specific, highest to lowest)
    if [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "i386" ]; then
        analyze_simd "${base}_avx512f" \
            "AVX-512F (512-bit ZMM registers)" "ZMM" \
            "(zmm|vmovups.*zmm|vmulps.*zmm|vpmaxu)"

        analyze_simd "${base}_avx2" \
            "AVX2 (256-bit YMM registers)" "YMM" \
            "(ymm|vmovups.*ymm|vmulps.*ymm|vpmaxu.*ymm)"

        analyze_simd "${base}_avx" \
            "AVX (256-bit YMM registers)" "YMM" \
            "(ymm|vmovups.*ymm|vmulps.*ymm)"

        analyze_simd "${base}_sse42" \
            "SSE4.2 (128-bit XMM registers)" "XMM" \
            "(xmm|movups.*xmm|mulps.*xmm|pmaxu.*xmm)"

        analyze_simd "${base}_sse2" \
            "SSE2 (128-bit XMM registers)" "XMM" \
            "(xmm|movups.*xmm|mulps.*xmm)"
    fi

    if [ "$ARCH" = "aarch64" ]; then
        analyze_simd "${base}_sve2" \
            "SVE2 (scalable vector)" "SVE" \
            "(whilelo|ptrue|ld1[whdb]\t|st1[whdb]\t|cnt[whdb]\t|umax\tz|fmax\tz|p[0-9]+/[zm])"

        analyze_simd "${base}_sve" \
            "SVE (scalable vector)" "SVE" \
            "(whilelo|ptrue|ld1[whdb]\t|st1[whdb]\t|cnt[whdb]\t|umax\tz|fmax\tz|p[0-9]+/[zm])"

        analyze_simd "${base}_neon" \
            "NEON / Advanced SIMD (128-bit V registers)" "NEON" \
            "(\{v[0-9]|v[0-9]+\.[0-9]+[sdwhb]|ld1\t|umaxv|fmaxp|fmaxv|movi\tv)"
    fi

    analyze_scalar "${base}_scalar"

    # Resolver
    if check_function_exists "${base}_resolver"; then
        analyze_resolver "${base}_resolver"
    fi
done

# --- Summary ---
print_header "Summary"
echo ""
for base in $BASES; do
    variant_count=$(echo "$ALL_SYMBOLS" | grep -c "${base}_" || echo "0")
    echo -e "  ${GREEN}•${NC} ${BOLD}$base${NC}: $variant_count variants"
done
echo ""
echo -e "${GREEN}✓${NC} Binary analysis complete!"
echo ""
echo -e "${CYAN}Tip: Run a benchmark to see which SIMD level is selected at runtime:${NC}"
echo -e "  ${BLUE}./build/features/max/bench_max${NC}"
echo ""
