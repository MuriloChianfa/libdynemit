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

# Feature -> space-separated list of type variants that have benchmark binaries.
# Binary name: bench_{feature}_{variant}  (e.g. bench_max_f64)
# Variant key: {feature}_{variant}        (e.g. max_f64)
declare -A FEATURE_VARIANTS
FEATURE_VARIANTS[add]="f32"
FEATURE_VARIANTS[concentration]="f64"
FEATURE_VARIANTS[entropy]="u16"
FEATURE_VARIANTS[gini]="f64"
FEATURE_VARIANTS[hhi]="u16"
FEATURE_VARIANTS[hill]="f64"
FEATURE_VARIANTS[histogram]="u16"
FEATURE_VARIANTS[kurtosis]="f64"
FEATURE_VARIANTS[max]="f64 u32"
FEATURE_VARIANTS[mean]="f64"
FEATURE_VARIANTS[min]="f64"
FEATURE_VARIANTS[mul]="f32"
FEATURE_VARIANTS[simpson]="u16"
FEATURE_VARIANTS[skewness]="f64"
FEATURE_VARIANTS[sub]="f32"
FEATURE_VARIANTS[sum]="f64"
FEATURE_VARIANTS[topk]="f64"
FEATURE_VARIANTS[variance]="f64"

FEATURES=(add concentration entropy gini hhi hill histogram kurtosis
          max mean min mul simpson skewness sub sum topk variance)

# Build flat list of all variant keys (e.g. add_f32 max_f64 max_u32 ...)
ALL_VARIANTS=()
for feat in "${FEATURES[@]}"; do
    for var in ${FEATURE_VARIANTS[$feat]}; do
        ALL_VARIANTS+=("${feat}_${var}")
    done
done

# Auto-detect SIMD levels based on host architecture
HOST_ARCH=$(uname -m)
case "$HOST_ARCH" in
    x86_64|i686)  SIMD_LEVELS=(scalar sse2 sse4.2 avx avx2 avx512f) ;;
    aarch64|arm*) SIMD_LEVELS=(scalar neon sve sve2) ;;
    *)            SIMD_LEVELS=(scalar) ;;
esac

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
    cpu_name=$(grep "model name" /proc/cpuinfo 2>/dev/null | head -1 | cut -d: -f2 | sed 's/^ *//')

    if [[ -z "$cpu_name" ]] && [[ "$(uname -m)" == "aarch64" ]]; then
        local impl part
        impl=$(awk '/^CPU implementer/ {print $NF; exit}' /proc/cpuinfo)
        part=$(awk '/^CPU part/ {print $NF; exit}' /proc/cpuinfo)

        declare -A ARM_CPUS
        ARM_CPUS["0x41:0xd4f"]="arm_neoverse_v2"
        ARM_CPUS["0x41:0xd49"]="arm_neoverse_n2"
        ARM_CPUS["0x41:0xd40"]="arm_neoverse_v1"
        ARM_CPUS["0x41:0xd0c"]="arm_neoverse_n1"
        ARM_CPUS["0x41:0xd0b"]="arm_cortex_a76"
        ARM_CPUS["0x41:0xd05"]="arm_cortex_a55"
        ARM_CPUS["0x41:0xd07"]="arm_cortex_a57"
        ARM_CPUS["0x41:0xd08"]="arm_cortex_a72"
        ARM_CPUS["0x41:0xd09"]="arm_cortex_a73"
        ARM_CPUS["0x41:0xd0a"]="arm_cortex_a75"
        ARM_CPUS["0x41:0xd0d"]="arm_cortex_a77"
        ARM_CPUS["0x41:0xd41"]="arm_cortex_a78"
        ARM_CPUS["0x41:0xd44"]="arm_cortex_x1"
        ARM_CPUS["0x61:0x022"]="apple_m1_icestorm"
        ARM_CPUS["0x61:0x023"]="apple_m1_firestorm"
        ARM_CPUS["0x61:0x032"]="apple_m2_blizzard"
        ARM_CPUS["0x61:0x033"]="apple_m2_avalanche"

        local key="${impl}:${part}"
        if [[ -n "${ARM_CPUS[$key]+x}" ]]; then
            echo "${ARM_CPUS[$key]}"
        else
            printf "arm_%s_%s" "${impl#0x}" "${part#0x}"
        fi
        return
    fi

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
echo -e "  Variants:     ${GREEN}${#ALL_VARIANTS[@]}${NC}"
echo -e "  SIMD levels:  ${GREEN}${SIMD_LEVELS[*]}${NC}"
echo -e "  Total runs:   ${GREEN}$(( ${#ALL_VARIANTS[@]} * ${#SIMD_LEVELS[@]} ))${NC}"
echo -e "  Output:       ${YELLOW}bench/cpus/${HOST_ARCH}/${CPU_MODEL}/data/${NC}"
echo ""

