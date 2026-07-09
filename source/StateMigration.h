#pragma once

#include "ParameterIds.h"

#include "base/source/fstreamer.h"

#include <array>

namespace Steinberg::WestCoastDrumSynth {

constexpr uint32 kStateVersion = 8;
constexpr uint32 kV7StateVersion = 7;
constexpr uint32 kV6StateVersion = 6;
constexpr uint32 kV5StateVersion = 5;
constexpr uint32 kV4StateVersion = 4;
constexpr uint32 kV3StateVersion = 3;
constexpr int32 kV4LaneCount = 5;
constexpr uint32 kPreviousStateVersion = 2;
constexpr uint32 kLegacyStateVersion = 1;
constexpr int32 kLegacyLaneCount = 4;
constexpr int32 kPreviousGlobalParamCount = 6;

inline bool readV7StreamIntoDenseByParamId (IBStreamer& streamer, std::array<double, 709>& out)
{
  out.fill (0.0);
  const auto idsV7 = allParameterIdsV7 ();
  for (int32 i = 0; i < kV7TotalParameterCount; ++i)
  {
    double v = 0.0;
    if (!streamer.readDouble (v))
      return false;
    out[static_cast<size_t> (idsV7[static_cast<size_t> (i)])] = v;
  }
  return true;
}

} // namespace Steinberg::WestCoastDrumSynth
