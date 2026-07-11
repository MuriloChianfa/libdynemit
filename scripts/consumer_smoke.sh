#!/usr/bin/env bash
# Install libdynemit into a prefix, then compile and run tiny C/C++ consumers
# against installed headers + pkg-config (shared and static).
#
# Usage:
#   ./scripts/consumer_smoke.sh
#   ./scripts/consumer_smoke.sh --build-dir build-smoke --prefix /tmp/libdynemit-prefix
#   ./scripts/consumer_smoke.sh --skip-build --prefix /tmp/libdynemit-prefix

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${REPO_ROOT}/build-consumer-smoke"
PREFIX="${REPO_ROOT}/install-consumer-smoke"
SKIP_BUILD=0
CC="${CC:-cc}"
CXX="${CXX:-c++}"

usage() {
    cat <<'EOF'
Usage: scripts/consumer_smoke.sh [options]

Options:
  --build-dir DIR     CMake build directory (default: build-consumer-smoke)
  --prefix DIR        Install prefix (default: install-consumer-smoke)
  --skip-build        Skip configure/build/install; use an existing prefix
  -h, --help          Show this help
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            BUILD_DIR="${2:-}"
            shift 2
            ;;
        --prefix)
            PREFIX="${2:-}"
            shift 2
            ;;
        --skip-build)
            SKIP_BUILD=1
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

if [[ -z "$BUILD_DIR" || -z "$PREFIX" ]]; then
    echo "Error: --build-dir and --prefix must be non-empty" >&2
    usage >&2
    exit 2
fi

# Resolve relative paths against the repo root for stable local/CI runs.
[[ "$BUILD_DIR" = /* ]] || BUILD_DIR="${REPO_ROOT}/${BUILD_DIR}"
[[ "$PREFIX" = /* ]] || PREFIX="${REPO_ROOT}/${PREFIX}"

SMOKE_C="${REPO_ROOT}/tests/consumer/smoke.c"
SMOKE_CXX="${REPO_ROOT}/tests/consumer/smoke.cpp"

if [[ ! -f "$SMOKE_C" || ! -f "$SMOKE_CXX" ]]; then
    echo "Error: consumer smoke sources not found under tests/consumer/" >&2
    exit 2
fi

if [[ "$SKIP_BUILD" -eq 0 ]]; then
    echo "=== Configure ==="
    cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$PREFIX" \
        -DDYNEMIT_BUILD_TESTS=OFF \
        -DDYNEMIT_ENABLE_GTEST=OFF

    echo "=== Build ==="
    cmake --build "$BUILD_DIR" -j"$(nproc)"

    echo "=== Install ==="
    rm -rf "$PREFIX"
    cmake --install "$BUILD_DIR"
else
    echo "=== Skip build/install (using existing prefix: ${PREFIX}) ==="
fi

PC_FILE="${PREFIX}/lib/pkgconfig/libdynemit.pc"
if [[ ! -f "$PC_FILE" ]]; then
    # Multiarch layouts (rare for non-/usr prefixes) may use lib/<triplet>/pkgconfig.
    PC_FILE="$(find "$PREFIX" -path '*/pkgconfig/libdynemit.pc' -print -quit 2>/dev/null || true)"
fi

if [[ -z "${PC_FILE}" || ! -f "$PC_FILE" ]]; then
    echo "Error: libdynemit.pc not found under ${PREFIX}" >&2
    exit 1
fi

PC_DIR="$(dirname "$PC_FILE")"
LIBDIR="$(pkg-config --variable=libdir --define-variable=prefix="$PREFIX" "$PC_FILE" 2>/dev/null || true)"
if [[ -z "$LIBDIR" ]]; then
    # Fall back to parsing the installed .pc directly.
    LIBDIR="$(sed -n 's/^libdir=//p' "$PC_FILE" | head -n1)"
    LIBDIR="${LIBDIR//@CMAKE_INSTALL_FULL_LIBDIR@/${PREFIX}/lib}"
    LIBDIR="${LIBDIR//\$\{prefix\}/${PREFIX}}"
    LIBDIR="${LIBDIR//\$\{exec_prefix\}/${PREFIX}}"
fi

export PKG_CONFIG_PATH="${PC_DIR}${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

echo "=== pkg-config ==="
if ! pkg-config --exists libdynemit; then
    echo "Error: pkg-config cannot find libdynemit (PKG_CONFIG_PATH=${PKG_CONFIG_PATH})" >&2
    exit 1
fi

echo "cflags: $(pkg-config --cflags libdynemit)"
echo "libs:   $(pkg-config --libs libdynemit)"
echo "libdir: ${LIBDIR}"

STATIC_LIB="${LIBDIR}/libdynemit.a"
if [[ ! -f "$STATIC_LIB" ]]; then
    echo "Error: static library not found: ${STATIC_LIB}" >&2
    exit 1
fi

OUT_DIR="${BUILD_DIR}/consumer-bin"
mkdir -p "$OUT_DIR"

CFLAGS="$(pkg-config --cflags libdynemit)"
LIBS="$(pkg-config --libs libdynemit)"

echo "=== Shared: C ==="
# shellcheck disable=SC2086
"${CC}" -O2 ${CFLAGS} "${SMOKE_C}" ${LIBS} -o "${OUT_DIR}/smoke_c_shared"
LD_LIBRARY_PATH="${LIBDIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    "${OUT_DIR}/smoke_c_shared"

echo "=== Shared: C++ ==="
# shellcheck disable=SC2086
"${CXX}" -O2 ${CFLAGS} "${SMOKE_CXX}" ${LIBS} -o "${OUT_DIR}/smoke_cxx_shared"
LD_LIBRARY_PATH="${LIBDIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" \
    "${OUT_DIR}/smoke_cxx_shared"

echo "=== Static: C ==="
# Explicit .a: both shared and static are installed, so -ldynemit prefers .so.
# shellcheck disable=SC2086
"${CC}" -O2 ${CFLAGS} "${SMOKE_C}" "${STATIC_LIB}" -lm -o "${OUT_DIR}/smoke_c_static"
"${OUT_DIR}/smoke_c_static"

echo "=== Static: C++ ==="
# shellcheck disable=SC2086
"${CXX}" -O2 ${CFLAGS} "${SMOKE_CXX}" "${STATIC_LIB}" -lm -o "${OUT_DIR}/smoke_cxx_static"
"${OUT_DIR}/smoke_cxx_static"

echo "=== Consumer install smoke: OK ==="
