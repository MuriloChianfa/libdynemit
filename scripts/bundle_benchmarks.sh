#!/bin/bash
# Build statically-linked benchmark binaries and package them into a portable
# tar.gz bundle that can run on any arch without dependencies.
#
# Usage:
#   ./scripts/bundle_benchmarks.sh [--arch ARCH] [--strip]
#
# Options:
#   --arch ARCH  Target architecture: x86_64 (default) or aarch64
#   --strip      Strip debug symbols from binaries
#
# Produces: dynemit-bench-{ARCH}.tar.gz

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Feature -> space-separated type variants with benchmark binaries.
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

STRIP_BINS=0
TARGET_ARCH="x86_64"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --strip) STRIP_BINS=1; shift ;;
        --arch)  TARGET_ARCH="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 [--arch ARCH] [--strip]"
            echo "  --arch ARCH  Target: x86_64 (default) or aarch64"
            echo "  --strip      Strip debug symbols from binaries"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if [[ "$TARGET_ARCH" != "x86_64" && "$TARGET_ARCH" != "aarch64" ]]; then
    echo -e "${RED}Error: unsupported architecture '${TARGET_ARCH}'. Use x86_64 or aarch64.${NC}"
    exit 1
fi

BUILD_DIR="build-static-${TARGET_ARCH}"
BUNDLE="dynemit-bench"
TARBALL="${BUNDLE}-${TARGET_ARCH}.tar.gz"

CMAKE_EXTRA_ARGS=()
STRIP_CMD="strip"
if [[ "$TARGET_ARCH" == "aarch64" ]]; then
    CMAKE_EXTRA_ARGS+=(--toolchain cmake/aarch64-linux-gnu.cmake)
    STRIP_CMD="aarch64-linux-gnu-strip"
fi

echo ""
echo -e "${BOLD}${CYAN}=== Building portable benchmark bundle (${TARGET_ARCH}) ===${NC}"
echo ""

# --- Build static binaries ---
echo -e "${CYAN}[1/4] Configuring static build...${NC}"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DDYNEMIT_BUILD_BENCHMARKS=ON \
    -DDYNEMIT_STATIC_BENCHMARKS=ON \
    "${CMAKE_EXTRA_ARGS[@]}" \
    -Wno-dev > /dev/null 2>&1
echo -e "  ${GREEN}done${NC}"

echo -e "${CYAN}[2/4] Compiling...${NC}"
cmake --build "$BUILD_DIR" -j"$(nproc)" > /dev/null 2>&1
echo -e "  ${GREEN}done${NC}"

# --- Verify all binaries exist ---
MISSING=0
BIN_COUNT=0
for feat in "${FEATURES[@]}"; do
    for var in ${FEATURE_VARIANTS[$feat]}; do
        BIN="${BUILD_DIR}/features/${feat}/bench_${feat}_${var}"
        if [[ ! -x "$BIN" ]]; then
            echo -e "  ${RED}Missing:${NC} $BIN"
            MISSING=1
        fi
        BIN_COUNT=$((BIN_COUNT + 1))
    done
done
if [[ "$MISSING" -eq 1 ]]; then
    echo -e "${RED}Build failed: some binaries are missing.${NC}"
    exit 1
fi

# --- Stage bundle ---
echo -e "${CYAN}[3/4] Staging bundle...${NC}"
rm -rf "$BUNDLE"
mkdir -p "$BUNDLE/bin"

for feat in "${FEATURES[@]}"; do
    for var in ${FEATURE_VARIANTS[$feat]}; do
        SRC="${BUILD_DIR}/features/${feat}/bench_${feat}_${var}"
        cp "$SRC" "$BUNDLE/bin/"
        if [[ "$STRIP_BINS" -eq 1 ]]; then
            $STRIP_CMD "$BUNDLE/bin/bench_${feat}_${var}"
        fi
    done
done

