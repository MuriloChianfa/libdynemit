# Shared Ubuntu 24.04 toolchain for local coverage/test images.
# Built per-platform as libdynemit-base:24.04-amd64 / libdynemit-base:24.04-arm64.
# Build via: ./scripts/ensure_docker_image_cache.sh base-amd64|base-arm64
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Cache mounts keep apt indexes/packages across rebuilds; docker-clean would wipe them.
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean \
    && apt update -qq \
    && apt install -y -qq --no-install-recommends \
        ca-certificates \
        cmake \
        gcc \
        g++ \
        make \
        git \
        lcov

WORKDIR /work
