#!/bin/bash
# Run all feature benchmarks across all SIMD levels on a single pinned CPU core.
# Requires: sudo (for nice -n -20), taskset, cmake build completed.
#
# Usage:
#   sudo ./scripts/run_all_benchmarks.sh [--cpu CORE] [--skip-build] [--charts-only]
#
# Options:
#   --cpu CORE      Pin benchmarks to this CPU core (default: last physical core)
#   --skip-build    Skip the cmake build step
#   --charts-only   Only regenerate charts from existing CSV data

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

FEATURES=(add concentration entropy gini hhi hill histogram kurtosis
          max mean min mul simpson skewness sub sum topk variance)

# Auto-detect SIMD levels based on host architecture
HOST_ARCH=$(uname -m)
case "$HOST_ARCH" in
    x86_64|i686)  SIMD_LEVELS=(scalar sse2 sse4.2 avx avx2 avx512f) ;;
    aarch64|arm*) SIMD_LEVELS=(scalar neon sve sve2) ;;
    *)            SIMD_LEVELS=(scalar) ;;
esac

OUTPUT_DIR="bench/data"
IMG_DIR="docs/img"

# --- Parse arguments ---
PIN_CPU=""
SKIP_BUILD=0
CHARTS_ONLY=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --cpu)      PIN_CPU="$2"; shift 2 ;;
        --skip-build) SKIP_BUILD=1; shift ;;
        --charts-only) CHARTS_ONLY=1; shift ;;
        -h|--help)
            echo "Usage: sudo $0 [--cpu CORE] [--skip-build] [--charts-only]"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# Default: pick the last physical core (avoids core 0 which handles interrupts)
if [[ -z "$PIN_CPU" ]]; then
    TOTAL_CORES=$(nproc --all)
    THREADS_PER_CORE=$(lscpu -p=cpu,core | grep -v '^#' | awk -F, '{print $2}' | sort -u | wc -l)
    PIN_CPU=$((THREADS_PER_CORE - 1))
fi

get_cpu_model() {
    local cpu_name
    cpu_name=$(grep "model name" /proc/cpuinfo | head -1 | cut -d: -f2 | sed 's/^ *//')
    cpu_name=$(echo "$cpu_name" | sed -E 's/[0-9]+-Core//gi' | sed 's/Processor//gi')
    echo "$cpu_name" | tr '[:upper:]' '[:lower:]' | tr -s ' _-' '_' | \
        sed 's/[^a-z0-9_]/_/g' | sed 's/_\+/_/g' | sed 's/^_//;s/_$//'
}

CPU_MODEL=$(get_cpu_model)

# --- Header ---
echo ""
echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${BLUE}║    libdynemit — Full Benchmark Suite             ║${NC}"
echo -e "${BOLD}${BLUE}╚══════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  CPU:          ${GREEN}$(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | sed 's/^ *//')${NC}"
echo -e "  Pinned core:  ${GREEN}${PIN_CPU}${NC}"
echo -e "  Priority:     ${GREEN}nice -n -20 (max)${NC}"
echo -e "  Features:     ${GREEN}${#FEATURES[@]}${NC}"
echo -e "  SIMD levels:  ${GREEN}${SIMD_LEVELS[*]}${NC}"
echo -e "  Total runs:   ${GREEN}$(( ${#FEATURES[@]} * ${#SIMD_LEVELS[@]} ))${NC}"
echo -e "  Output:       ${YELLOW}${OUTPUT_DIR}/${NC}"
echo ""

if [[ "$CHARTS_ONLY" -eq 1 ]]; then
    echo -e "${CYAN}--- Charts-only mode: skipping benchmarks ---${NC}"
    echo ""
    # Jump to chart generation
