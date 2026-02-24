#include "presets/FactoryPresets.h"

namespace Steinberg::WestCoastDrumSynth {

const std::array<FactoryPreset, kFactoryPresetCount>& getFactoryPresets ()
{
  static const std::array<FactoryPreset, kFactoryPresetCount> presets {
    FactoryPreset {
      "Buchla Punch Matrix",
      0.82,
      0.56,
      0.09,
      {{
        {0.45, 0.52, 0.42, 0.35, 0.06, 0.21, 0.92, 0.47}, // Kick
        {0.52, 0.44, 0.62, 0.57, 0.55, 0.24, 0.70, 0.57}, // Snare
        {0.67, 0.26, 0.74, 0.39, 0.90, 0.12, 0.55, 0.28}, // Hat
        {0.58, 0.39, 0.67, 0.43, 0.29, 0.18, 0.62, 0.69}, // Perc
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, true, false, true, false, false, false}},
        {{false, false, true, false, false, true, false, false, false, false, true, false, false, true, false, false}},
        {{true, true, true, true, true, false, true, true, true, true, true, false, true, true, true, false}},
        {{false, false, false, true, false, true, false, false, true, false, false, true, false, true, false, false}},
      }},
    },
    FactoryPreset {
      "Coastline Electro Flux",
      0.78,
      0.64,
      0.17,
      {{
        {0.39, 0.61, 0.49, 0.44, 0.09, 0.30, 0.88, 0.46},
        {0.56, 0.40, 0.69, 0.63, 0.62, 0.31, 0.68, 0.58},
        {0.71, 0.31, 0.61, 0.34, 0.95, 0.18, 0.57, 0.30},
        {0.66, 0.36, 0.74, 0.50, 0.22, 0.25, 0.60, 0.72},
      }},
      {{
        {{true, false, false, false, false, false, true, false, true, false, false, false, false, false, true, false}},
        {{false, false, true, false, false, true, false, true, false, false, true, false, false, true, false, true}},
        {{true, false, true, false, true, true, true, false, true, false, true, false, true, true, true, false}},
        {{false, false, false, true, true, false, false, false, false, true, false, true, true, false, false, false}},
      }},
    },
    FactoryPreset {
      "LPG Metallic Ritual",
      0.75,
      0.47,
      0.23,
      {{
        {0.47, 0.50, 0.53, 0.26, 0.05, 0.27, 0.89, 0.45},
        {0.63, 0.33, 0.79, 0.70, 0.66, 0.23, 0.66, 0.60},
        {0.73, 0.24, 0.82, 0.42, 0.98, 0.17, 0.58, 0.34},
        {0.61, 0.34, 0.88, 0.56, 0.25, 0.29, 0.64, 0.71},
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, false, false, true, false, true, false}},
        {{false, false, true, false, false, false, false, true, false, false, true, false, false, false, false, true}},
        {{true, true, true, false, true, false, true, true, true, false, true, false, true, true, true, false}},
        {{false, true, false, true, false, false, false, false, false, true, false, false, false, true, false, true}},
      }},
    },
    FactoryPreset {
      "West Coast Broken Clock",
      0.80,
      0.60,
      0.31,
      {{
        {0.42, 0.58, 0.57, 0.50, 0.10, 0.35, 0.90, 0.46},
        {0.54, 0.37, 0.74, 0.73, 0.74, 0.34, 0.67, 0.59},
        {0.68, 0.28, 0.79, 0.39, 0.99, 0.21, 0.53, 0.31},
        {0.70, 0.43, 0.86, 0.63, 0.31, 0.28, 0.63, 0.75},
      }},
      {{
        {{true, false, false, true, false, false, true, false, true, false, false, false, true, false, false, false}},
        {{false, true, false, false, true, false, false, true, false, true, false, false, true, false, false, true}},
        {{true, true, false, true, true, false, true, false, true, true, false, true, true, false, true, false}},
        {{false, false, true, false, false, true, false, true, false, false, true, false, false, true, false, true}},
      }},
    },
    FactoryPreset {
      "Voltage Dust Stepper",
      0.77,
      0.52,
      0.14,
      {{
        {0.44, 0.54, 0.44, 0.31, 0.07, 0.19, 0.87, 0.47},
        {0.57, 0.45, 0.60, 0.54, 0.49, 0.20, 0.69, 0.56},
        {0.65, 0.30, 0.70, 0.30, 0.90, 0.11, 0.55, 0.33},
        {0.60, 0.40, 0.63, 0.41, 0.23, 0.18, 0.61, 0.68},
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, false, false, true, false, false, false}},
        {{false, false, true, false, false, true, false, false, false, false, true, false, false, true, false, false}},
        {{true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false}},
        {{false, false, false, true, false, false, true, false, false, true, false, false, true, false, false, true}},
      }},
    },
  };

  return presets;
}

} // namespace Steinberg::WestCoastDrumSynth
