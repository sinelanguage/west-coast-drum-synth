#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build_release}"
PLUGIN_PATH="${1:-${BUILD_DIR}/VST3/Release/WestCoastDrumSynth.vst3}"
EDITORHOST_BIN="${BUILD_DIR}/bin/Release/editorhost"

echo "Building plugin + editorhost in: ${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --target WestCoastDrumSynth editorhost -j"$(nproc)"

if [[ ! -x "${EDITORHOST_BIN}" ]]; then
  echo "error: editorhost binary not found at ${EDITORHOST_BIN}" >&2
  exit 1
fi

if [[ ! -d "${PLUGIN_PATH}" ]]; then
  echo "error: plugin bundle not found at ${PLUGIN_PATH}" >&2
  exit 1
fi

echo "Launching editorhost with plugin: ${PLUGIN_PATH}"
exec "${EDITORHOST_BIN}" "${PLUGIN_PATH}"
