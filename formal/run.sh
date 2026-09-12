#!/usr/bin/env bash
# SPDX-License-Identifier: BSL-1.0
# Run the one-off CBMC proofs. Not wired into CMake.
# CI: .github/workflows/formal.yml
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FORMAL_DIR="${ROOT}/formal"
OUT_DIR="${FORMAL_DIR}/out"
CBMC="${CBMC:-cbmc}"

if [[ -t 1 ]]; then
    C_OK=$'\033[32m'
    C_FAIL=$'\033[31m'
    C_DIM=$'\033[2m'
    C_BOLD=$'\033[1m'
    C_RESET=$'\033[0m'
else
    C_OK= C_FAIL= C_DIM= C_BOLD= C_RESET=
fi

if ! command -v "${CBMC}" >/dev/null 2>&1; then
    echo "error: cbmc not found (set CBMC= or install CBMC)" >&2
    echo "  sudo apt install cbmc" >&2
    echo "  or a release .deb from https://github.com/diffblue/cbmc/releases" >&2
    exit 1
fi

mkdir -p "${OUT_DIR}"

COMMON=(
    --c11
    --bounds-check
    --pointer-check
    --signed-overflow-check
    --div-by-zero-check
    --unwinding-assertions
)

MEM_INC=(-I "${ROOT}/src" -I "${FORMAL_DIR}")

failed=0
total_props=0
total_failed_props=0

# List proof_* assertions in source order. Count the rest as safety checks.
# Last line: STATS <failed> <total>
summarize_log() {
    awk -v ok="${C_OK}" -v failc="${C_FAIL}" -v dim="${C_DIM}" -v reset="${C_RESET}" '
        BEGIN { in_results = 0; safety = 0; failed_n = 0; total_n = 0; nfn = 0 }

        /^\*\* Results:/ { in_results = 1; next }
        !in_results { next }

        /^\*\* [0-9]+ of [0-9]+ failed/ {
            failed_n = $2 + 0
            total_n = $4 + 0
            next
        }

        / function / {
            fn = $NF
            next
        }

        $1 ~ /^\[/ && $2 == "line" {
            id = $1
            gsub(/[\[\]]/, "", id)
            lineno = $3 + 0

            rest = $0
            sub(/^\[[^]]+\] line [0-9]+ /, "", rest)
            status = rest
            sub(/.*: /, "", status)
            desc = rest
            sub(/: (SUCCESS|FAILURE)$/, "", desc)

            show = (status == "FAILURE" || (fn ~ /^proof_/ && id ~ /\.assertion\./))
            if (!show) {
                safety++
                next
            }

            if (!(fn in first_line)) {
                nfn++
                order[nfn] = fn
                first_line[fn] = lineno
            }
            n[fn]++
            lines[fn, n[fn]] = lineno
            descs[fn, n[fn]] = desc
            stats[fn, n[fn]] = status
            next
        }

        END {
            for (i = 1; i <= nfn; i++)
                for (j = i + 1; j <= nfn; j++)
                    if (first_line[order[j]] < first_line[order[i]]) {
                        tmp = order[i]
                        order[i] = order[j]
                        order[j] = tmp
                    }

            for (i = 1; i <= nfn; i++) {
                fn = order[i]
                printf "  %s\n", fn
                for (k = 1; k <= n[fn]; k++) {
                    if (stats[fn, k] == "SUCCESS")
                        printf "    %sok%s    L%-4s %s\n", ok, reset, lines[fn, k], descs[fn, k]
                    else
                        printf "    %sFAIL%s  L%-4s %s\n", failc, reset, lines[fn, k], descs[fn, k]
                }
            }
            if (safety > 0)
                printf "  %s+%d safety checks (bounds, pointer, overflow, unwind)%s\n",
                    dim, safety, reset
            printf "STATS %d %d\n", failed_n, total_n
        }
    ' "$1"
}

run_proof() {
    local name="$1"
    local src="$2"
    local note="$3"
    shift 3

    local log="${OUT_DIR}/${name}.log"
    local rel="${src#"${ROOT}/"}"

    echo
    echo "${C_BOLD}==> ${name}${C_RESET}  ${C_DIM}${rel}${C_RESET}"
    echo "    ${C_DIM}${note}${C_RESET}"

    local start=$SECONDS
    local ok=0
    if "${CBMC}" "$@" >"${log}" 2>&1; then
        if grep -q "VERIFICATION SUCCESSFUL" "${log}"; then
            ok=1
        fi
    fi
    local elapsed=$((SECONDS - start))

    local listing stats
    listing="$(summarize_log "${log}")"
    stats="$(printf '%s\n' "${listing}" | sed -n '/^STATS /p' | tail -n 1)"
    printf '%s\n' "${listing}" | sed '/^STATS /d'

    local props_failed=0
    local props_total=0
    if [[ -n "${stats}" ]]; then
        props_failed="${stats#STATS }"
        props_total="${props_failed#* }"
        props_failed="${props_failed%% *}"
    fi

    total_props=$((total_props + props_total))
    total_failed_props=$((total_failed_props + props_failed))

    if [[ "${ok}" -eq 1 ]]; then
        echo "    ${C_OK}VERIFICATION SUCCESSFUL${C_RESET}  ${C_DIM}${props_total} properties, ${elapsed}s${C_RESET}"
        return 0
    fi
    echo "    ${C_FAIL}VERIFICATION FAILED${C_RESET}  ${C_DIM}${props_failed}/${props_total} failed, ${elapsed}s  ${log}${C_RESET}"
    echo
    tail -n 40 "${log}" || true
    failed=1
}

echo "${C_BOLD}CBMC${C_RESET}  $("${CBMC}" --version | head -n 1)"
echo "${C_DIM}checks  bounds, pointer, signed-overflow, div-by-zero, unwinding${C_RESET}"
echo "${C_DIM}logs    ${OUT_DIR#"${ROOT}/"}/${C_RESET}"

echo
echo "${C_BOLD}Proofs${C_RESET}"
printf '  %-12s %s\n' "mem_align" "mem_align_up / mem_aligned_bytes / mem_aligned_count"
printf '  %-12s %s\n' ""          "no-wrap: >= size, 64-aligned, slack < 64"
printf '  %-12s %s\n' ""          "wrap of size+mask and count*elem_size characterized"
printf '  %-12s %s\n' "mem_copy"  "memsets / memcpys: NULL, RSIZE_MAX, success, count>destsz"
printf '  %-12s %s\n' ""          "destsz <= 16, unwind 17, unsigned-overflow on"

run_proof mem_align \
    "${FORMAL_DIR}/mem_align_proof.c" \
    "no loop bound; unsigned wrap is in-scope (no --unsigned-overflow-check)" \
    "${COMMON[@]}" \
    "${MEM_INC[@]}" \
    "${FORMAL_DIR}/mem_align_proof.c"

run_proof mem_copy \
    "${FORMAL_DIR}/mem_copy_proof.c" \
    "destsz <= 16, --unwind 17, --unsigned-overflow-check" \
    "${COMMON[@]}" \
    --unsigned-overflow-check \
    --unwind 17 \
    "${MEM_INC[@]}" \
    "${FORMAL_DIR}/mem_copy_proof.c"

echo
if [[ "${failed}" -ne 0 ]]; then
    echo "${C_FAIL}${total_failed_props} of ${total_props} properties failed${C_RESET}"
    exit 1
fi
echo "${C_OK}all proofs passed${C_RESET}  ${C_DIM}${total_props} properties${C_RESET}"
