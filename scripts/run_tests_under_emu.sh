#!/usr/bin/env bash
# Run the libdynemit test suite, optionally under an ISA emulator wrapper.
#
# Without --emu, delegates to ctest (same as the coverage target default).
# With --emu, runs each test binary through the wrapper so SDE/QEMU can
# expose higher SIMD levels than the host CPU reports.
#
# Usage:
#   ./scripts/run_tests_under_emu.sh --build-dir build
#   ./scripts/run_tests_under_emu.sh --build-dir build --emu sde64 -spr --
#   ./scripts/run_tests_under_emu.sh --build-dir build \
#       --emu qemu-aarch64 -cpu max,sve=on,sve2=on --
#
# The wrapper command must end with "--" (the separator before the test binary).

set -euo pipefail

BUILD_DIR=""
WRAPPER=()
USE_CTEST=1

usage() {
    cat <<'EOF'
Usage: scripts/run_tests_under_emu.sh --build-dir DIR [--emu WRAPPER... --]

Required:
  --build-dir DIR     CMake build directory

Optional:
  --emu CMD... --      Emulator prefix ending with "--" (disables ctest path)
  -h, --help           Show this help

Environment:
  DYNEMIT_TEST_WRAPPER  Space-separated wrapper prefix (alternative to --emu)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --emu)
            USE_CTEST=0
            shift
            while [[ $# -gt 0 && "$1" != "--" ]]; do
                WRAPPER+=("$1")
                shift
            done
            if [[ $# -eq 0 || "$1" != "--" ]]; then
                echo "Error: --emu command must end with '--'" >&2
                exit 2
            fi
            shift
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

if [[ -z "$BUILD_DIR" ]]; then
    echo "Error: --build-dir is required" >&2
    usage >&2
    exit 2
fi

if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Error: build directory not found: $BUILD_DIR" >&2
    exit 2
fi

if [[ ${#WRAPPER[@]} -eq 0 && -n "${DYNEMIT_TEST_WRAPPER:-}" ]]; then
    USE_CTEST=0
    read -r -a WRAPPER <<< "$DYNEMIT_TEST_WRAPPER"
    if [[ ${WRAPPER[-1]} != "--" ]]; then
        WRAPPER+=("--")
    fi
fi

if [[ "$USE_CTEST" -eq 1 ]]; then
    exec ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [[ ${#WRAPPER[@]} -eq 0 ]]; then
    echo "Error: emulator wrapper is empty" >&2
    exit 2
fi

run_one() {
    local bin="$1"
    echo "==> ${WRAPPER[*]} $bin"
    "${WRAPPER[@]}" "$bin"
}

FAILED=0
RAN=0

run_if_executable() {
    local bin="$1"
    if [[ -x "$bin" ]]; then
        RAN=$((RAN + 1))
        if ! run_one "$bin"; then
            echo "FAILED: $bin" >&2
            FAILED=$((FAILED + 1))
        fi
    fi
}

CORE_TESTS=(
    test_features
    test_mem
    test_thread_safe_detection
    test_resolver_macro
    test_cpp_basic
    test_cpp_features
    test_cpp_resolver_macro
)

for name in "${CORE_TESTS[@]}"; do
    run_if_executable "${BUILD_DIR}/${name}"
done

FEATURES_DIR="${BUILD_DIR}/features"
if [[ -d "$FEATURES_DIR" ]]; then
    mapfile -t FEATURE_TESTS < <(
        find "$FEATURES_DIR" -type f -executable -name 'test_*' ! -path '*/_deps/*' | sort
    )
    for bin in "${FEATURE_TESTS[@]}"; do
        run_if_executable "$bin"
    done
else
    echo "Warning: no features directory at $FEATURES_DIR" >&2
fi

echo "============================================"
echo "Emulator test run complete: $RAN binaries, $FAILED failed"
echo "============================================"

if [[ "$RAN" -eq 0 ]]; then
    echo "Error: no test binaries found under $BUILD_DIR" >&2
    exit 2
fi

if [[ "$FAILED" -ne 0 ]]; then
    exit 1
fi
