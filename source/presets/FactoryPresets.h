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
};

struct FactoryPreset {
  std::string_view name;
  double master {0.8};
  double internalTempo {0.5}; // Normalized 0..1, mapped to 60..180 BPM.
  double swing {0.12};
  std::array<NormalizedLanePreset, kLaneCount> lanes {};
  PatternGrid pattern {};
};

constexpr int32 kFactoryPresetCount = 5;

const std::array<FactoryPreset, kFactoryPresetCount>& getFactoryPresets ();

} // namespace Steinberg::WestCoastDrumSynth
