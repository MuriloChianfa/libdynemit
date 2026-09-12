#!/usr/bin/env bash
# Install Intel SDE (Software Development Emulator) for x86 AVX-512 coverage.
# Fails loudly when the pinned release cannot be downloaded or unpacked.
#
# Intel's product page now advertises the latest SDE (10.13.x). The 10.8.0
# tarball stays at a versioned downloadmirror ID; a public redistribution of
# the same file is used if Intel's page or CDN is unreachable. SHA256 is
# pinned either way.
#
# Usage: ./scripts/install_intel_sde.sh [INSTALL_DIR]
# Sets SDE_BIN in GITHUB_ENV when run in GitHub Actions, otherwise prints export hint.

set -euo pipefail

INSTALL_DIR="${1:-/opt/intel-sde}"

# Pinned release: AVX-512F + AVX-512-VBMI2 via sde64 -spr
SDE_VERSION="10.8.0"
SDE_TARBALL="sde-external-10.8.0-2026-03-15-lin.tar.xz"
# SHA256 from https://www.intel.com/content/www/us/en/download/684897/915934/...
SDE_SHA256="50B320CD226ACEF7A491F5B321FC1BE3C3C7984F9E27A456E64894B5B0979DD3"
SDE_URLS=(
    "https://downloadmirror.intel.com/915934/${SDE_TARBALL}"
    "https://ci-mirrors.rust-lang.org/${SDE_TARBALL}"
)

if [[ "$(uname -m)" != "x86_64" ]]; then
    echo "Error: Intel SDE install is only supported on x86_64 hosts" >&2
    exit 2
fi

TMPDIR="${TMPDIR:-/tmp}"
ARCHIVE="${TMPDIR}/${SDE_TARBALL}"

download() {
    curl -fsSL --retry 5 --retry-delay 2 --retry-all-errors "$1" -o "$2"
}

got=0
for url in "${SDE_URLS[@]}"; do
    echo "Downloading Intel SDE ${SDE_VERSION} from ${url}..."
    if ! download "$url" "$ARCHIVE"; then
        echo "Download failed from ${url}" >&2
        continue
    fi

    ACTUAL_SHA256="$(sha256sum "$ARCHIVE" | awk '{print toupper($1)}')"
    if [[ "$ACTUAL_SHA256" != "$SDE_SHA256" ]]; then
        echo "Error: SDE tarball SHA256 mismatch from ${url} (expected ${SDE_SHA256}, got ${ACTUAL_SHA256})" >&2
        rm -f "$ARCHIVE"
        continue
    fi

    got=1
    break
done

if [[ "${got}" -ne 1 ]]; then
    echo "Error: could not download a SHA256-matching Intel SDE ${SDE_VERSION} tarball" >&2
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
