#!/bin/bash
# Build statically-linked benchmark binaries and package them into a portable
# tar.gz bundle that can run on any x86_64 Linux without dependencies.
#
# Usage:
#   ./scripts/bundle_benchmarks.sh [--strip]
#
# Produces: dynemit-bench-x86_64.tar.gz

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

FEATURES=(add concentration entropy gini hhi hill histogram kurtosis
          max mean min mul simpson skewness sub sum topk variance)

STRIP_BINS=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --strip) STRIP_BINS=1; shift ;;
        -h|--help)
            echo "Usage: $0 [--strip]"
            echo "  --strip   Strip debug symbols from binaries (smaller bundle)"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

BUILD_DIR="build-static"
BUNDLE="dynemit-bench"
TARBALL="${BUNDLE}-x86_64.tar.gz"

echo ""
echo -e "${BOLD}${CYAN}=== Building portable benchmark bundle ===${NC}"
echo ""

# --- Build static binaries ---
echo -e "${CYAN}[1/4] Configuring static build...${NC}"
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DDYNEMIT_STATIC_BENCHMARKS=ON \
    -Wno-dev > /dev/null 2>&1
echo -e "  ${GREEN}done${NC}"

echo -e "${CYAN}[2/4] Compiling...${NC}"
cmake --build "$BUILD_DIR" -j"$(nproc)" > /dev/null 2>&1
echo -e "  ${GREEN}done${NC}"

# --- Verify all binaries are static ---
MISSING=0
for feat in "${FEATURES[@]}"; do
    BIN="${BUILD_DIR}/features/${feat}/bench_${feat}"
    if [[ ! -x "$BIN" ]]; then
        echo -e "  ${RED}Missing:${NC} $BIN"
        MISSING=1
    fi
done
if [[ "$MISSING" -eq 1 ]]; then
    echo -e "${RED}Build failed: some binaries are missing.${NC}"
    exit 1
fi

# --- Stage bundle ---
echo -e "${CYAN}[3/4] Staging bundle...${NC}"
rm -rf "$BUNDLE"
mkdir -p "$BUNDLE/bin" "$BUNDLE/data"

for feat in "${FEATURES[@]}"; do
    SRC="${BUILD_DIR}/features/${feat}/bench_${feat}"
    cp "$SRC" "$BUNDLE/bin/"
    if [[ "$STRIP_BINS" -eq 1 ]]; then
        strip "$BUNDLE/bin/bench_${feat}"
    fi
done

# Verify one is truly static
SAMPLE=$(file "$BUNDLE/bin/bench_add")
if [[ "$SAMPLE" != *"statically linked"* ]]; then
    echo -e "${RED}Warning: bench_add is not statically linked!${NC}"
    echo "  $SAMPLE"
    echo "  You may need glibc-static / libc6-dev installed."
    exit 1
fi

# --- Generate self-contained run.sh ---
cat > "$BUNDLE/run.sh" << 'RUNNER_EOF'
#!/bin/bash
# libdynemit portable benchmark runner
# No build tools or source tree required -- just static binaries.
#
# Usage:
#   sudo ./run.sh [--cpu CORE] [--levels LEVELS]
#
# Options:
#   --cpu CORE       Pin to this CPU core (default: last physical core)
#   --levels LEVELS  Comma-separated SIMD levels to test
#                    (default: scalar,sse2,sse4.2,avx,avx2,avx512f)

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

FEATURES=(add concentration entropy gini hhi hill histogram kurtosis
          max mean min mul simpson skewness sub sum topk variance)

PIN_CPU=""
SIMD_LEVELS_STR="scalar,sse2,sse4.2,avx,avx2,avx512f"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --cpu)    PIN_CPU="$2"; shift 2 ;;
        --levels) SIMD_LEVELS_STR="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: sudo $0 [--cpu CORE] [--levels LEVELS]"
            echo ""
            echo "Options:"
            echo "  --cpu CORE       Pin to CPU core (default: last physical core)"
            echo "  --levels LEVELS  Comma-separated: scalar,sse2,sse4.2,avx,avx2,avx512f"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