if [[ "$CHARTS_ONLY" -eq 1 ]]; then
    echo -e "${CYAN}--- Charts-only mode: skipping benchmarks ---${NC}"
    echo ""
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
        for var in ${FEATURE_VARIANTS[$feat]}; do
            BIN="build/features/${feat}/bench_${feat}_${var}"
            if [[ ! -x "$BIN" ]]; then
                echo -e "  ${RED}Missing:${NC} $BIN"
                MISSING=1
            fi
        done
    done
    if [[ "$MISSING" -eq 1 ]]; then
        echo -e "${RED}Some benchmark binaries are missing. Run cmake build first.${NC}"
        exit 1
    fi

    # --- Run benchmarks ---
    echo -e "${CYAN}--- Running benchmarks ---${NC}"
    echo ""

    TOTAL_START=$(date +%s)
    RUN=0
    TOTAL_RUNS=$(( ${#ALL_VARIANTS[@]} * ${#SIMD_LEVELS[@]} ))

    for feat in "${FEATURES[@]}"; do
        for var in ${FEATURE_VARIANTS[$feat]}; do
            VARIANT="${feat}_${var}"
            echo -e "${BOLD}${BLUE}[${VARIANT}]${NC}"
            BIN="build/features/${feat}/bench_${feat}_${var}"

            for lvl in "${SIMD_LEVELS[@]}"; do
                RUN=$((RUN + 1))
                printf "  %-10s " "$lvl"

                START=$(date +%s%N)
                taskset -c "$PIN_CPU" nice -n -20 \
                    "./$BIN" --auto-detect --force-level "$lvl" 2>/dev/null
                END=$(date +%s%N)

                ELAPSED_MS=$(( (END - START) / 1000000 ))

                LVL_PAT="${lvl//./_}"
                LVL_PAT="${LVL_PAT/avx512f/avx_512f}"
                CSV=$(ls -t "bench/cpus/${HOST_ARCH}/${CPU_MODEL}/data/${VARIANT}_${LVL_PAT}"*.csv 2>/dev/null | head -1 || true)
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
    done

    TOTAL_END=$(date +%s)
    echo -e "${GREEN}All benchmarks finished in $((TOTAL_END - TOTAL_START)) seconds.${NC}"
    echo ""
fi

# --- Helper: extract SIMD slug from CSV filename ---
# New format: {variant}_{simd}.csv  (CPU is in the directory path, not the filename)
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

# --- Helper: extract type suffix from variant name ---
variant_type_suffix() {
    for t in f32 f64 u16 u32 u64 histogram; do
        if [[ "$1" == *"_${t}" ]]; then echo "$t"; return; fi
    done
    echo "unknown"
}

# --- Helper: type suffix -> C type name for axis labels ---
type_to_ctype() {
    case "$1" in
        f32) echo "float" ;;
        f64) echo "double" ;;
        u16) echo "uint16_t" ;;
        u32) echo "uint32_t" ;;
        u64) echo "uint64_t" ;;
        *)   echo "$1" ;;
    esac
}

# --- Generate charts ---
#
# Chart layout:
#   bench/cpus/{arch}/{cpu}/features/{variant}/timing.png       — SIMD comparison (time)
#   bench/cpus/{arch}/{cpu}/features/{variant}/throughput.png    — SIMD comparison (GFLOP/s)
#   bench/features/{variant}/timing.png                         — CPU comparison (time)
#   bench/features/{variant}/throughput.png                     — CPU comparison (GFLOP/s)
#
echo -e "${CYAN}--- Generating charts ---${NC}"
echo ""

# Discover all CPU directories across all architectures
shopt -s nullglob
CPU_DIRS=( bench/cpus/*/*/data )
shopt -u nullglob

