#!/usr/bin/env bash
# Phase 1/2: read-only Pantograph evidence collection.
# Writes evidence files to /Pantograph.ext/reports/evidence/.
# MUST NOT modify anything under /root/mycode/Pantograph.

set -euo pipefail

OUT="/Pantograph.ext/reports/evidence"
mkdir -p "${OUT}"

# 1. Inventory of source files
{
    echo "# Pantograph source inventory"
    echo
    echo "Generated: $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    echo
    echo "## Lean files (line counts)"
    echo
    find /root/mycode/Pantograph -name "*.lean" -type f -not -path "*/.lake/*" \
        -exec wc -l {} + | sort -n | tail -25
    echo
    echo "## Toolchain"
    cat /root/mycode/Pantograph/lean-toolchain
    echo
    echo "## Lake manifest summary"
    head -30 /root/mycode/Pantograph/lake-manifest.json
} > "${OUT}/01_inventory.md"

# 2. REPL command list extracted from Repl.lean dispatch table.
{
    echo "# Pantograph REPL command set"
    echo
    echo "## Command dispatch (Repl.lean)"
    grep -E '^\s*\| "' /root/mycode/Pantograph/Repl.lean | head -40
    echo
    echo "## Total commands:"
    grep -cE '^\s*\| "' /root/mycode/Pantograph/Repl.lean
} > "${OUT}/02_repl_commands.md"

# 3. Hash of source tree (for immutability proof)
{
    echo "# Source tree hash (immutability reference)"
    find /root/mycode/Pantograph -name "*.lean" -type f -not -path "*/.lake/*" \
        -exec md5sum {} + | sort | md5sum
    echo
    echo "Total .lean files: $(find /root/mycode/Pantograph -name '*.lean' -type f -not -path '*/.lake/*' | wc -l)"
} > "${OUT}/03_source_hash.txt"

echo "[evidence] written to ${OUT}"
ls -la "${OUT}"