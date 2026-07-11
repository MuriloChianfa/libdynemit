# Pre-built toolchain for local aarch64 coverage matrix legs (QEMU user emulation).
# Build once: ./scripts/ensure_docker_coverage_aarch64.sh
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq \
    && apt-get install -y -qq --no-install-recommends \
        ca-certificates \
        cmake \
        gcc \
        g++ \
        make \
        git \
        lcov \
        qemu-user \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
