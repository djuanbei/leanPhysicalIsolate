#!/usr/bin/env bash
# Validation suite: run all validation targets.

set -euo pipefail

EXT_DIR="/Pantograph.ext"
BUILD_DIR="${EXT_DIR}/builds"
REPORT_DIR="${EXT_DIR}/reports"

mkdir -p "${REPORT_DIR}"

ts() { date -u +"%Y-%m-%dT%H:%M:%SZ"; }

echo "[$(ts)] validation_start" >> "${REPORT_DIR}/validation.log"

# 1) Semantic equivalence
echo "[$(ts)] validate_semantic"
"${BUILD_DIR}/validate_semantic" \
    --repl "/root/mycode/Pantograph/.lake/build/bin/repl" || true

# 2) Isolation
echo "[$(ts)] validate_isolation"
"${BUILD_DIR}/validate_isolation" \
    --repl "/root/mycode/Pantograph/.lake/build/bin/repl" || true

# 3) Requirements scanning
echo "[$(ts)] validate_requirements"
"${BUILD_DIR}/validate_requirements" || true

# 4) Memory check (short)
echo "[$(ts)] memory_check"
"${BUILD_DIR}/memory_check" \
    --repl "/root/mycode/Pantograph/.lake/build/bin/repl" \
    --duration 10 \
    --interval 500 || true

echo "[$(ts)] validation_end" >> "${REPORT_DIR}/validation.log"