IFS=',' read -ra SIMD_LEVELS <<< "$SIMD_LEVELS_STR"

if [[ -z "$PIN_CPU" ]]; then
    THREADS_PER_CORE=$(lscpu -p=cpu,core | grep -v '^#' | awk -F, '{print $2}' | sort -u | wc -l)
    PIN_CPU=$((THREADS_PER_CORE - 1))
fi

echo ""
echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${BLUE}║    libdynemit — Portable Benchmark Runner        ║${NC}"
echo -e "${BOLD}${BLUE}╚══════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  CPU:          ${GREEN}$(grep 'model name' /proc/cpuinfo | head -1 | cut -d: -f2 | sed 's/^ *//')${NC}"
echo -e "  Pinned core:  ${GREEN}${PIN_CPU}${NC}"
echo -e "  Priority:     ${GREEN}nice -n -20${NC}"
echo -e "  Features:     ${GREEN}${#FEATURES[@]}${NC}"
echo -e "  SIMD levels:  ${GREEN}${SIMD_LEVELS[*]}${NC}"
echo -e "  Total runs:   ${GREEN}$(( ${#FEATURES[@]} * ${#SIMD_LEVELS[@]} ))${NC}"
echo -e "  Output:       ${YELLOW}data/${NC}"
echo ""

# Verify binaries exist
for feat in "${FEATURES[@]}"; do
    if [[ ! -x "bin/bench_${feat}" ]]; then
        echo -e "${RED}Missing binary: bin/bench_${feat}${NC}"
        exit 1
    fi
done

mkdir -p data

TOTAL_START=$(date +%s)
RUN=0
TOTAL_RUNS=$(( ${#FEATURES[@]} * ${#SIMD_LEVELS[@]} ))

for feat in "${FEATURES[@]}"; do
    echo -e "${BOLD}${BLUE}[$feat]${NC}"

    for lvl in "${SIMD_LEVELS[@]}"; do
        RUN=$((RUN + 1))
        printf "  %-10s " "$lvl"

        START=$(date +%s%N)
        taskset -c "$PIN_CPU" nice -n -20 \
            ./bin/bench_${feat} --auto-detect --force-level "$lvl" 2>/dev/null
        END=$(date +%s%N)

        ELAPSED_MS=$(( (END - START) / 1000000 ))

        LVL_PAT="${lvl//./_}"
        LVL_PAT="${LVL_PAT/avx512f/avx_512f}"
        CSV=$(ls -t "data/${feat}_"*"_${LVL_PAT}"*.csv 2>/dev/null | head -1 || true)
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
echo -e "Results in ${YELLOW}data/*.csv${NC}"
echo ""
echo "Next steps:"
echo "  1. Copy data/*.csv back to your build machine's bench/data/"
echo "  2. Run:  bash scripts/run_all_benchmarks.sh --charts-only"
echo ""
RUNNER_EOF
chmod +x "$BUNDLE/run.sh"
echo -e "  ${GREEN}done${NC}  (${#FEATURES[@]} binaries + run.sh)"

# --- Create tarball ---
echo -e "${CYAN}[4/4] Packaging...${NC}"
tar czf "$TARBALL" "$BUNDLE"
rm -rf "$BUNDLE"

SIZE=$(du -h "$TARBALL" | cut -f1)
echo -e "  ${GREEN}done${NC}  ${TARBALL} (${SIZE})"

echo ""
echo -e "${BOLD}${GREEN}Bundle ready!${NC}"
echo ""
echo "Usage:"
echo "  scp ${TARBALL} server:"
echo "  ssh server 'tar xzf ${TARBALL} && cd ${BUNDLE} && sudo ./run.sh --cpu 0'"
echo ""
echo "Then copy results back:"
echo "  scp 'server:${BUNDLE}/data/*.csv' bench/data/"
echo "  bash scripts/run_all_benchmarks.sh --charts-only"
echo ""
