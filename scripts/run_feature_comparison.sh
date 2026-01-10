#!/bin/bash
# Script to run all SIMD level benchmarks and collect results
# Outputs CSV files for each SIMD level

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Get CPU model name for filenames
get_cpu_model() {
    local cpu_name=$(grep "model name" /proc/cpuinfo | head -1 | cut -d: -f2 | sed 's/^ *//')
    # Remove core count and processor suffix
    cpu_name=$(echo "$cpu_name" | sed -E 's/[0-9]+-Core//gi' | sed 's/Processor//gi')
    # Convert to lowercase and replace spaces/special chars with underscores
    cpu_name=$(echo "$cpu_name" | tr '[:upper:]' '[:lower:]' | tr -s ' _-' '_' | sed 's/[^a-z0-9_]/_/g' | sed 's/_\+/_/g' | sed 's/^_//;s/_$//')
    echo "$cpu_name"
}

CPU_MODEL=$(get_cpu_model)
OUTPUT_DIR="bench/data"
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Running SIMD Feature Comparison Benchmarks${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "CPU Model: ${GREEN}${CPU_MODEL}${NC}"
echo -e "Output Directory: ${YELLOW}${OUTPUT_DIR}${NC}"
echo ""

# Check if binaries exist
SIMD_LEVELS=("scalar" "sse2" "sse4_2" "avx" "avx2" "avx512f")
MISSING_BINARIES=0

for level in "${SIMD_LEVELS[@]}"; do
    BINARY="bench/benchmark_vector_mul_${level}"
    if [ ! -f "$BINARY" ]; then
        echo -e "${RED}✗${NC} Missing binary: ${BINARY}"
        MISSING_BINARIES=1
    fi
done

if [ $MISSING_BINARIES -eq 1 ]; then
    echo ""
    echo -e "${RED}Error: Some binaries are missing!${NC}"
    echo -e "Run ${YELLOW}./scripts/build_feature_compare.sh${NC} first to build all binaries."
    exit 1
fi

echo -e "${GREEN}All binaries found!${NC}"
echo ""

# Run benchmark for each SIMD level
declare -A RESULTS
TOTAL_TIME_START=$(date +%s)

for level in "${SIMD_LEVELS[@]}"; do
    BINARY="bench/benchmark_vector_mul_${level}"
    # Convert level name for output (sse4_2 -> sse4.2 for display)
    LEVEL_DISPLAY="${level//_/.}"
    OUTPUT_FILE="${OUTPUT_DIR}/simd_levels_${CPU_MODEL}_${level}.csv"
    
    echo -e "${BLUE}Running benchmark: ${level}${NC}"
    echo "  Binary: ${BINARY}"
    echo "  Output: ${OUTPUT_FILE}"
    
    START=$(date +%s)
    
    # Run the benchmark with --csv and --force-level flags
    "$BINARY" --csv --force-level "$LEVEL_DISPLAY" > "$OUTPUT_FILE"
    
    END=$(date +%s)
    DURATION=$((END - START))
    
    # Get a quick summary stat (median time from last row)
    if [ -f "$OUTPUT_FILE" ]; then
        LAST_ROW=$(tail -1 "$OUTPUT_FILE")
        MEDIAN_MS=$(echo "$LAST_ROW" | cut -d',' -f2)
        GFLOPS=$(echo "$LAST_ROW" | cut -d',' -f8)
        RESULTS[$level]="${MEDIAN_MS}ms / ${GFLOPS} GFLOP/s"
        echo -e "  ${GREEN}✓${NC} Completed in ${DURATION}s (4M elements: ${MEDIAN_MS}ms, ${GFLOPS} GFLOP/s)"
    else
        echo -e "  ${RED}✗${NC} Failed to create output file"
        exit 1
    fi
    
    echo ""
done

TOTAL_TIME_END=$(date +%s)
TOTAL_DURATION=$((TOTAL_TIME_END - TOTAL_TIME_START))

echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}All benchmarks completed!${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Total execution time: ${TOTAL_DURATION} seconds"
echo ""
echo "Results Summary (4M elements):"
echo "┌────────────┬─────────────────────────────────┐"
echo "│ SIMD Level │ Performance                     │"
echo "├────────────┼─────────────────────────────────┤"
for level in "${SIMD_LEVELS[@]}"; do
    LEVEL_DISPLAY=$(printf "%-10s" "$level")
    PERF="${RESULTS[$level]}"
    printf "│ %s │ %-31s │\n" "$LEVEL_DISPLAY" "$PERF"
done
echo "└────────────┴─────────────────────────────────┘"
echo ""
echo "CSV files generated:"
for level in "${SIMD_LEVELS[@]}"; do
    OUTPUT_FILE="${OUTPUT_DIR}/simd_levels_${CPU_MODEL}_${level}.csv"
    if [ -f "$OUTPUT_FILE" ]; then
        echo "  - ${OUTPUT_FILE}"
    fi
done
echo ""
echo -e "Next step: Generate comparison chart with:"
echo -e "${YELLOW}  python3 scripts/plot_benchmark.py \\${NC}"
echo -e "${YELLOW}    ${OUTPUT_DIR}/simd_levels_${CPU_MODEL}_*.csv \\${NC}"
echo -e "${YELLOW}    --output docs/img/benchmark_vector_mul_feature_compare.png \\${NC}"
echo -e "${YELLOW}    --title \"SIMD Feature Comparison - ${CPU_MODEL}\"${NC}"
