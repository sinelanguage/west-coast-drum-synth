#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build_release}"
PLUGIN_PATH="${1:-${BUILD_DIR}/VST3/Release/WestCoastDrumSynth.vst3}"
EDITORHOST_BIN="${BUILD_DIR}/bin/Release/editorhost"

if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
  JOBS="$(sysctl -n hw.ncpu 2>/dev/null || echo 4)"
else
  JOBS="4"
fi

echo "Building plugin + editorhost in: ${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --target WestCoastDrumSynth editorhost -j"${JOBS}"

if [[ ! -x "${EDITORHOST_BIN}" && -x "${BUILD_DIR}/bin/editorhost.app/Contents/MacOS/editorhost" ]]; then
  EDITORHOST_BIN="${BUILD_DIR}/bin/editorhost.app/Contents/MacOS/editorhost"
fi
if [[ ! -x "${EDITORHOST_BIN}" && -x "${BUILD_DIR}/bin/Release/editorhost.app/Contents/MacOS/editorhost" ]]; then
  EDITORHOST_BIN="${BUILD_DIR}/bin/Release/editorhost.app/Contents/MacOS/editorhost"
fi

if [[ ! -x "${EDITORHOST_BIN}" ]]; then
  echo "error: editorhost binary not found at ${EDITORHOST_BIN}" >&2
  exit 1
fi

if [[ ! -d "${PLUGIN_PATH}" ]]; then
  echo "error: plugin bundle not found at ${PLUGIN_PATH}" >&2
  exit 1
fi

echo "Launching editorhost with plugin: ${PLUGIN_PATH}"
echo "(Release build: host lists this as \"West Coast Drum Synth (Release)\" unless WCDS_PUBLIC_PLUGIN_NAME is ON.)"
exec "${EDITORHOST_BIN}" "${PLUGIN_PATH}"
