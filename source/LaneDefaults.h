#pragma once

#include "ParameterIds.h"

#include <array>

namespace Steinberg::WestCoastDrumSynth {

// Canonical per-lane defaults used by processor state migration and controller registration.
constexpr std::array<std::array<double, kLaneExtraParamCount>, kLaneCount> kLaneExtraDefaults {{
  {{0.84, 0.28, 0.78, 0.30, 0.22, 0.18}},
  {{0.44, 0.44, 0.72, 0.62, 0.52, 0.86}},
  {{0.16, 0.16, 0.34, 0.94, 0.14, 0.70}},
  {{0.40, 0.34, 0.58, 0.40, 0.36, 0.32}},
  {{0.46, 0.30, 0.62, 0.46, 0.32, 0.40}},
  {{0.56, 0.28, 0.68, 0.66, 0.28, 0.52}},
  {{0.24, 0.32, 0.56, 0.82, 0.74, 0.52}},
  {{0.32, 0.20, 0.82, 0.46, 0.18, 0.84}},
}};

constexpr std::array<std::array<double, kLaneMacroParamCount>, kLaneCount> kLaneMacroDefaults {{
  {{0.26, 0.50, 0.28, 0.48}},
  {{0.36, 0.64, 0.58, 0.74}},
  {{0.16, 0.40, 0.72, 0.90}},
  {{0.34, 0.54, 0.38, 0.48}},
  {{0.30, 0.58, 0.40, 0.46}},
  {{0.24, 0.64, 0.44, 0.44}},
  {{0.44, 0.72, 0.62, 0.78}},
  {{0.14, 0.78, 0.32, 0.34}},
}};

constexpr std::array<std::array<double, kLaneFilterParamCount>, kLaneCount> kLaneFilterDefaults {{
  {{0.72, 0.05, 0.28, 0.54, 0.18, 0.26}},
  {{0.66, 0.10, 0.24, 0.62, 0.22, 0.40}},
  {{0.84, 0.04, 0.16, 0.88, 0.05, 0.24}},
  {{0.70, 0.09, 0.22, 0.48, 0.28, 0.30}},
  {{0.76, 0.08, 0.20, 0.58, 0.26, 0.34}},
  {{0.80, 0.06, 0.18, 0.68, 0.20, 0.28}},
  {{0.52, 0.14, 0.20, 0.78, 0.12, 0.44}},
  {{0.62, 0.07, 0.14, 0.38, 0.34, 0.18}},
}};

} // namespace Steinberg::WestCoastDrumSynth
