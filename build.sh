#!/usr/bin/env bash
# Build script for the lean_physical_isolate C++ orchestration layer.
#
# This script:
#   1. Builds the Pantograph `repl` binary inside /root/mycode/Pantograph
#      (read-only source — we only run `lake build` there, never edit).
#   2. Configures & builds the C++ orchestration layer in this workspace.
#   3. Stages the resulting `repl` binary at ./repl so the manager can
#      execv() it directly.
#
# Run: bash build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PANT="/root/mycode/Pantograph"
REPL_SRC="$PANT/.lake/build/bin/repl"
REPL_DST="$ROOT/repl"

echo "==> build Pantograph repl (read-only source tree)"
( cd "$PANT" && lake build repl )

if [[ ! -x "$REPL_SRC" ]]; then
    echo "repl not produced at $REPL_SRC" >&2
    exit 1
fi
cp -f "$REPL_SRC" "$REPL_DST"
chmod +x "$REPL_DST"

echo "==> configure C++ orchestration"
mkdir -p "$ROOT/build"
( cd "$ROOT/build" && cmake "$ROOT/cmake" -DCMAKE_BUILD_TYPE=Release )

echo "==> build C++ orchestration"
cmake --build "$ROOT/build" -j"$(nproc)"

echo "==> build complete"
ls -la "$REPL_DST" "$ROOT/build/lpi_"* 2>/dev/null || true
