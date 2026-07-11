#!/usr/bin/env bash
# Build (once) the cached Docker image used by run_coverage_matrix_local.sh aarch64 legs.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${LIBDYNEMIT_COVERAGE_AARCH64_IMAGE:-libdynemit-coverage-aarch64:24.04}"

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "Building $IMAGE (linux/arm64)..." >&2
    docker build --platform linux/arm64 \
        -t "$IMAGE" \
        -f "$ROOT/docker/coverage-aarch64/Dockerfile" \
        "$ROOT" >&2
else
    echo "Using cached image: $IMAGE" >&2
fi

echo "$IMAGE"
