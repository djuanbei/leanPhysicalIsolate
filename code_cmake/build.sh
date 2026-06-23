#!/usr/bin/env bash
# Build script for LeanFFI evolution layer.
# Reads external CMakeLists, builds all targets into /Pantograph.ext/builds/.

set -euo pipefail

EXT_DIR="/Pantograph.ext"
BUILD_DIR="${EXT_DIR}/builds"
CMAKE_DIR="${EXT_DIR}/cmake"

mkdir -p "${BUILD_DIR}"

# Configure + build
cmake -S "${CMAKE_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPANTOGRAPH_REPL="/root/mycode/Pantograph/.lake/build/bin/repl"

cmake --build "${BUILD_DIR}" --parallel 2

echo "[build] done. executables in ${BUILD_DIR}:"
ls -1 "${BUILD_DIR}" | grep -v '\.' || true
ls -1 "${BUILD_DIR}" | head -30