#include "presets/FactoryPresets.h"

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr NormalizedLanePreset lane (double tune, double decay, double fold, double fm, double noise, double drive,
                                     double level, double pan, double pitchEnvAmount, double pitchEnvDecay,
                                     double transientAttack, double noiseTone, double noiseDecay, double snap)
{
  return {tune, decay, fold, fm, noise, drive, level, pan, pitchEnvAmount, pitchEnvDecay,
          transientAttack, noiseTone, noiseDecay, snap};
}

} // namespace

const std::array<FactoryPreset, kFactoryPresetCount>& getFactoryPresets ()
{
  static const std::array<FactoryPreset, kFactoryPresetCount> presets {
    FactoryPreset {
      "Buchla Punch Matrix",
      0.82,
      0.56,
      0.09,
      {{
        lane (0.45, 0.52, 0.42, 0.35, 0.05, 0.23, 0.92, 0.47, 0.90, 0.28, 0.72, 0.34, 0.20, 0.18), // Kick
        lane (0.52, 0.44, 0.62, 0.57, 0.62, 0.24, 0.70, 0.57, 0.36, 0.42, 0.40, 0.63, 0.41, 0.74), // Snare
        lane (0.67, 0.26, 0.74, 0.39, 0.93, 0.13, 0.55, 0.28, 0.20, 0.24, 0.34, 0.87, 0.14, 0.62), // Hat
        lane (0.58, 0.39, 0.67, 0.43, 0.30, 0.18, 0.62, 0.64, 0.44, 0.38, 0.36, 0.58, 0.36, 0.40), // Perc A
        lane (0.63, 0.33, 0.71, 0.49, 0.27, 0.20, 0.58, 0.74, 0.52, 0.32, 0.41, 0.61, 0.30, 0.46), // Perc B
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, true, false, true, false, false, false}}, // Kick
        {{false, false, true, false, false, true, false, false, false, false, true, false, false, true, false, false}}, // Snare
        {{true, true, true, true, true, false, true, true, true, true, true, false, true, true, true, false}}, // Hat
        {{false, false, false, true, false, true, false, false, true, false, false, true, false, true, false, false}}, // Perc A
        {{false, true, false, false, false, false, true, false, false, true, false, false, false, false, true, false}}, // Perc B
      }},
    },
    FactoryPreset {
      "Coastline Electro Flux",
      0.78,
      0.64,
      0.17,
      {{
        lane (0.39, 0.61, 0.49, 0.44, 0.08, 0.30, 0.88, 0.46, 0.87, 0.36, 0.66, 0.39, 0.23, 0.24),
        lane (0.56, 0.40, 0.69, 0.63, 0.66, 0.31, 0.68, 0.58, 0.42, 0.46, 0.45, 0.67, 0.38, 0.82),
        lane (0.71, 0.31, 0.61, 0.34, 0.96, 0.18, 0.57, 0.30, 0.18, 0.21, 0.30, 0.92, 0.16, 0.70),
        lane (0.66, 0.36, 0.74, 0.50, 0.24, 0.25, 0.60, 0.71, 0.48, 0.33, 0.38, 0.55, 0.34, 0.46),
        lane (0.61, 0.43, 0.81, 0.59, 0.31, 0.29, 0.56, 0.78, 0.56, 0.28, 0.44, 0.64, 0.30, 0.54),
      }},
      {{
        {{true, false, false, false, false, false, true, false, true, false, false, false, false, false, true, false}},
        {{false, false, true, false, false, true, false, true, false, false, true, false, false, true, false, true}},
        {{true, false, true, false, true, true, true, false, true, false, true, false, true, true, true, false}},
        {{false, false, false, true, true, false, false, false, false, true, false, true, true, false, false, false}},
        {{false, true, false, false, false, false, true, false, false, false, true, false, false, true, false, false}},
      }},
    },
    FactoryPreset {
      "LPG Metallic Ritual",
      0.75,
      0.47,
      0.23,
      {{
        lane (0.47, 0.50, 0.53, 0.26, 0.04, 0.27, 0.89, 0.45, 0.84, 0.31, 0.62, 0.30, 0.20, 0.16),
        lane (0.63, 0.33, 0.79, 0.70, 0.70, 0.23, 0.66, 0.60, 0.40, 0.40, 0.42, 0.72, 0.36, 0.86),
        lane (0.73, 0.24, 0.82, 0.42, 0.99, 0.17, 0.58, 0.34, 0.20, 0.20, 0.36, 0.96, 0.14, 0.78),
        lane (0.61, 0.34, 0.88, 0.56, 0.28, 0.29, 0.64, 0.69, 0.52, 0.30, 0.34, 0.61, 0.30, 0.50),
        lane (0.68, 0.30, 0.84, 0.62, 0.34, 0.24, 0.55, 0.80, 0.58, 0.25, 0.40, 0.68, 0.26, 0.56),
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, false, false, true, false, true, false}},
        {{false, false, true, false, false, false, false, true, false, false, true, false, false, false, false, true}},
        {{true, true, true, false, true, false, true, true, true, false, true, false, true, true, true, false}},
        {{false, true, false, true, false, false, false, false, false, true, false, false, false, true, false, true}},
        {{false, false, true, false, false, true, false, false, true, false, false, true, false, false, true, false}},
      }},
    },
    FactoryPreset {
      "West Coast Broken Clock",
      0.80,
      0.60,
      0.31,
      {{
        lane (0.42, 0.58, 0.57, 0.50, 0.10, 0.35, 0.90, 0.46, 0.93, 0.30, 0.76, 0.40, 0.22, 0.20),
        lane (0.54, 0.37, 0.74, 0.73, 0.77, 0.34, 0.67, 0.59, 0.48, 0.45, 0.48, 0.74, 0.40, 0.87),
        lane (0.68, 0.28, 0.79, 0.39, 0.99, 0.21, 0.53, 0.31, 0.24, 0.23, 0.32, 0.94, 0.18, 0.71),
        lane (0.70, 0.43, 0.86, 0.63, 0.32, 0.28, 0.63, 0.74, 0.55, 0.35, 0.45, 0.60, 0.34, 0.58),
        lane (0.74, 0.37, 0.90, 0.68, 0.36, 0.31, 0.56, 0.83, 0.62, 0.29, 0.52, 0.69, 0.30, 0.64),
      }},
      {{
        {{true, false, false, true, false, false, true, false, true, false, false, false, true, false, false, false}},
        {{false, true, false, false, true, false, false, true, false, true, false, false, true, false, false, true}},
        {{true, true, false, true, true, false, true, false, true, true, false, true, true, false, true, false}},
        {{false, false, true, false, false, true, false, true, false, false, true, false, false, true, false, true}},
        {{true, false, false, false, true, false, true, false, false, true, false, false, true, false, true, false}},
      }},
    },
    FactoryPreset {
      "Voltage Dust Stepper",
      0.77,
      0.52,
      0.14,
      {{
        lane (0.44, 0.54, 0.44, 0.31, 0.06, 0.19, 0.87, 0.47, 0.88, 0.29, 0.70, 0.36, 0.21, 0.19),
        lane (0.57, 0.45, 0.60, 0.54, 0.56, 0.20, 0.69, 0.56, 0.38, 0.39, 0.39, 0.62, 0.39, 0.73),
        lane (0.65, 0.30, 0.70, 0.30, 0.91, 0.11, 0.55, 0.33, 0.16, 0.22, 0.28, 0.88, 0.14, 0.63),
        lane (0.60, 0.40, 0.63, 0.41, 0.24, 0.18, 0.61, 0.67, 0.45, 0.37, 0.34, 0.56, 0.32, 0.44),
        lane (0.67, 0.35, 0.72, 0.47, 0.28, 0.20, 0.58, 0.77, 0.52, 0.33, 0.39, 0.63, 0.28, 0.50),
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, false, false, true, false, false, false}},
        {{false, false, true, false, false, true, false, false, false, false, true, false, false, true, false, false}},
        {{true, false, true, false, true, false, true, false, true, false, true, false, true, false, true, false}},
        {{false, false, false, true, false, false, true, false, false, true, false, false, true, false, false, true}},
        {{false, true, false, false, false, false, false, true, false, false, true, false, false, false, false, true}},
      }},
    },
  };

  return presets;
}

} // namespace Steinberg::WestCoastDrumSynth
