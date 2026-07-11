#!/usr/bin/env bash
# Build and run ctest inside the cached aarch64 Docker image (no apt on each run).
#
# Usage: ./scripts/run_aarch64_tests.sh [--build-dir DIR] [--coverage] [--sve]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="build-aarch64-test"
COVERAGE=0
SVE=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --coverage) COVERAGE=1; shift ;;
        --sve) SVE=1; shift ;;
        -h|--help)
            echo "Usage: $0 [--build-dir DIR] [--coverage] [--sve]"
            exit 0
            ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

if ! docker info >/dev/null 2>&1; then
    echo "Docker daemon not running." >&2
    exit 1
fi

IMAGE="$("$ROOT/scripts/ensure_docker_coverage_aarch64.sh")"

if ! docker run --rm --platform linux/arm64 "$IMAGE" uname -m >/dev/null 2>&1; then
    echo "Registering QEMU binfmt for linux/arm64 Docker..."
    docker run --rm --privileged multiarch/qemu-user-static --reset -p yes >/dev/null
fi

COVERAGE_FLAG=""
[[ "$COVERAGE" -eq 1 ]] && COVERAGE_FLAG="-DDYNEMIT_COVERAGE=ON"
GTEST_FLAG="-DDYNEMIT_ENABLE_GTEST=OFF"
HOST_ARCH="$(uname -m)"
AARCH64_ON_X86=0
if [[ "$HOST_ARCH" != "aarch64" && "$HOST_ARCH" != "arm64" ]]; then
    AARCH64_ON_X86=1
fi

docker run --rm --platform linux/arm64 \
    --ulimit stack=67108864:67108864 \
    -e AARCH64_ON_X86="$AARCH64_ON_X86" \
    -v "$ROOT:/work" -w /work \
    "$IMAGE" bash -euxo pipefail -c "
        uid=$(id -u)
        gid=$(id -g)
        if [[ \"\$AARCH64_ON_X86\" == 1 ]]; then
            export DYNEMIT_MAX_SIMD_LEVEL=10
        fi
        chown -R \"\$uid:\$gid\" /work/$BUILD_DIR 2>/dev/null || true
        rm -rf '$BUILD_DIR'
        cmake -B '$BUILD_DIR' -DCMAKE_BUILD_TYPE=Debug $COVERAGE_FLAG $GTEST_FLAG
        for attempt in 1 2 3 4 5; do
            cmake --build '$BUILD_DIR' -j1 && break
            echo \"build attempt \${attempt} failed; retrying...\" >&2
            [[ \"\$attempt\" -eq 5 ]] && exit 1
        done
        ./'$BUILD_DIR'/dynemit_simd_level_probe 0
        cd '$BUILD_DIR' && ctest --output-on-failure
        chown -R \"\$uid:\$gid\" /work/$BUILD_DIR
    "
