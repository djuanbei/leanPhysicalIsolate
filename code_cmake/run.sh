#!/usr/bin/env bash
# Run the configured evolution scenario.
# Args (optional, with defaults):
#   --logical N    logical instance count (default 10000)
#   --per N        tasks per logical instance (default 10)
#   --policy P     ROUND_ROBIN | LEAST_LOAD | AFFINITY | DAG_AWARE
#   --duration S   wall-clock budget in seconds (default 18000 = 5h)
#
# Resources on this host: 2 CPUs, ~1GB RAM. Physical concurrency is
# auto-derived by compute_physical_concurrency() in instance_manager.cpp.

set -euo pipefail

EXT_DIR="/Pantograph.ext"
BUILD_DIR="${EXT_DIR}/builds"
LOG_DIR="${EXT_DIR}/evolution_logs"
REPORT_DIR="${EXT_DIR}/reports"

mkdir -p "${LOG_DIR}" "${REPORT_DIR}"

ts() { date -u +"%Y-%m-%dT%H:%M:%SZ"; }

echo "[$(ts)] run_start logical=${1:-10000} per=${2:-10} policy=${3:-ROUND_ROBIN}" >> "${LOG_DIR}/run.log"

# Compose args (default to small scenario for time-bounded run)
LOGICAL="${1:-10000}"
PER_INSTANCE="${2:-10}"
POLICY="${3:-ROUND_ROBIN}"
DURATION="${4:-14400}"

# Wall-clock budget: run the orchestrator with bounded logical count.
# The orchestrator will complete in finite time; we wrap with `timeout`.
timeout --kill-after=60 "${DURATION}" \
    "${BUILD_DIR}/spawn_10000_instances" \
    --logical "${LOGICAL}" \
    --per-instance "${PER_INSTANCE}" \
    --policy "${POLICY}" \
    --repl "/root/mycode/Pantograph/.lake/build/bin/repl" \
    --samples-dir "${EXT_DIR}/generated/lean_samples" \
    --log-dir "${LOG_DIR}" \
    --report-dir "${REPORT_DIR}" \
    || echo "[$(ts)] run finished/exited rc=$?" >> "${LOG_DIR}/run.log"

echo "[$(ts)] run_end" >> "${LOG_DIR}/run.log"