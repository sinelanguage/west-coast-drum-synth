#!/usr/bin/env bash
# Build WestCoastDrumSynth and copy the bundle to the user VST3 folder.
# The plugin name in Bitwig includes "(Debug)" or "(Release)" unless you configure
# with -DWCDS_PUBLIC_PLUGIN_NAME=ON.
#
# Usage:
#   BUILD_TYPE=Debug ./scripts/install_vst3_mac.sh
#   BUILD_TYPE=Release ./scripts/install_vst3_mac.sh
#   BUILD_DIR=/path/to/build BUILD_TYPE=Release ./scripts/install_vst3_mac.sh
#   ./scripts/install_vst3_mac.sh -- -DWCDS_PUBLIC_PLUGIN_NAME=ON
#
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 8)}"

if [[ -z "${BUILD_DIR:-}" ]]; then
  if [[ "${BUILD_TYPE}" == "Release" ]]; then
    BUILD_DIR="${ROOT_DIR}/build_release"
  else
    BUILD_DIR="${ROOT_DIR}/build"
  fi
fi

echo "Configuring ${BUILD_TYPE} in ${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "$@"
cmake --build "${BUILD_DIR}" --target WestCoastDrumSynth -j"${JOBS}"

PLUGIN_SRC="${BUILD_DIR}/VST3/${BUILD_TYPE}/WestCoastDrumSynth.vst3"
DEST_DIR="${HOME}/Library/Audio/Plug-Ins/VST3"
DEST="${DEST_DIR}/WestCoastDrumSynth.vst3"

if [[ ! -d "${PLUGIN_SRC}" ]]; then
  echo "error: plugin bundle not found at ${PLUGIN_SRC}" >&2
  exit 1
fi

mkdir -p "${DEST_DIR}"
rm -rf "${DEST}"
cp -R "${PLUGIN_SRC}" "${DEST}"

echo "Installed: ${DEST}"
echo "Typical Bitwig names: Debug -> \"West Coast Drum Synth (Debug)\"; Release -> \"West Coast Drum Synth (Release)\"."
echo "For a neutral shipping name, configure with -DWCDS_PUBLIC_PLUGIN_NAME=ON (see CMakeLists.txt)."
echo "Restart Bitwig and rescan VST3 plug-ins if needed."