else
    # --- Build ---
    if [[ "$SKIP_BUILD" -eq 0 ]]; then
        echo -e "${CYAN}--- Building (Release) ---${NC}"
        cmake -B build -DCMAKE_BUILD_TYPE=Release -Wno-dev > /dev/null 2>&1
        cmake --build build -j"$(nproc)" > /dev/null 2>&1
        echo -e "  ${GREEN}Build complete${NC}"
        echo ""
    fi

    # --- Verify binaries ---
    MISSING=0
    for feat in "${FEATURES[@]}"; do
        BIN="build/features/${feat}/bench_${feat}"
        if [[ ! -x "$BIN" ]]; then
            echo -e "  ${RED}Missing:${NC} $BIN"
            MISSING=1
        fi
    done
    if [[ "$MISSING" -eq 1 ]]; then
        echo -e "${RED}Some benchmark binaries are missing. Run cmake build first.${NC}"
        exit 1
    fi

    mkdir -p "$OUTPUT_DIR"

    # --- Run benchmarks ---
    echo -e "${CYAN}--- Running benchmarks ---${NC}"
    echo ""

    TOTAL_START=$(date +%s)
    RUN=0
    TOTAL_RUNS=$(( ${#FEATURES[@]} * ${#SIMD_LEVELS[@]} ))

    for feat in "${FEATURES[@]}"; do
        echo -e "${BOLD}${BLUE}[$feat]${NC}"
        BIN="build/features/${feat}/bench_${feat}"

        for lvl in "${SIMD_LEVELS[@]}"; do
            RUN=$((RUN + 1))
            printf "  %-10s " "$lvl"

            START=$(date +%s%N)
            taskset -c "$PIN_CPU" nice -n -20 \
                "./$BIN" --auto-detect --force-level "$lvl" 2>/dev/null
            END=$(date +%s%N)

            ELAPSED_MS=$(( (END - START) / 1000000 ))

            # Find the generated CSV (normalize level name to match filename)
            LVL_PAT="${lvl//./_}"
            # avx512f -> avx_512f in filenames (bench_auto_detect_open inserts underscore)
            LVL_PAT="${LVL_PAT/avx512f/avx_512f}"
            CSV=$(ls -t "${OUTPUT_DIR}/${feat}_"*"_${LVL_PAT}"*.csv 2>/dev/null | head -1 || true)
            if [[ -n "$CSV" && -f "$CSV" ]]; then
                LAST=$(tail -1 "$CSV")
                GFLOPS=$(echo "$LAST" | cut -d, -f8)
                printf "${GREEN}done${NC}  %6d ms  %s GFLOP/s  [%d/%d]\n" "$ELAPSED_MS" "$GFLOPS" "$RUN" "$TOTAL_RUNS"
            else
                printf "${RED}FAIL${NC}  (no CSV produced)  [%d/%d]\n" "$RUN" "$TOTAL_RUNS"
            fi
        done
        echo ""
    done

    TOTAL_END=$(date +%s)
    echo -e "${GREEN}All benchmarks finished in $((TOTAL_END - TOTAL_START)) seconds.${NC}"
    echo ""
fi

# --- Helper: extract SIMD slug from CSV filename ---
simd_slug_from_csv() {
    local base
    base=$(basename "$1" .csv)
    for s in avx_512f sse4_2 avx2 avx sse2 sve2 sve neon scalar; do
        if [[ "$base" == *"_${s}" ]]; then echo "$s"; return; fi
    done
    echo "unknown"
}

# --- Helper: SIMD slug -> display name ---
simd_display() {
    case "$1" in
        scalar)   echo "Scalar" ;;
        sse2)     echo "SSE2" ;;
        sse4_2)   echo "SSE4.2" ;;
        avx)      echo "AVX" ;;
        avx2)     echo "AVX2" ;;
        avx_512f) echo "AVX-512F" ;;
        neon)     echo "NEON" ;;
        sve)      echo "SVE" ;;
        sve2)     echo "SVE2" ;;
        *)        echo "$1" ;;
    esac
}

# --- Helper: extract CPU slug from CSV filename ---
cpu_slug_from_csv() {
    local base feat="$2"
    base=$(basename "$1" .csv)
    base="${base#${feat}_}"
    for s in avx_512f sse4_2 avx2 avx sse2 sve2 sve neon scalar; do
        if [[ "$base" == *"_${s}" ]]; then
            base="${base%_${s}}"
            break
        fi
    done
    echo "$base"
}

# --- Helper: CPU slug -> display name ---
cpu_display() {
    echo "$1" | sed -e 's/_/ /g' \
                    -e 's/\bamd\b/AMD/gI' \
                    -e 's/\bintel\b/Intel/gI' \
                    -e 's/\bryzen\b/Ryzen/gI' \
                    -e 's/\bxeon\b/Xeon/gI' \
                    -e 's/\bcore\b/Core/gI' \
                    -e 's/\bepyc\b/EPYC/gI' \
                    -e 's/\b9950x3d\b/9950X3D/gI' \
                    -e 's/\b7900x\b/7900X/gI' \
                    -e 's/\b13900k\b/13900K/gI' \
                    -e 's/\barm\b/ARM/gI' \
                    -e 's/\bneoverse\b/Neoverse/gI' \
                    -e 's/\bcortex\b/Cortex/gI' \
                    -e 's/\bapple\b/Apple/gI' \
                    -e 's/\bv1\b/V1/g' \
                    -e 's/\bv2\b/V2/g' \
                    -e 's/\bn1\b/N1/g' \
                    -e 's/\bn2\b/N2/g'
}

# --- Helper: SIMD priority (higher = better) ---
simd_priority() {
    case "$1" in
        avx_512f) echo 6 ;; avx2) echo 5 ;; avx) echo 4 ;;
        sse4_2)   echo 3 ;; sse2) echo 2 ;;
        sve2)     echo 4 ;; sve)  echo 3 ;; neon) echo 2 ;;
        scalar)   echo 1 ;;
        *)        echo 0 ;;
    esac
}