if [[ ${#CPU_DIRS[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No benchmark data found in bench/cpus/. Skipping chart generation.${NC}"
    exit 0
fi

for variant in "${ALL_VARIANTS[@]}"; do
    tsuffix=$(variant_type_suffix "$variant")
    ctype=$(type_to_ctype "$tsuffix")
    xlabel="Number of elements (${ctype})"

    # Collect all CPUs that have data for this variant
    declare -A SEEN_CPUS
    CPU_LIST=()    # (arch/cpu_slug)
    for data_dir in "${CPU_DIRS[@]}"; do
        shopt -s nullglob
        CSVS=( "${data_dir}/${variant}_"*.csv )
        shopt -u nullglob
        if [[ ${#CSVS[@]} -eq 0 ]]; then continue; fi

        # data_dir = bench/cpus/{arch}/{cpu}/data
        local_path="${data_dir%/data}"          # bench/cpus/{arch}/{cpu}
        cpu_slug=$(basename "$local_path")
        arch_slug=$(basename "$(dirname "$local_path")")
        key="${arch_slug}/${cpu_slug}"
        if [[ -z "${SEEN_CPUS[$key]+x}" ]]; then
            SEEN_CPUS[$key]=1
            CPU_LIST+=("$key")
        fi
    done
    unset SEEN_CPUS

    if [[ ${#CPU_LIST[@]} -eq 0 ]]; then
        echo -e "  ${YELLOW}skip${NC} ${variant} — no CSV data"
        continue
    fi

    # ---- SIMD comparison chart (per CPU) ----
    for cpu_key in "${CPU_LIST[@]}"; do
        cpu_slug=$(basename "$cpu_key")
        arch_slug=$(dirname "$cpu_key")
        cpu_name=$(cpu_display "$cpu_slug")
        DATA_DIR="bench/cpus/${cpu_key}/data"
        IMG_DIR="bench/cpus/${cpu_key}/features/${variant}"
        mkdir -p "$IMG_DIR"

        SIMD_ARGS=()
        SIMD_ORDER=(scalar sse2 sse4_2 avx avx2 avx_512f neon sve sve2)
        for s in "${SIMD_ORDER[@]}"; do
            CSV="${DATA_DIR}/${variant}_${s}.csv"
            if [[ -f "$CSV" ]]; then
                SIMD_ARGS+=("${CSV}:${cpu_name} ($(simd_display "$s"))")
            fi
        done

        if [[ ${#SIMD_ARGS[@]} -gt 0 ]]; then
            python3 scripts/plot_benchmark.py \
                --input "${SIMD_ARGS[@]}" \
                --title "${variant} — SIMD Comparison" \
                --xlabel "$xlabel" \
                --metric time \
                --output "${IMG_DIR}/timing.png" > /dev/null 2>&1

            python3 scripts/plot_benchmark.py \
                --input "${SIMD_ARGS[@]}" \
                --title "${variant} — SIMD Throughput" \
                --xlabel "$xlabel" \
                --metric gflops \
                --output "${IMG_DIR}/throughput.png" > /dev/null 2>&1

            echo -e "  ${GREEN}saved${NC} ${IMG_DIR}/timing.png + throughput.png  (${#SIMD_ARGS[@]} SIMD levels)"
        fi
    done

    # ---- CPU comparison chart (best SIMD per CPU) ----
    CPU_ARGS=()
    for cpu_key in "${CPU_LIST[@]}"; do
        cpu_slug=$(basename "$cpu_key")
        cpu_name=$(cpu_display "$cpu_slug")
        DATA_DIR="bench/cpus/${cpu_key}/data"

        BEST_CSV="" BEST_PRI=0 BEST_SIMD=""
        for s in scalar sse2 sse4_2 avx avx2 avx_512f neon sve sve2; do
            CSV="${DATA_DIR}/${variant}_${s}.csv"
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
        FEAT_IMG_DIR="bench/features/${variant}"
        mkdir -p "$FEAT_IMG_DIR"

        python3 scripts/plot_benchmark.py \
            --input "${CPU_ARGS[@]}" \
            --title "${variant} — CPU Comparison" \
            --xlabel "$xlabel" \
            --metric time \
            --output "${FEAT_IMG_DIR}/timing.png" > /dev/null 2>&1

        python3 scripts/plot_benchmark.py \
            --input "${CPU_ARGS[@]}" \
            --title "${variant} — CPU Throughput Comparison" \
            --xlabel "$xlabel" \
            --metric gflops \
            --output "${FEAT_IMG_DIR}/throughput.png" > /dev/null 2>&1

        echo -e "  ${GREEN}saved${NC} ${FEAT_IMG_DIR}/timing.png + throughput.png  (${#CPU_ARGS[@]} CPUs)"
    fi
done

echo ""
echo -e "${BOLD}${GREEN}Done!${NC}"
echo -e "  SIMD charts:  ${YELLOW}bench/cpus/{arch}/{cpu}/features/{variant}/timing.png${NC}"
echo -e "  CPU charts:   ${YELLOW}bench/features/{variant}/timing.png${NC}"
echo ""
