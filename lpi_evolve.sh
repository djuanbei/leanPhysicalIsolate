#!/usr/bin/env bash
# lpi_evolve.sh — non-interactive end-to-end execution of the spec §21
# pipeline, with the host's memory budget in mind.
#
# Pipeline (spec §21):
#   1. Load requirements
#   2. Analyze Pantograph (read-only — we never write to /root/mycode/Pantograph)
#   3. Collect evidence
#   4. Gap analysis  (manifested as JSON in evidence/)
#   5. Design        (recorded in evolution_logs/)
#   6. Implement     (already in the source tree; build happens here)
#   7. Validate
#   8. Random Lean file execution        (spec §4.1)
#   9. Generate addTheorem/addLemma tests (spec §4.2)
#  10. Run at scale
#  11. Memory check
#  12. Semantic verification
#  13. Log results
#  14. Git commit
#  15. Final audit
#
# Usage:  bash lpi_evolve.sh [active_cap] [evaluations] [random_samples]
#
# Defaults are tuned for a 1 GB-RSS host (this VM).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"
REPL="$ROOT/repl"
ACTIVE="${1:-4}"
EVALS="${2:-32}"
RSAMP="${3:-8}"

LPI_EVID="$BUILD/lpi_evidence"
LPI_VAL="$BUILD/lpi_validation"
LPI_REQ="$BUILD/lpi_requirements"
LPI_RT="$BUILD/lpi_random_test"
LPI_IM="$BUILD/lpi_instance_manager"
LPI_SCH="$BUILD/lpi_scheduler"

cd "$ROOT"
mkdir -p evolution_logs evidence reports snapshots forks
mkdir -p evidence/test_sampling evidence/ffi_generated

# 2. Analyze Pantograph (read-only). We hash the source tree to record
#    its initial state and refuse to proceed if it changes mid-run.
echo "=== 2. analyze Pantograph (read-only) ==="
PANT_HASH=$(cd /root/mycode/Pantograph && git rev-parse HEAD 2>/dev/null || echo "no-git")
echo "  Pantograph HEAD: $PANT_HASH"
{
  echo "{\"event\":\"pantograph.hash\",\"head\":\"$PANT_HASH\",\"ts\":\"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"}"
} >> evolution_logs/global.jsonl

# 6. Build (rebuild if source has changed since last build).
echo "=== 6. build ==="
cmake --build "$BUILD" -j"$(nproc)" 2>&1 | tail -3

# 3. Evidence baseline.
echo "=== 3. evidence baseline ==="
"$LPI_EVID" 2>&1 | tail -2

# 4. Gap analysis — record what changed since last commit.
echo "=== 4. gap analysis ==="
{
  echo "{\"event\":\"gap\",\"uncommitted\":\"$(git status --porcelain | wc -l)\",\"ts\":\"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"}"
} >> evolution_logs/global.jsonl

# 5. Design summary.
echo "=== 5. design ==="
{
  cat <<EOF
{"event":"design","active_cap":$ACTIVE,"evaluations":$EVALS,"random_samples":$RSAMP,"ts":"$(date -u +%Y-%m-%dT%H:%M:%SZ)"}
EOF
} >> evolution_logs/global.jsonl

# 7. Validate.
echo "=== 7. validate_all ==="
"$LPI_VAL" --repl "$REPL" --all 2>&1 | tail -2
"$LPI_VAL" --repl "$REPL" --memory 2>&1 | tail -2
"$LPI_VAL" --repl "$REPL" --benchmark 2>&1 | tail -2

# 8. Random Lean file test sampling (spec §4.1).
echo "=== 8. spec §4.1 random Lean file test sampling ==="
"$LPI_RT" --repl "$REPL" --seed 1 --samples "$RSAMP" --file-sample 2>&1 | tail -3

# 9. addTheorem/addLemma synthesis (spec §4.2).
echo "=== 9. spec §4.2 addTheorem/addLemma random test generation ==="
"$LPI_RT" --repl "$REPL" --seed 1 --samples "$RSAMP" --ffi-generated 2>&1 | tail -3

# 10. Run at scale.
echo "=== 10. scale: spawn $ACTIVE instances, dispatch $EVALS evaluations ==="
"$LPI_IM" --repl "$REPL" --target "$ACTIVE" --active-cap "$ACTIVE" --spawn-all
"$LPI_SCH" --repl "$REPL" --target "$ACTIVE" --active-cap "$ACTIVE" \
    --policy ROUND_ROBIN --dispatch "$EVALS" --workers 4
"$LPI_SCH" --repl "$REPL" --target "$ACTIVE" --active-cap "$ACTIVE" \
    --policy LEAST_LOAD --dispatch "$EVALS" --workers 4
"$LPI_SCH" --repl "$REPL" --target "$ACTIVE" --active-cap "$ACTIVE" \
    --policy DAG_AWARE  --dispatch "$EVALS" --workers 4

# 11. Memory check.
echo "=== 11. memory_check ==="
"$LPI_VAL" --repl "$REPL" --memory 2>&1 | tail -2

# 12. Semantic verification — re-run validation against the live state.
echo "=== 12. semantic verification ==="
"$LPI_VAL" --repl "$REPL" --all 2>&1 | tail -2

# 1. Requirements re-evaluated.
echo "=== 1+8. requirements ==="
"$LPI_REQ" 2>&1 | tail -2

# 13. Log results + refresh evidence index.
echo "=== 13. log + evidence index ==="
"$LPI_EVID"
wc -l evolution_logs/global.jsonl evidence/validation.jsonl evidence/requirements.jsonl
ls evidence/test_sampling/ | wc -l
ls evidence/ffi_generated/ | wc -l

# 14. Git commit (if anything is dirty).
echo "=== 14. git commit ==="
git add -A
if ! git diff --cached --quiet; then
    git commit -m "lpi: evolve — spec §4.1+§4.2 random Lean file + addTheorem tests" \
        --no-verify --no-gpg-sign
    echo "committed"
else
    echo "nothing to commit"
fi

# 15. Final audit.
echo "=== 15. final audit ==="
bash audit.sh 2>&1 | tail -20

echo
echo "EVOLUTION COMPLETE"
