#!/usr/bin/env bash
# Run the CI coverage matrix locally and print line-coverage summaries.
#
# Usage: ./scripts/run_coverage_matrix_local.sh [--skip-sde] [--skip-aarch64]
#
# x86-native / x86-ts run on the host. x86-sde and aarch64 legs use cached Docker
# images when the daemon is available (see scripts/ensure_docker_image_cache.sh).
# Without Docker, x86-sde falls back to scripts/install_intel_sde.sh on the host.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

SKIP_SDE=0
SKIP_AARCH64=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-sde) SKIP_SDE=1; shift ;;
        --skip-aarch64) SKIP_AARCH64=1; shift ;;
        -h|--help)
            echo "Usage: $0 [--skip-sde] [--skip-aarch64]"
            exit 0
            ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done

HOST_ARCH="$(uname -m)"
AARCH64_ON_X86=0
if [[ "$HOST_ARCH" != "aarch64" && "$HOST_ARCH" != "arm64" ]]; then
    AARCH64_ON_X86=1
fi

summarize_coverage() {
    local info="$1"
    local label="$2"
    if [[ ! -f "$info" ]]; then
        echo "[$label] MISSING $info"
        return 1
    fi
    lcov --summary "$info" 2>/dev/null | awk -v l="$label" '
        /^  lines/ { print "[" l "] " $0 }
        /^  functions/ { print "[" l "] " $0 }
    '
}

run_x86_leg() {
    local name="$1"
    local build_dir="$2"
    shift 2
    local -a extra=("$@")

    echo ""
    echo "========== $name =========="
    cmake -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON "${extra[@]}"
    cmake --build "$build_dir" -j"$(nproc)"
    "$build_dir/dynemit_simd_level_probe" 0
    cmake --build "$build_dir" --target coverage
    summarize_coverage "$build_dir/coverage.info" "$name"
}

echo "Host: $(uname -m)"

# --- x86-native ---
run_x86_leg "x86-native" "build-cov-x86-native"

# --- x86-ts ---
run_x86_leg "x86-ts" "build-cov-x86-ts" -DDYNEMIT_TS=ON

# --- x86-sde ---
if [[ "$SKIP_SDE" -eq 0 ]]; then
    echo ""
    echo "========== x86-sde (Intel SDE) =========="
    if [[ "$HOST_ARCH" == "x86_64" ]] && docker info >/dev/null 2>&1; then
        SDE_IMAGE="$("$ROOT/scripts/ensure_docker_image_cache.sh" coverage-x86-sde)"
        echo "Using Docker image: $SDE_IMAGE"
        docker run --rm --platform linux/amd64 \
            -v "$ROOT:/work" -w /work \
            "$SDE_IMAGE" bash -euxo pipefail -c '
                uid='"$(id -u)"'
                gid='"$(id -g)"'
                chown -R "$uid:$gid" /work/build-cov-x86-sde 2>/dev/null || true
                rm -rf build-cov-x86-sde
                "$SDE_BIN" -spr -- ./build-cov-x86-native/dynemit_simd_level_probe 5
                cmake -B build-cov-x86-sde -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON \
                    "-DDYNEMIT_COVERAGE_TEST_WRAPPER=${SDE_BIN} -spr --"
                cmake --build build-cov-x86-sde -j"$(nproc)"
                cmake --build build-cov-x86-sde --target coverage
                chown -R "$uid:$gid" /work/build-cov-x86-sde
            '
        summarize_coverage "build-cov-x86-sde/coverage.info" "x86-sde"
    else
        SDE_ROOT="${SDE_ROOT:-/tmp/intel-sde}"
        if [[ ! -x "${SDE_BIN:-}" ]]; then
            if ! ./scripts/install_intel_sde.sh "$SDE_ROOT"; then
                echo "WARNING: SDE install failed; skipping x86-sde leg." >&2
                SDE_BIN=""
            else
                SDE_BIN="$(find "$SDE_ROOT" -type f -name sde64 -print -quit)"
            fi
        fi
        if [[ -n "${SDE_BIN:-}" && -x "$SDE_BIN" ]]; then
            export SDE_BIN
            echo "Using host SDE: $SDE_BIN"
            "$SDE_BIN" -spr -- ./build-cov-x86-native/dynemit_simd_level_probe 5

            cmake -B build-cov-x86-sde -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON \
                "-DDYNEMIT_COVERAGE_TEST_WRAPPER=${SDE_BIN} -spr --"
            cmake --build build-cov-x86-sde -j"$(nproc)"
            cmake --build build-cov-x86-sde --target coverage
            summarize_coverage "build-cov-x86-sde/coverage.info" "x86-sde"
        fi
    fi
