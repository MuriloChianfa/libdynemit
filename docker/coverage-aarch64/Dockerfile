# aarch64 coverage/test toolchain (QEMU user emulation) on top of libdynemit-base.
# Build via: ./scripts/ensure_docker_image_cache.sh coverage-aarch64
ARG BASE_IMAGE=libdynemit-base:24.04-arm64
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    rm -f /etc/apt/apt.conf.d/docker-clean \
    && apt-get update -qq \
    && apt-get install -y -qq --no-install-recommends \
        qemu-user

WORKDIR /work
