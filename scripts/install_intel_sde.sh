#!/usr/bin/env bash
# Install Intel SDE (Software Development Emulator) for x86 AVX-512 coverage.
# Fails loudly when the pinned release cannot be downloaded or unpacked.
#
# The download URL is resolved from Intel's official download page (mirror IDs
# rotate). The tarball is verified against Intel's published SHA256 checksum.
#
# Usage: ./scripts/install_intel_sde.sh [INSTALL_DIR]
# Sets SDE_BIN in GITHUB_ENV when run in GitHub Actions, otherwise prints export hint.

set -euo pipefail

INSTALL_DIR="${1:-/opt/intel-sde}"

# Pinned release: AVX-512F + AVX-512-VBMI2 via sde64 -spr
SDE_VERSION="10.8.0"
SDE_TARBALL="sde-external-10.8.0-2026-03-15-lin.tar.xz"
SDE_PAGE="https://www.intel.com/content/www/us/en/download/684897/intel-software-development-emulator.html"
# SHA256 from https://www.intel.com/content/www/us/en/download/684897/...
SDE_SHA256="50B320CD226ACEF7A491F5B321FC1BE3C3C7984F9E27A456E64894B5B0979DD3"

if [[ "$(uname -m)" != "x86_64" ]]; then
    echo "Error: Intel SDE install is only supported on x86_64 hosts" >&2
    exit 2
fi

resolve_official_url() {
    local page_url="$1"
    local tarball="$2"
    local url

    url="$(curl -fsSL "$page_url" \
        | grep -oE "https://downloadmirror\\.intel\\.com/[0-9]+/${tarball//./\\.}" \
        | head -n1)"

    if [[ -z "$url" ]]; then
        url="$(curl -fsSL "$page_url" \
            | grep -oP '(?<=data-href=")(https://downloadmirror\.intel\.com/[^"]+'"${tarball//./\\.}"')' \
            | head -n1)"
    fi

    if [[ -z "$url" ]]; then
        echo "Error: could not resolve official SDE URL from ${page_url}" >&2
        return 1
    fi

    if [[ "$url" != https://downloadmirror.intel.com/* ]]; then
        echo "Error: resolved URL is not an Intel download mirror: ${url}" >&2
        return 1
    fi

    printf '%s\n' "$url"
}

TMPDIR="${TMPDIR:-/tmp}"
ARCHIVE="${TMPDIR}/${SDE_TARBALL}"

echo "Resolving Intel SDE ${SDE_VERSION} download URL..."
SDE_URL="$(resolve_official_url "$SDE_PAGE" "$SDE_TARBALL")"
echo "Downloading from ${SDE_URL}..."
if ! curl -fsSL "$SDE_URL" -o "$ARCHIVE"; then
    echo "Error: failed to download Intel SDE from ${SDE_URL}" >&2
    exit 1
fi

ACTUAL_SHA256="$(sha256sum "$ARCHIVE" | awk '{print toupper($1)}')"
if [[ "$ACTUAL_SHA256" != "$SDE_SHA256" ]]; then
    echo "Error: SDE tarball SHA256 mismatch (expected ${SDE_SHA256}, got ${ACTUAL_SHA256})" >&2
    exit 1
fi

rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR"
tar -xJf "$ARCHIVE" -C "$INSTALL_DIR" --strip-components=1

SDE_BIN="$(find "$INSTALL_DIR" -type f -name sde64 -print -quit)"
if [[ -z "$SDE_BIN" || ! -x "$SDE_BIN" ]]; then
    echo "Error: sde64 not found under ${INSTALL_DIR}" >&2
    exit 1
fi

echo "Intel SDE installed: ${SDE_BIN}"
"$SDE_BIN" -version || true

if [[ -n "${GITHUB_ENV:-}" ]]; then
    echo "SDE_BIN=${SDE_BIN}" >> "$GITHUB_ENV"
else
    echo "export SDE_BIN=${SDE_BIN}"
fi