else
    echo ""
    echo "========== x86-sde SKIPPED (--skip-sde) =========="
fi

# --- aarch64 via Docker (cached toolchain image, single container for both legs) ---
if [[ "$SKIP_AARCH64" -eq 0 ]] && docker info >/dev/null 2>&1; then
    AARCH64_IMAGE="$("$ROOT/scripts/ensure_docker_image_cache.sh" coverage-aarch64)"

    echo ""
    echo "========== aarch64-native + aarch64-sve (Docker $AARCH64_IMAGE) =========="
    AARCH64_CMAKE_EXTRA=(-DDYNEMIT_ENABLE_GTEST=OFF)
    docker run --rm --platform linux/arm64 \
        --ulimit stack=67108864:67108864 \
        -e AARCH64_ON_X86="$AARCH64_ON_X86" \
        -v "$ROOT:/work" -w /work \
        "$AARCH64_IMAGE" bash -euxo pipefail -c '
            uid='"$(id -u)"'
            gid='"$(id -g)"'
            if [[ "$AARCH64_ON_X86" == 1 ]]; then
                export DYNEMIT_MAX_SIMD_LEVEL=10
            fi
            for d in build-cov-aarch64-native build-cov-aarch64-sve; do
                chown -R "$uid:$gid" "/work/$d" 2>/dev/null || true
                rm -rf "$d"
            done

            cmake -B build-cov-aarch64-native -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON '"${AARCH64_CMAKE_EXTRA[*]}"'
            for attempt in 1 2 3 4 5; do
                cmake --build build-cov-aarch64-native -j1 && break
                echo "aarch64-native build attempt ${attempt} failed; retrying..." >&2
                [[ "$attempt" -eq 5 ]] && exit 1
            done
            ./build-cov-aarch64-native/dynemit_simd_level_probe 0
            cmake --build build-cov-aarch64-native --target coverage

            if [[ "$AARCH64_ON_X86" == 1 ]]; then
                echo "x86 host: SVE not runnable under binfmt; aarch64-sve leg reuses native NEON coverage."
                mkdir -p build-cov-aarch64-sve
                cp build-cov-aarch64-native/coverage.info build-cov-aarch64-sve/coverage.info
            else
                cmake -B build-cov-aarch64-sve -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON '"${AARCH64_CMAKE_EXTRA[*]}"'
                for attempt in 1 2 3 4 5; do
                    cmake --build build-cov-aarch64-sve -j1 && break
                    echo "aarch64-sve build attempt ${attempt} failed; retrying..." >&2
                    [[ "$attempt" -eq 5 ]] && exit 1
                done
                if ./build-cov-aarch64-sve/dynemit_simd_level_probe 12; then
                    echo "Native aarch64 host reports SVE2+; reusing native coverage artifact."
                    cp build-cov-aarch64-native/coverage.info build-cov-aarch64-sve/coverage.info
                else
                    qemu-aarch64 -cpu max,sve=on -- ./build-cov-aarch64-sve/dynemit_simd_level_probe 11
                    cmake -B build-cov-aarch64-sve -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON '"${AARCH64_CMAKE_EXTRA[*]}"' \
                        "-DDYNEMIT_COVERAGE_TEST_WRAPPER=qemu-aarch64 -cpu max,sve=on --"
                    for attempt in 1 2 3 4 5; do
                        cmake --build build-cov-aarch64-sve -j1 && break
                        echo "aarch64-sve rebuild attempt ${attempt} failed; retrying..." >&2
                        [[ "$attempt" -eq 5 ]] && exit 1
                    done
                    cmake --build build-cov-aarch64-sve --target coverage
                fi
            fi

            chown -R "$uid:$gid" /work/build-cov-aarch64-native /work/build-cov-aarch64-sve
        '
    summarize_coverage "build-cov-aarch64-native/coverage.info" "aarch64-native"
    summarize_coverage "build-cov-aarch64-sve/coverage.info" "aarch64-sve"
