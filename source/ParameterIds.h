#pragma once

#include "pluginterfaces/vst/vsttypes.h"

#include <array>

namespace Steinberg::WestCoastDrumSynth {

constexpr int32 kLaneCount = 5;
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

enum LaneExtraParamOffset : int32 {
  kLanePitchEnvAmount = 0,
  kLanePitchEnvDecay,
  kLaneTransientAttack,
  kLaneNoiseTone,
  kLaneNoiseDecay,
  kLaneSnap,
  kLaneExtraParamCount
};

constexpr Vst::ParamID kLaneParamBase = 100;
constexpr Vst::ParamID kLaneExtraParamBase = 200;
constexpr Vst::ParamID kLaneCoreMaxParamId = kLaneParamBase + (kLaneCount * kLaneParamCount) - 1;
constexpr Vst::ParamID kLaneExtraMaxParamId = kLaneExtraParamBase + (kLaneCount * kLaneExtraParamCount) - 1;
constexpr Vst::ParamID kMaxParamId = (kLaneCoreMaxParamId > kLaneExtraMaxParamId) ? kLaneCoreMaxParamId :
                                                                                       kLaneExtraMaxParamId;
constexpr int32 kParameterStateSize = kMaxParamId + 1;
constexpr int32 kTotalParameterCount =
  kParamGlobalCount + (kLaneCount * kLaneParamCount) + (kLaneCount * kLaneExtraParamCount);

inline constexpr Vst::ParamID laneParamID (int32 lane, LaneParamOffset offset)
{
  return kLaneParamBase + (lane * kLaneParamCount) + offset;
}

inline constexpr Vst::ParamID laneExtraParamID (int32 lane, LaneExtraParamOffset offset)
{
  return kLaneExtraParamBase + (lane * kLaneExtraParamCount) + offset;
}

inline constexpr int32 laneFromParamID (Vst::ParamID paramId)
{
  if (paramId < kLaneParamBase || paramId > kLaneCoreMaxParamId)
    return -1;
  return (paramId - kLaneParamBase) / kLaneParamCount;
}

inline constexpr int32 laneOffsetFromParamID (Vst::ParamID paramId)
{
  if (paramId < kLaneParamBase || paramId > kLaneCoreMaxParamId)
    return -1;
  return (paramId - kLaneParamBase) % kLaneParamCount;
}

inline constexpr int32 laneFromExtraParamID (Vst::ParamID paramId)
{
  if (paramId < kLaneExtraParamBase || paramId > kLaneExtraMaxParamId)
    return -1;
  return (paramId - kLaneExtraParamBase) / kLaneExtraParamCount;
}

inline constexpr int32 laneExtraOffsetFromParamID (Vst::ParamID paramId)
{
  if (paramId < kLaneExtraParamBase || paramId > kLaneExtraMaxParamId)
    return -1;
  return (paramId - kLaneExtraParamBase) % kLaneExtraParamCount;
}

inline constexpr std::array<Vst::ParamID, kTotalParameterCount> allParameterIds ()
{
  std::array<Vst::ParamID, kTotalParameterCount> ids {};
  int32 index = 0;
  for (int32 i = 0; i < kParamGlobalCount; ++i)
    ids[index++] = static_cast<Vst::ParamID> (i);
  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 param = 0; param < kLaneParamCount; ++param)
      ids[index++] = laneParamID (lane, static_cast<LaneParamOffset> (param));
  }
  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 param = 0; param < kLaneExtraParamCount; ++param)
      ids[index++] = laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (param));
  }
  return ids;
}

} // namespace Steinberg::WestCoastDrumSynth
