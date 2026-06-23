#!/usr/bin/env bash
# Integration test: spawn N isolated instances, dispatch evaluations,
# verify isolation, memory, snapshot, and report. Non-interactive.
#
# Usage: bash run_pipeline.sh [active_cap] [evaluations]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"
REPL="$ROOT/repl"
ACTIVE="${1:-16}"
EVALS="${2:-16}"

if [[ ! -x "$REPL" ]]; then
    # If ./repl doesn't exist (or is stale), fall back to building a mock
    # so the pipeline can be smoke-tested without a full Pantograph build.
    if [[ -f "$ROOT/src/mock_repl.cpp" ]]; then
        echo "repl not found; building mock repl from src/mock_repl.cpp"
        g++ -O2 -std=c++17 "$ROOT/src/mock_repl.cpp" -o "$ROOT/repl"
    else
        echo "repl not found at $REPL — run bash build.sh first" >&2
        exit 1
    fi
fi

cd "$ROOT"
mkdir -p evolution_logs evidence reports snapshots forks

echo "=== lpi: spawn $ACTIVE instances ==="
"$BUILD/lpi_instance_manager" --repl "$ROOT/repl" --target "$ACTIVE" --active-cap "$ACTIVE" --spawn-all

echo
echo "=== lpi: dispatch $EVALS evaluations (ROUND_ROBIN) ==="
"$BUILD/lpi_scheduler" --repl "$ROOT/repl" --target "$ACTIVE" --active-cap "$ACTIVE" \
    --policy ROUND_ROBIN --dispatch "$EVALS" --workers 4

echo
echo "=== lpi: validate ==="
"$BUILD/lpi_validation" --repl "$ROOT/repl" --all

echo
echo "=== lpi: memory + benchmark ==="
"$BUILD/lpi_validation" --repl "$ROOT/repl" --memory
"$BUILD/lpi_validation" --repl "$ROOT/repl" --benchmark

echo
echo "=== lpi: requirements evaluation ==="
"$BUILD/lpi_requirements"

echo
echo "=== lpi: evidence catalogue ==="
"$BUILD/lpi_evidence"
ls evidence/ | head -30
echo
echo "DONE"
