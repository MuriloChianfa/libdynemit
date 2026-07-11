#!/usr/bin/env bash
# Ensure cached local Docker toolchain images are built (and QEMU binfmt when needed).
#
# Usage: ./scripts/ensure_docker_image_cache.sh <image-key>
#
# Keys:
#   base-amd64          Shared Ubuntu toolchain (linux/amd64)
#   base-arm64          Shared Ubuntu toolchain (linux/arm64)
#   coverage-aarch64    aarch64 coverage/tests (+ QEMU binfmt); builds base-arm64 first
#   coverage-x86-sde    x86 + Intel SDE coverage; builds base-amd64 first
#
# Rebuilds when the image's Dockerfile (and inputs) change via content-hash label.
# Prints the image name on stdout; status messages go to stderr.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LABEL_KEY="libdynemit.dockerfile_sha"

usage() {
    cat <<'EOF' >&2
Usage: ./scripts/ensure_docker_image_cache.sh <image-key>

Keys: base-amd64 | base-arm64 | coverage-aarch64 | coverage-x86-sde
EOF
    exit 2
}

[[ $# -eq 1 ]] || usage
KEY="$1"

content_sha() {
    # Stable short hash over one or more files (paths relative to ROOT or absolute).
    local f
    {
        for f in "$@"; do
            sha256sum "$f"
        done
    } | sha256sum | awk '{print substr($1, 1, 12)}'
}

image_label() {
    local image="$1"
    docker image inspect -f "{{index .Config.Labels \"$LABEL_KEY\"}}" "$image" 2>/dev/null || true
}

ensure_built() {
    local image="$1"
    local platform="$2"
    local want_sha="$3"
    local dockerfile="$4"
    local context="$5"
    shift 5
    # remaining args: extra docker build args

    local have_sha
    if docker image inspect "$image" >/dev/null 2>&1; then
        have_sha="$(image_label "$image")"
        if [[ "$have_sha" == "$want_sha" ]]; then
            echo "Using cached image: $image (dockerfile_sha=$want_sha)" >&2
            return 0
        fi
        echo "Inputs changed (have=${have_sha:-none} want=$want_sha); rebuilding $image..." >&2
    else
        echo "Building $image ($platform, dockerfile_sha=$want_sha)..." >&2
    fi

    DOCKER_BUILDKIT=1 docker build --platform "$platform" \
        --label "${LABEL_KEY}=${want_sha}" \
        -t "$image" \
        -f "$dockerfile" \
        "$@" \
        "$context" >&2
}

ensure_qemu_binfmt() {
    local image="$1"
    if docker run --rm --platform linux/arm64 "$image" uname -m >/dev/null 2>&1; then
        return 0
    fi
    echo "Registering QEMU binfmt for linux/arm64 Docker..." >&2
    docker run --rm --privileged multiarch/qemu-user-static --reset -p yes >/dev/null
}

ensure_base_amd64() {
    local image="${LIBDYNEMIT_BASE_AMD64_IMAGE:-libdynemit-base:24.04-amd64}"
    local dir="$ROOT/docker/base"
    local want_sha
    want_sha="$(content_sha "$dir/Dockerfile")"
    ensure_built "$image" "linux/amd64" "$want_sha" "$dir/Dockerfile" "$dir"
    echo "$image"
}

ensure_base_arm64() {
    local image="${LIBDYNEMIT_BASE_ARM64_IMAGE:-libdynemit-base:24.04-arm64}"
    local dir="$ROOT/docker/base"
    local want_sha
    want_sha="$(content_sha "$dir/Dockerfile")"
    ensure_built "$image" "linux/arm64" "$want_sha" "$dir/Dockerfile" "$dir"
    echo "$image"
}

ensure_coverage_aarch64() {
    local base_image image dir want_sha
    base_image="$(ensure_base_arm64)"
    image="${LIBDYNEMIT_COVERAGE_AARCH64_IMAGE:-libdynemit-coverage-aarch64:24.04}"
    dir="$ROOT/docker/coverage-aarch64"
    want_sha="$(content_sha "$ROOT/docker/base/Dockerfile" "$dir/Dockerfile")"
    ensure_built "$image" "linux/arm64" "$want_sha" "$dir/Dockerfile" "$dir" \
        --build-arg "BASE_IMAGE=$base_image"
    ensure_qemu_binfmt "$image"
    echo "$image"
}

ensure_coverage_x86_sde() {
    local base_image image dir want_sha
    base_image="$(ensure_base_amd64)"
    image="${LIBDYNEMIT_COVERAGE_X86_SDE_IMAGE:-libdynemit-coverage-x86-sde:24.04}"
    dir="$ROOT/docker/coverage-x86-sde"
    want_sha="$(content_sha \
        "$ROOT/docker/base/Dockerfile" \
        "$dir/Dockerfile" \
        "$ROOT/scripts/install_intel_sde.sh")"
    ensure_built "$image" "linux/amd64" "$want_sha" "$dir/Dockerfile" "$dir" \
        --build-arg "BASE_IMAGE=$base_image" \
        --build-context "scripts=$ROOT/scripts"
    echo "$image"
}

case "$KEY" in
    base-amd64) ensure_base_amd64 ;;
    base-arm64) ensure_base_arm64 ;;
    coverage-aarch64) ensure_coverage_aarch64 ;;
    coverage-x86-sde) ensure_coverage_x86_sde ;;
    *)
        echo "Unknown image key: $KEY" >&2
        usage
        ;;
esac