# Verify one is truly static
SAMPLE_BIN=$(ls "$BUNDLE/bin/"bench_* | head -1)
SAMPLE=$(file "$SAMPLE_BIN")
if [[ "$SAMPLE" != *"statically linked"* ]]; then
    echo -e "${RED}Warning: $(basename "$SAMPLE_BIN") is not statically linked!${NC}"
    echo "  $SAMPLE"
    if [[ "$TARGET_ARCH" == "aarch64" ]]; then
        echo "  You may need libc6-dev-arm64-cross installed."
    else
        echo "  You may need glibc-static / libc6-dev installed."
    fi
    exit 1
fi

# --- Generate self-contained run.sh ---
cat > "$BUNDLE/run.sh" << 'RUNNER_EOF'
#!/bin/bash
# libdynemit portable benchmark runner
# No build tools or source tree required -- just static binaries.
#
# Usage:
#   sudo ./run.sh [--cpu CORE] [--levels LEVELS] [--features FEATURES]
#
# Options:
#   --cpu CORE          Pin to this CPU core (default: last physical core)
#   --levels LEVELS     Comma-separated SIMD levels to test
#                       (auto-detected from architecture if not specified)
#   --features FEATURES Comma-separated variant names (default: all)

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Feature -> variants mapping
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

ALL_FEATURES=(add concentration entropy gini hhi hill histogram kurtosis
              max mean min mul simpson skewness sub sum topk variance)

PIN_CPU=""
SIMD_LEVELS_STR=""
FEATURES_STR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --cpu)      PIN_CPU="$2"; shift 2 ;;
        --levels)   SIMD_LEVELS_STR="$2"; shift 2 ;;
        --features) FEATURES_STR="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: sudo $0 [--cpu CORE] [--levels LEVELS] [--features FEATURES]"
            echo ""
            echo "Options:"
            echo "  --cpu CORE          Pin to CPU core (default: last physical core)"
            echo "  --levels LEVELS     Comma-separated SIMD levels:"
            echo "                        x86: scalar,sse2,sse4.2,avx,avx2,avx512f"
            echo "                        ARM: scalar,neon,sve,sve2"
            echo "  --features FEATURES Comma-separated variant names (default: all)"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# Auto-detect default SIMD levels based on architecture
HOST_ARCH=$(uname -m)
if [[ -z "$SIMD_LEVELS_STR" ]]; then
    case "$HOST_ARCH" in
        x86_64|i686)  SIMD_LEVELS_STR="scalar,sse2,sse4.2,avx,avx2,avx512f" ;;
        aarch64|arm*) SIMD_LEVELS_STR="scalar,neon,sve,sve2" ;;
        *)            SIMD_LEVELS_STR="scalar" ;;
    esac
fi

IFS=',' read -ra SIMD_LEVELS <<< "$SIMD_LEVELS_STR"

# Build variant list
ALL_VARIANTS=()
if [[ -n "$FEATURES_STR" ]]; then
    IFS=',' read -ra ALL_VARIANTS <<< "$FEATURES_STR"
else
    for feat in "${ALL_FEATURES[@]}"; do
        for var in ${FEATURE_VARIANTS[$feat]}; do
            ALL_VARIANTS+=("${feat}_${var}")
        done
    done
fi

if [[ -z "$PIN_CPU" ]]; then
    THREADS_PER_CORE=$(lscpu -p=cpu,core | grep -v '^#' | awk -F, '{print $2}' | sort -u | wc -l)
    PIN_CPU=$((THREADS_PER_CORE - 1))
fi

# Detect CPU name for header display
CPU_NAME=$(grep 'model name' /proc/cpuinfo 2>/dev/null | head -1 | cut -d: -f2 | sed 's/^ *//')
if [[ -z "$CPU_NAME" ]]; then
    CPU_NAME=$(lscpu 2>/dev/null | grep 'Vendor ID' | cut -d: -f2 | sed 's/^ *//')
    CPU_NAME="${CPU_NAME} $(uname -m)"
fi

