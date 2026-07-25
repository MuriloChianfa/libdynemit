# x86 coverage toolchain with Intel SDE (AVX-512) on top of libdynemit-base.
# Build via: ./scripts/ensure_docker_image_cache.sh coverage-x86-sde
# SDE pins live in scripts/install_intel_sde.sh (mounted at build time).
ARG BASE_IMAGE=libdynemit-base:24.04-amd64
FROM ${BASE_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    --mount=type=bind,from=scripts,source=install_intel_sde.sh,target=/tmp/install_intel_sde.sh \
    rm -f /etc/apt/apt.conf.d/docker-clean \
    && apt update -qq \
    && apt install -y -qq --no-install-recommends \
        curl \
        xz-utils \
    && bash /tmp/install_intel_sde.sh /opt/intel-sde

ENV SDE_BIN=/opt/intel-sde/sde64
ENV PATH="/opt/intel-sde:${PATH}"

WORKDIR /work
