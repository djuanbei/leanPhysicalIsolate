#!/usr/bin/env bash
# Final audit (spec §21 step 13). Non-interactive.
#
#   1. Verify Pantograph is unmodified
#   2. Verify all evidence/log files exist
#   3. Print summary
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PANT="/root/mycode/Pantograph"
cd "$ROOT"

echo "=== 1. Pantograph invariant ==="
PANT_HASH=$(cd "$PANT" && git rev-parse HEAD 2>/dev/null || echo "no-git")
echo "  Pantograph HEAD: $PANT_HASH"
echo "  Files modified in last 60 minutes (should be 0):"
find "$PANT" -mmin -60 -type f ! -path "*/.lake/*" 2>/dev/null | wc -l

echo
echo "=== 2. Evidence + log presence ==="
for d in evidence evolution_logs snapshots forks runtime; do
    if [[ -d "$d" ]]; then
        n=$(find "$d" -type f | wc -l)
        echo "  $d/: $n files"
    else
        echo "  $d/: MISSING"
    fi
done

echo
echo "=== 3. Summary ==="
if [[ -f evidence/INDEX.json ]]; then
    python3 -c "import json; d=json.load(open('evidence/INDEX.json')); print(f'  evidence files: {len(d[\"files\"])}')" 2>/dev/null || true
fi
if [[ -f evidence/validation.jsonl ]]; then
    echo "  validation rows: $(wc -l < evidence/validation.jsonl)"
fi
if [[ -f evidence/requirements.jsonl ]]; then
    echo "  requirements rows: $(wc -l < evidence/requirements.jsonl)"
fi
if [[ -f evolution_logs/global.jsonl ]]; then
    echo "  evolution events: $(wc -l < evolution_logs/global.jsonl)"
fi

echo
echo "AUDIT DONE"