# --- Feature display titles ---
declare -A CHART_TITLES
CHART_TITLES[add]="Add f32"
CHART_TITLES[concentration]="Concentration f64"
CHART_TITLES[entropy]="Entropy"
CHART_TITLES[gini]="Gini"
CHART_TITLES[hhi]="HHI"
CHART_TITLES[hill]="Hill Estimator"
CHART_TITLES[histogram]="Histogram"
CHART_TITLES[kurtosis]="Kurtosis f64"
CHART_TITLES[max]="Max"
CHART_TITLES[mean]="Mean"
CHART_TITLES[min]="Min"
CHART_TITLES[mul]="Mul f32"
CHART_TITLES[simpson]="Simpson"
CHART_TITLES[skewness]="Skewness f64"
CHART_TITLES[sub]="Sub f32"
CHART_TITLES[sum]="Sum"
CHART_TITLES[topk]="Top-K"
CHART_TITLES[variance]="Variance f64"

# --- Generate charts ---
#
# Chart naming convention:
#   {feature}_simd_{cpu-slug}.png  — SIMD level comparison for one CPU
#   {feature}_cpus.png             — CPU comparison (best SIMD per CPU)
#
echo -e "${CYAN}--- Generating charts ---${NC}"
echo ""
mkdir -p "$IMG_DIR"

for feat in "${FEATURES[@]}"; do
    title="${CHART_TITLES[$feat]}"

    shopt -s nullglob
    ALL_CSVS=( "${OUTPUT_DIR}/${feat}_"*.csv )
    shopt -u nullglob
    if [[ ${#ALL_CSVS[@]} -eq 0 ]]; then
        echo -e "  ${YELLOW}skip${NC} $feat — no CSV data"
        continue
    fi

    # Discover unique CPUs for this feature
    declare -A SEEN_CPUS
    CPU_LIST=()
    for csv in "${ALL_CSVS[@]}"; do
        slug=$(cpu_slug_from_csv "$csv" "$feat")
        if [[ -z "${SEEN_CPUS[$slug]+x}" ]]; then
            SEEN_CPUS[$slug]=1
            CPU_LIST+=("$slug")
        fi
    done
    unset SEEN_CPUS

    # ---- SIMD comparison chart (per CPU) ----
    for cpu_slug in "${CPU_LIST[@]}"; do
        cpu_name=$(cpu_display "$cpu_slug")
        cpu_dash="${cpu_slug//_/-}"

        SIMD_ARGS=()
        SIMD_ORDER=(scalar sse2 sse4_2 avx avx2 avx_512f neon sve sve2)
        for s in "${SIMD_ORDER[@]}"; do
            CSV="${OUTPUT_DIR}/${feat}_${cpu_slug}_${s}.csv"
            if [[ -f "$CSV" ]]; then
                SIMD_ARGS+=("${CSV}:$(simd_display "$s")")
            fi
        done

        if [[ ${#SIMD_ARGS[@]} -gt 0 ]]; then
            OUT="${IMG_DIR}/${feat}_simd_${cpu_dash}.png"
            python3 scripts/plot_benchmark.py \
                --input "${SIMD_ARGS[@]}" \
                --title "${title} — SIMD Comparison on ${cpu_name}" \
                --output "$OUT" > /dev/null 2>&1
            echo -e "  ${GREEN}saved${NC} ${OUT}  (${#SIMD_ARGS[@]} SIMD levels)"
        fi
    done

    # ---- CPU comparison chart (best SIMD per CPU) ----
    CPU_ARGS=()
    for cpu_slug in "${CPU_LIST[@]}"; do
        cpu_name=$(cpu_display "$cpu_slug")
        BEST_CSV="" BEST_PRI=0
        for s in scalar sse2 sse4_2 avx avx2 avx_512f neon sve sve2; do
            CSV="${OUTPUT_DIR}/${feat}_${cpu_slug}_${s}.csv"
            if [[ -f "$CSV" ]]; then
                PRI=$(simd_priority "$s")
                if (( PRI > BEST_PRI )); then
                    BEST_PRI=$PRI
                    BEST_CSV="$CSV"
                    BEST_SIMD="$s"
                fi
            fi
        done
        if [[ -n "$BEST_CSV" ]]; then
            LABEL="${cpu_name} ($(simd_display "$BEST_SIMD"))"
            CPU_ARGS+=("${BEST_CSV}:${LABEL}")
        fi
    done

    if [[ ${#CPU_ARGS[@]} -gt 0 ]]; then
        OUT="${IMG_DIR}/${feat}_cpus.png"
        python3 scripts/plot_benchmark.py \
            --input "${CPU_ARGS[@]}" \
            --title "${title} — CPU Comparison" \
            --output "$OUT" > /dev/null 2>&1
        echo -e "  ${GREEN}saved${NC} ${OUT}  (${#CPU_ARGS[@]} CPUs)"
    fi
done

echo ""
echo -e "${BOLD}${GREEN}Done!${NC}"
echo -e "  SIMD charts:  ${YELLOW}${IMG_DIR}/{feature}_simd_{cpu}.png${NC}"
echo -e "  CPU charts:    ${YELLOW}${IMG_DIR}/{feature}_cpus.png${NC}"
echo ""
