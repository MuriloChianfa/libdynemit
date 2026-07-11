#!/usr/bin/env bash
# Run Valgrind over all feature test binaries under a build directory.
#
# Usage:
#   ./scripts/run_valgrind.sh --build-dir build-valgrind --tool memcheck
#   ./scripts/run_valgrind.sh --build-dir build-valgrind --tool helgrind --parallel 4
#   ./scripts/run_valgrind.sh --build-dir build-valgrind --tool massif --output-dir reports
#
# Checker tools (memcheck, helgrind, drd) exit non-zero if any test fails.
# Profiling tools (massif, cachegrind, dhat) always exit 0 after collecting reports.

set -euo pipefail

BUILD_DIR=""
TOOL=""
PARALLEL=1
OUTPUT_DIR=""

usage() {
    cat <<'EOF'
Usage: scripts/run_valgrind.sh --build-dir DIR --tool TOOL [options]

Required:
  --build-dir DIR     CMake build directory containing features/*/test_*
  --tool TOOL         memcheck|helgrind|drd|massif|cachegrind|dhat

Options:
  --parallel N        Max concurrent Valgrind processes (default: 1)
  --output-dir DIR    Directory for per-test logs and profiling outputs
                      (default: <build-dir>/valgrind-<tool>)
  -h, --help          Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --tool)
            TOOL="${2:-}"
            shift 2
            ;;
        --parallel)
            PARALLEL="${2:-}"
            shift 2
            ;;
        --output-dir)
            OUTPUT_DIR="${2:-}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [[ -z "$BUILD_DIR" || -z "$TOOL" ]]; then
    echo "Error: --build-dir and --tool are required" >&2
    usage >&2
    exit 2
fi

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Error: build directory not found: $BUILD_DIR" >&2
    exit 2
fi

if ! command -v valgrind >/dev/null 2>&1; then
    echo "Error: valgrind not found in PATH" >&2
    exit 2
fi

case "$TOOL" in
    memcheck|helgrind|drd|massif|cachegrind|dhat) ;;
    *)
        echo "Error: unsupported tool '$TOOL'" >&2
        echo "Supported: memcheck helgrind drd massif cachegrind dhat" >&2
        exit 2
        ;;
esac

if ! [[ "$PARALLEL" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: --parallel must be a positive integer (got: $PARALLEL)" >&2
    exit 2
fi

if [[ -z "$OUTPUT_DIR" ]]; then
    OUTPUT_DIR="${BUILD_DIR}/valgrind-${TOOL}"
fi
mkdir -p "$OUTPUT_DIR"

FEATURES_DIR="${BUILD_DIR}/features"
if [[ ! -d "$FEATURES_DIR" ]]; then
    echo "Error: features directory not found: $FEATURES_DIR" >&2
    exit 2
fi

mapfile -t TESTS < <(find "$FEATURES_DIR" -type f -executable -name 'test_*' ! -path '*/_deps/*' | sort)

if [[ ${#TESTS[@]} -eq 0 ]]; then
    echo "Error: no feature test binaries found under $FEATURES_DIR" >&2
    exit 2
fi

# Checker tools fail the job on Valgrind errors; profiling tools always succeed.
IS_CHECKER=0
case "$TOOL" in
    memcheck|helgrind|drd) IS_CHECKER=1 ;;
esac

echo "============================================"
echo "Valgrind tool:  ${TOOL}"
echo "Build dir:      ${BUILD_DIR}"
echo "Output dir:     ${OUTPUT_DIR}"
echo "Parallelism:    ${PARALLEL}"
echo "Tests found:    ${#TESTS[@]}"
echo "============================================"
echo ""

run_one() {
    local test_bin="$1"
    local feature
    feature="$(basename "$(dirname "$test_bin")")"
    local name
    name="$(basename "$test_bin")"
    local log_file="${OUTPUT_DIR}/${feature}_${TOOL}.log"
    local vg_args=(--tool="$TOOL" --quiet)

    case "$TOOL" in
        memcheck)
            vg_args+=(
                --leak-check=full
                --show-leak-kinds=all
                --track-origins=yes
                --error-exitcode=1
            )
            ;;
        helgrind)
            vg_args+=(--fair-sched=yes --error-exitcode=1)
            ;;
        drd)
            vg_args+=(--error-exitcode=1)
            ;;
        massif)
            vg_args+=(--massif-out-file="${OUTPUT_DIR}/${feature}.massif")
            ;;
        cachegrind)
            vg_args+=(--cachegrind-out-file="${OUTPUT_DIR}/${feature}.cg")
            ;;
        dhat)
            vg_args+=(--dhat-out-file="${OUTPUT_DIR}/${feature}.dhat")
            ;;
    esac

    echo ">>> ${feature}/${name}"
    if valgrind "${vg_args[@]}" "$test_bin" >"$log_file" 2>&1; then
        echo "PASS ${feature} (${TOOL})"
        return 0
    fi

    echo "FAIL ${feature} (${TOOL}) — see ${log_file}"
    # Profiling tools always report success so CI stays green.
    if [[ $IS_CHECKER -eq 1 ]]; then
        return 1
    fi
    return 0
}

PASS=0
FAIL=0
RUNNING=0

reap_one() {
    local rc=0
    wait -n || rc=$?
    RUNNING=$((RUNNING - 1))
    if [[ $rc -eq 0 ]]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
    fi
}

if [[ "$PARALLEL" -eq 1 ]]; then
    for test_bin in "${TESTS[@]}"; do
        if run_one "$test_bin"; then
            PASS=$((PASS + 1))
        else
            FAIL=$((FAIL + 1))
        fi
        echo ""
    done
else
    for test_bin in "${TESTS[@]}"; do
        while [[ $RUNNING -ge $PARALLEL ]]; do
            reap_one
        done
        run_one "$test_bin" &
        RUNNING=$((RUNNING + 1))
        echo ""
    done
    while [[ $RUNNING -gt 0 ]]; do
        reap_one
    done
fi

echo "============================================"
echo "Valgrind ${TOOL} complete"
echo "  Passed: ${PASS}"
echo "  Failed: ${FAIL}"
echo "  Reports: ${OUTPUT_DIR}"
echo "============================================"

if [[ $IS_CHECKER -eq 1 && $FAIL -gt 0 ]]; then
    exit 1
fi
exit 0