echo ""
echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${BLUE}║    libdynemit : Portable Benchmark Runner        ║${NC}"
echo -e "${BOLD}${BLUE}╚══════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  CPU:          ${GREEN}${CPU_NAME}${NC}"
echo -e "  Pinned core:  ${GREEN}${PIN_CPU}${NC}"
echo -e "  Priority:     ${GREEN}nice -n -20${NC}"
echo -e "  Variants:     ${GREEN}${#ALL_VARIANTS[@]}${NC}"
echo -e "  SIMD levels:  ${GREEN}${SIMD_LEVELS[*]}${NC}"
echo -e "  Total runs:   ${GREEN}$(( ${#ALL_VARIANTS[@]} * ${#SIMD_LEVELS[@]} ))${NC}"
echo -e "  Output:       ${YELLOW}bench/cpus/${HOST_ARCH}/...${NC}"
echo ""

# Verify binaries exist
for variant in "${ALL_VARIANTS[@]}"; do
    if [[ ! -x "bin/bench_${variant}" ]]; then
        echo -e "${RED}Missing binary: bin/bench_${variant}${NC}"
        exit 1
    fi
done

TOTAL_START=$(date +%s)
RUN=0
TOTAL_RUNS=$(( ${#ALL_VARIANTS[@]} * ${#SIMD_LEVELS[@]} ))

for variant in "${ALL_VARIANTS[@]}"; do
    echo -e "${BOLD}${BLUE}[${variant}]${NC}"

    for lvl in "${SIMD_LEVELS[@]}"; do
        RUN=$((RUN + 1))
        printf "  %-10s " "$lvl"

        START=$(date +%s%N)
        taskset -c "$PIN_CPU" nice -n -20 \
            ./bin/bench_${variant} --auto-detect --force-level "$lvl" 2>/dev/null
        END=$(date +%s%N)

        ELAPSED_MS=$(( (END - START) / 1000000 ))

        LVL_PAT="${lvl//./_}"
        LVL_PAT="${LVL_PAT/avx512f/avx_512f}"
        CSV=$(ls -t bench/cpus/*/*/data/"${variant}_${LVL_PAT}"*.csv 2>/dev/null | head -1 || true)
        if [[ -n "$CSV" && -f "$CSV" ]]; then
            LAST=$(tail -1 "$CSV")
            GFLOPS=$(echo "$LAST" | cut -d, -f8)
            printf "${GREEN}done${NC}  %6d ms  %s GFLOP/s  [%d/%d]\n" \
                "$ELAPSED_MS" "$GFLOPS" "$RUN" "$TOTAL_RUNS"
        else
            printf "${RED}FAIL${NC}  (no CSV produced)  [%d/%d]\n" "$RUN" "$TOTAL_RUNS"
        fi
    done
    echo ""
done

TOTAL_END=$(date +%s)
echo -e "${GREEN}All benchmarks finished in $((TOTAL_END - TOTAL_START)) seconds.${NC}"
echo ""
echo -e "Results in ${YELLOW}bench/cpus/${HOST_ARCH}/*/data/*.csv${NC}"
echo ""
echo "Next steps:"
echo "  1. Copy the bench/cpus/ tree back to your build machine"
echo "  2. Run:  bash scripts/run_all_benchmarks.sh --charts-only"
echo ""
RUNNER_EOF
chmod +x "$BUNDLE/run.sh"
echo -e "  ${GREEN}done${NC}  (${BIN_COUNT} binaries + run.sh)"

# --- Create tarball ---
echo -e "${CYAN}[4/4] Packaging...${NC}"
tar czf "$TARBALL" "$BUNDLE"
rm -rf "$BUNDLE"

SIZE=$(du -h "$TARBALL" | cut -f1)
echo -e "  ${GREEN}done${NC}  ${TARBALL} (${SIZE})"

echo ""
echo -e "${BOLD}${GREEN}Bundle ready!${NC}  (${TARGET_ARCH})"
echo ""
echo "Usage:"
echo "  scp ${TARBALL} server:"
echo "  ssh server 'tar xzf ${TARBALL} && cd ${BUNDLE} && sudo ./run.sh --cpu 0'"
echo ""
echo "Then copy results back:"
echo "  scp -r 'server:${BUNDLE}/bench/cpus' bench/"
echo "  bash scripts/run_all_benchmarks.sh --charts-only"
echo ""