else
    echo ""
    if [[ "$SKIP_AARCH64" -eq 1 ]]; then
        echo "========== aarch64 legs SKIPPED (--skip-aarch64) =========="
    else
        echo "========== aarch64 legs SKIPPED (Docker daemon not running) =========="
    fi
fi

echo ""
echo "========== Emulator harness smoke test (ctest path) =========="
./scripts/run_tests_under_emu.sh --build-dir build-cov-x86-native 2>&1 | tail -3

echo ""
echo "========== Merged coverage (union across all matrix legs) =========="
MERGE_INPUTS=()
MERGE_LABELS=()
for spec in \
    "x86-native:build-cov-x86-native/coverage.info" \
    "x86-ts:build-cov-x86-ts/coverage.info" \
    "x86-sde:build-cov-x86-sde/coverage.info" \
    "aarch64-native:build-cov-aarch64-native/coverage.info" \
    "aarch64-sve:build-cov-aarch64-sve/coverage.info"
do
    label="${spec%%:*}"
    info="${spec#*:}"
    if [[ -f "$info" ]]; then
        fixed="/tmp/coverage-${label}.fixed.info"
        sed "s|SF:/work|SF:${ROOT}|g" "$info" > "$fixed"
        MERGE_INPUTS+=("$fixed")
        MERGE_LABELS+=("$label")
        summarize_coverage "$fixed" "$label"
    else
        echo "[$label] skip merge (missing $info)"
    fi
done

if [[ ${#MERGE_INPUTS[@]} -eq 0 ]]; then
    echo "No coverage.info files to merge." >&2
    exit 1
fi

mkdir -p build-cov-merged
python3 - "$ROOT" "${MERGE_INPUTS[@]}" <<'PY'
import sys
from collections import defaultdict

root = sys.argv[1]
inputs = sys.argv[2:]
union = defaultdict(dict)

def parse_info(path):
    recs = defaultdict(dict)
    cur = None
    with open(path) as fh:
        for line in fh:
            line = line.rstrip("\n")
            if line.startswith("SF:"):
                cur = line[3:]
            elif cur and line.startswith("DA:"):
                ln, hit = line[3:].split(",")
                recs[cur][int(ln)] = max(recs[cur].get(int(ln), 0), int(hit))
            elif line == "end_of_record":
                cur = None
    return recs

for path in inputs:
    for sf, das in parse_info(path).items():
        for ln, hit in das.items():
            union[sf][ln] = max(union[sf].get(ln, 0), hit)

out = f"{root}/build-cov-merged/coverage_union.info"
with open(out, "w") as fh:
    for sf in sorted(union):
        fh.write(f"SF:{sf}\n")
        for ln in sorted(union[sf]):
            fh.write(f"DA:{ln},{union[sf][ln]}\n")
        fh.write("end_of_record\n")

feat_h = feat_t = 0
all_h = all_t = 0
for sf, das in union.items():
    h = sum(1 for v in das.values() if v > 0)
    t = len(das)
    all_h += h
    all_t += t
    if "/features/" in sf and "/tests/" not in sf and "/benchmarks/" not in sf:
        feat_h += h
        feat_t += t

print(f"UNION features/ implementations: {100 * feat_h / feat_t:.2f}% ({feat_h}/{feat_t} lines)")
print(f"UNION all tracked sources:       {100 * all_h / all_t:.2f}% ({all_h}/{all_t} lines, {len(union)} files)")
print(f"Union report: {out}")
PY

LCOV_IGNORE=(--ignore-errors inconsistent,inconsistent
               --ignore-errors mismatch,mismatch
               --ignore-errors negative,negative
               --ignore-errors unused,unused)
genhtml build-cov-merged/coverage_union.info \
    --output-directory build-cov-merged/coverage_report \
    --branch-coverage "${LCOV_IGNORE[@]}"
echo "HTML report: build-cov-merged/coverage_report/index.html"

echo ""
echo "All local matrix legs finished."
