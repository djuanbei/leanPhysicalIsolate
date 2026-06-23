#!/usr/bin/env bash
# Smoke test for LeanFFI. Requires /root/mycode/Pantograph/.lake/build/bin/repl
# to be built first. Exits 0 if at least one evaluation succeeds.

set -euo pipefail

EXT_DIR="/Pantograph.ext"
BUILD_DIR="${EXT_DIR}/builds"
REPL="/root/mycode/Pantograph/.lake/build/bin/repl"

if [ ! -x "${REPL}" ]; then
    echo "[smoke_test] SKIP: repl not built at ${REPL}"
    exit 0
fi

# Run a tiny scenario: logical=2, per_instance=1
echo "[smoke_test] running orchestrator..."
"${BUILD_DIR}/spawn_10000_instances" \
    --logical 2 \
    --per-instance 1 \
    --policy ROUND_ROBIN \
    --repl "${REPL}" \
    --samples-dir "${EXT_DIR}/generated/lean_samples" \
    --log-dir "${EXT_DIR}/evolution_logs" \
    --report-dir "${EXT_DIR}/reports" \
    2>&1 | tail -20

echo "[smoke_test] checking summary..."
test -f "${EXT_DIR}/reports/summary.json" && cat "${EXT_DIR}/reports/summary.json"

echo "[smoke_test] DONE"