#pragma once

#include "ParameterIds.h"

#include <array>
#include <string_view>

namespace Steinberg::WestCoastDrumSynth {

struct NormalizedLanePreset {
  double tune {0.5};
  double decay {0.4};
  double fold {0.35};
  double fm {0.35};
  double noise {0.2};
  double drive {0.15};
  double level {0.75};
  double pan {0.5};
  double pitchEnvAmount {0.4};
  double pitchEnvDecay {0.3};
  double transientAttack {0.3};
  double noiseTone {0.5};
  double noiseDecay {0.3};
  double snap {0.3};
  double transientDecay {0.30};
  double transientMix {0.40};
  double noiseFilterReso {0.35};
  double noiseEnvAmount {0.50};
  double oscFilterCutoff {0.65};
  double oscFilterRes {0.08};
  double oscFilterEnv {0.35};
  double transFilterCutoff {0.70};
  double transFilterRes {0.05};
  double transFilterEnv {0.40};
};

struct FactoryPreset {
  std::string_view name;
  double master {0.8};
  double internalTempo {0.5};
  double swing {0.12};
  std::array<NormalizedLanePreset, kLaneCount> lanes {};
  PatternGrid pattern {};
};

constexpr int32 kFactoryPresetCount = 16;

const std::array<FactoryPreset, kFactoryPresetCount>& getFactoryPresets ();

} // namespace Steinberg::WestCoastDrumSynth
