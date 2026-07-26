#!/bin/bash
# Run benchmarks on a remote aarch64 machine via SSH.
#
# This script cross-compiles static ARM binaries, uploads them to the remote
# host, runs benchmarks there, and downloads the resulting CSVs.
#
# Usage:
#   ./scripts/run_remote_benchmark.sh --host HOST --user USER --key KEY [OPTIONS]
#
# Required:
#   --host HOST        Remote machine hostname or IP
#   --user USER        SSH username
#   --key  KEY         Path to SSH private key
#
# Options:
#   --cpu CORE         Pin to this CPU core on remote (default: 0)
#   --skip-build       Reuse existing dynemit-bench-aarch64.tar.gz
#   --features FEATS   Comma-separated feature list (default: all)
#   --levels LEVELS    Comma-separated SIMD levels (default: scalar,neon,sve,sve2)

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

SSH_HOST=""
SSH_USER=""
SSH_KEY=""
PIN_CPU="0"
SKIP_BUILD=0
FEATURES_STR=""
LEVELS_STR="scalar,neon,sve,sve2"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --host)       SSH_HOST="$2"; shift 2 ;;
        --user)       SSH_USER="$2"; shift 2 ;;
        --key)        SSH_KEY="$2"; shift 2 ;;
        --cpu)        PIN_CPU="$2"; shift 2 ;;
        --skip-build) SKIP_BUILD=1; shift ;;
        --features)   FEATURES_STR="$2"; shift 2 ;;
        --levels)     LEVELS_STR="$2"; shift 2 ;;
        -h|--help)
            echo "Usage: $0 --host HOST --user USER --key KEY [OPTIONS]"
            echo ""
            echo "Required:"
            echo "  --host HOST        Remote hostname/IP"
            echo "  --user USER        SSH username"
            echo "  --key  KEY         SSH private key path"
            echo ""
            echo "Options:"
            echo "  --cpu CORE         Pin to CPU core on remote (default: 0)"
            echo "  --skip-build       Reuse existing aarch64 bundle"
            echo "  --features FEATS   Comma-separated features (default: all)"
            echo "  --levels LEVELS    Comma-separated SIMD levels (default: scalar,neon,sve,sve2)"
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# Validate required args
if [[ -z "$SSH_HOST" || -z "$SSH_USER" || -z "$SSH_KEY" ]]; then
    echo -e "${RED}Error: --host, --user, and --key are all required.${NC}"
    echo "Run with --help for usage."
    exit 1
fi

if [[ ! -f "$SSH_KEY" ]]; then
    echo -e "${RED}Error: SSH key not found: ${SSH_KEY}${NC}"
    exit 1
fi

SSH_OPTS="-i $SSH_KEY -o StrictHostKeyChecking=no -o ConnectTimeout=10"
SSH_CMD="ssh $SSH_OPTS ${SSH_USER}@${SSH_HOST}"
SCP_CMD="scp $SSH_OPTS"

BUNDLE="dynemit-bench"
TARBALL="${BUNDLE}-aarch64.tar.gz"
REMOTE_DIR="/tmp/${BUNDLE}"

echo ""
echo -e "${BOLD}${BLUE}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${BLUE}║    libdynemit : Remote ARM Benchmark Runner      ║${NC}"
echo -e "${BOLD}${BLUE}╚══════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "  Remote:       ${GREEN}${SSH_USER}@${SSH_HOST}${NC}"
echo -e "  SSH key:      ${GREEN}${SSH_KEY}${NC}"
echo -e "  CPU core:     ${GREEN}${PIN_CPU}${NC}"
echo -e "  SIMD levels:  ${GREEN}${LEVELS_STR}${NC}"
if [[ -n "$FEATURES_STR" ]]; then
    echo -e "  Features:     ${GREEN}${FEATURES_STR}${NC}"
else
    echo -e "  Features:     ${GREEN}all${NC}"
fi
echo ""

if [[ "$SKIP_BUILD" -eq 0 ]]; then
    echo -e "${CYAN}[1/5] Building aarch64 static bundle...${NC}"
    bash scripts/bundle_benchmarks.sh --arch aarch64
    echo ""
else
    if [[ ! -f "$TARBALL" ]]; then
        echo -e "${RED}Error: --skip-build specified but ${TARBALL} not found.${NC}"
        exit 1
    fi
    echo -e "${CYAN}[1/5] Skipping build (using existing ${TARBALL})${NC}"
    echo ""
fi

echo -e "${CYAN}[2/5] Checking remote host...${NC}"
REMOTE_ARCH=$($SSH_CMD 'uname -m' 2>/dev/null)
if [[ "$REMOTE_ARCH" != "aarch64" ]]; then
    echo -e "${RED}Error: Remote is ${REMOTE_ARCH}, expected aarch64.${NC}"
    exit 1
fi
echo -e "  ${GREEN}Connected${NC} : ${REMOTE_ARCH}"
echo ""

echo -e "${CYAN}[3/5] Uploading bundle to remote...${NC}"
$SCP_CMD "$TARBALL" "${SSH_USER}@${SSH_HOST}:/tmp/" 2>/dev/null
$SSH_CMD "cd /tmp && rm -rf ${BUNDLE} && tar xzf ${TARBALL}" 2>/dev/null
echo -e "  ${GREEN}done${NC}"
echo ""

echo -e "${CYAN}[4/5] Running benchmarks on remote...${NC}"
echo ""

RUN_ARGS="--cpu ${PIN_CPU} --levels ${LEVELS_STR}"
if [[ -n "$FEATURES_STR" ]]; then
    RUN_ARGS="${RUN_ARGS} --features ${FEATURES_STR}"
fi

$SSH_CMD "cd ${REMOTE_DIR} && sudo bash run.sh ${RUN_ARGS}" 2>/dev/null

echo ""

echo -e "${CYAN}[5/5] Downloading benchmark results...${NC}"
mkdir -p bench/cpus

REMOTE_CPUS_DIR="${REMOTE_DIR}/bench/cpus"
REMOTE_CHECK=$($SSH_CMD "ls -d ${REMOTE_CPUS_DIR}/*/*/data/*.csv 2>/dev/null | head -1" 2>/dev/null || true)
if [[ -z "$REMOTE_CHECK" ]]; then
    echo -e "${RED}No CSV files found on remote.${NC}"
    exit 1
fi

$SCP_CMD -r "${SSH_USER}@${SSH_HOST}:${REMOTE_CPUS_DIR}" bench/ 2>/dev/null

CSV_COUNT=$($SSH_CMD "ls ${REMOTE_CPUS_DIR}/*/*/data/*.csv 2>/dev/null | wc -l" 2>/dev/null || echo "0")
echo -e "  ${GREEN}Downloaded ${CSV_COUNT} CSV files${NC} to bench/cpus/"

$SSH_CMD "rm -rf ${REMOTE_DIR} /tmp/${TARBALL}" 2>/dev/null || true

echo ""
echo -e "${BOLD}${GREEN}Remote benchmarks complete!${NC}"
echo ""
echo "Next steps:"
echo "  bash scripts/run_all_benchmarks.sh --charts-only"
echo ""
