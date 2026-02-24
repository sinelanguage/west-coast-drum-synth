#pragma once

#include "pluginterfaces/vst/vsttypes.h"

#include <array>

namespace Steinberg::WestCoastDrumSynth {

constexpr int32 kLaneCount = 4;
constexpr int32 kPatternSteps = 16;
using PatternGrid = std::array<std::array<bool, kPatternSteps>, kLaneCount>;

enum ParameterId : Vst::ParamID {
  kParamMaster = 0,
  kParamInternalTempo,
  kParamSwing,
  kParamRun,
  kParamFollowTransport,
  kParamPresetSelect,
  kParamGlobalCount
};

enum LaneParamOffset : int32 {
  kLaneTune = 0,
  kLaneDecay,
  kLaneFold,
  kLaneFm,
  kLaneNoise,
  kLaneDrive,
  kLaneLevel,
  kLanePan,
  kLaneParamCount
};

constexpr Vst::ParamID kLaneParamBase = 100;
constexpr Vst::ParamID kMaxParamId = kLaneParamBase + (kLaneCount * kLaneParamCount) - 1;
constexpr int32 kParameterStateSize = kMaxParamId + 1;

inline constexpr Vst::ParamID laneParamID (int32 lane, LaneParamOffset offset)
{
  return kLaneParamBase + (lane * kLaneParamCount) + offset;
}

inline constexpr int32 laneFromParamID (Vst::ParamID paramId)
{
  if (paramId < kLaneParamBase || paramId > kMaxParamId)
    return -1;
  return (paramId - kLaneParamBase) / kLaneParamCount;
}

inline constexpr int32 laneOffsetFromParamID (Vst::ParamID paramId)
{
  if (paramId < kLaneParamBase || paramId > kMaxParamId)
    return -1;
  return (paramId - kLaneParamBase) % kLaneParamCount;
}

inline constexpr std::array<Vst::ParamID, kParamGlobalCount + (kLaneCount * kLaneParamCount)> allParameterIds ()
{
  std::array<Vst::ParamID, kParamGlobalCount + (kLaneCount * kLaneParamCount)> ids {};
  int32 index = 0;
  for (int32 i = 0; i < kParamGlobalCount; ++i)
    ids[index++] = static_cast<Vst::ParamID> (i);
  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 param = 0; param < kLaneParamCount; ++param)
      ids[index++] = laneParamID (lane, static_cast<LaneParamOffset> (param));
  }
  return ids;
}

} // namespace Steinberg::WestCoastDrumSynth
