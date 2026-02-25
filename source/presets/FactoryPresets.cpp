#include "presets/FactoryPresets.h"

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr NormalizedLanePreset lane (double tune, double decay, double fold, double fm, double noise, double drive,
                                     double level, double pan, double pitchEnvAmount, double pitchEnvDecay,
                                     double transientAttack, double noiseTone, double noiseDecay, double snap,
                                     double transientDecay, double transientMix, double noiseFilterReso,
                                     double noiseEnvAmount, double oscFltCut, double oscFltRes, double oscFltEnv,
                                     double transFltCut, double transFltRes, double transFltEnv)
{
  return {tune, decay, fold, fm, noise, drive, level, pan, pitchEnvAmount, pitchEnvDecay,
          transientAttack, noiseTone, noiseDecay, snap, transientDecay, transientMix,
          noiseFilterReso, noiseEnvAmount, oscFltCut, oscFltRes, oscFltEnv, transFltCut, transFltRes, transFltEnv};
}

} // namespace

const std::array<FactoryPreset, kFactoryPresetCount>& getFactoryPresets ()
{
  // clang-format off
  static const std::array<FactoryPreset, kFactoryPresetCount> presets {
    FactoryPreset {
      "Buchla Punch Matrix",
      0.82, 0.56, 0.09,
      {{
        lane (0.45, 0.52, 0.42, 0.35, 0.28, 0.23, 0.92, 0.47, 0.90, 0.28, 0.72, 0.34, 0.30, 0.18, 0.32, 0.55, 0.10, 0.48,  0.72, 0.08, 0.40, 0.75, 0.05, 0.35),
        lane (0.52, 0.44, 0.62, 0.57, 0.62, 0.24, 0.70, 0.57, 0.36, 0.42, 0.60, 0.63, 0.50, 0.74, 0.38, 0.60, 0.55, 0.68,  0.68, 0.12, 0.30, 0.72, 0.08, 0.45),
        lane (0.67, 0.26, 0.74, 0.39, 0.80, 0.13, 0.55, 0.28, 0.20, 0.24, 0.54, 0.87, 0.20, 0.62, 0.24, 0.72, 0.48, 0.52,  0.82, 0.06, 0.20, 0.85, 0.04, 0.30),
        lane (0.58, 0.39, 0.67, 0.43, 0.48, 0.18, 0.62, 0.64, 0.44, 0.38, 0.52, 0.58, 0.42, 0.40, 0.34, 0.50, 0.36, 0.48,  0.70, 0.10, 0.35, 0.74, 0.06, 0.38),
        lane (0.63, 0.33, 0.71, 0.49, 0.44, 0.20, 0.58, 0.74, 0.52, 0.32, 0.56, 0.61, 0.38, 0.46, 0.32, 0.54, 0.38, 0.52,  0.72, 0.10, 0.32, 0.76, 0.06, 0.40),
      }},
      {{
        {{true, false, false, false, true, false, false, false, true, false, true, false, true, false, false, false}},
        {{false, false, true, false, false, true, false, false, false, false, true, false, false, true, false, false}},
        {{true, true, true, true, true, false, true, true, true, true, true, false, true, true, true, false}},
        {{false, false, false, true, false, true, false, false, true, false, false, true, false, true, false, false}},
        {{false, true, false, false, false, false, true, false, false, true, false, false, false, false, true, false}},
      }},
    },
    FactoryPreset {
      "Coastline Electro Flux",
      0.78, 0.64, 0.17,
      {{
        lane (0.39, 0.61, 0.49, 0.44, 0.32, 0.30, 0.88, 0.46, 0.87, 0.36, 0.66, 0.39, 0.34, 0.24, 0.36, 0.58, 0.14, 0.52,  0.68, 0.10, 0.45, 0.70, 0.06, 0.38),
        lane (0.56, 0.40, 0.69, 0.63, 0.66, 0.31, 0.68, 0.58, 0.42, 0.46, 0.62, 0.67, 0.48, 0.82, 0.40, 0.62, 0.62, 0.74,  0.65, 0.14, 0.32, 0.74, 0.10, 0.48),
        lane (0.71, 0.31, 0.61, 0.34, 0.85, 0.18, 0.57, 0.30, 0.18, 0.21, 0.50, 0.92, 0.22, 0.70, 0.22, 0.68, 0.46, 0.50,  0.84, 0.06, 0.18, 0.88, 0.04, 0.28),
        lane (0.66, 0.36, 0.74, 0.50, 0.46, 0.25, 0.60, 0.71, 0.48, 0.33, 0.54, 0.55, 0.40, 0.46, 0.36, 0.52, 0.40, 0.52,  0.72, 0.10, 0.35, 0.76, 0.06, 0.40),
        lane (0.61, 0.43, 0.81, 0.59, 0.50, 0.29, 0.56, 0.78, 0.56, 0.28, 0.58, 0.64, 0.38, 0.54, 0.34, 0.56, 0.42, 0.56,  0.70, 0.12, 0.30, 0.78, 0.08, 0.42),
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
      0.75, 0.47, 0.23,
      {{
        lane (0.47, 0.50, 0.53, 0.26, 0.26, 0.27, 0.89, 0.45, 0.84, 0.31, 0.62, 0.30, 0.28, 0.16, 0.30, 0.52, 0.16, 0.44,  0.74, 0.06, 0.42, 0.68, 0.05, 0.32),
        lane (0.63, 0.33, 0.79, 0.70, 0.70, 0.23, 0.66, 0.60, 0.40, 0.40, 0.58, 0.72, 0.44, 0.86, 0.42, 0.64, 0.66, 0.78,  0.62, 0.16, 0.28, 0.70, 0.12, 0.50),
        lane (0.73, 0.24, 0.82, 0.42, 0.88, 0.17, 0.58, 0.34, 0.20, 0.20, 0.52, 0.96, 0.20, 0.78, 0.20, 0.74, 0.54, 0.58,  0.86, 0.04, 0.16, 0.90, 0.04, 0.25),
        lane (0.61, 0.34, 0.88, 0.56, 0.48, 0.29, 0.64, 0.69, 0.52, 0.30, 0.54, 0.61, 0.38, 0.50, 0.38, 0.54, 0.42, 0.54,  0.68, 0.12, 0.34, 0.72, 0.08, 0.42),
        lane (0.68, 0.30, 0.84, 0.62, 0.52, 0.24, 0.55, 0.80, 0.58, 0.25, 0.56, 0.68, 0.34, 0.56, 0.34, 0.58, 0.44, 0.58,  0.70, 0.12, 0.30, 0.76, 0.08, 0.44),
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
      0.80, 0.60, 0.31,
      {{
        lane (0.42, 0.58, 0.57, 0.50, 0.34, 0.35, 0.90, 0.46, 0.93, 0.30, 0.76, 0.40, 0.32, 0.20, 0.34, 0.60, 0.14, 0.52,  0.66, 0.12, 0.48, 0.72, 0.08, 0.40),
        lane (0.54, 0.37, 0.74, 0.73, 0.72, 0.34, 0.67, 0.59, 0.48, 0.45, 0.64, 0.74, 0.50, 0.87, 0.44, 0.62, 0.64, 0.76,  0.60, 0.18, 0.30, 0.68, 0.14, 0.52),
        lane (0.68, 0.28, 0.79, 0.39, 0.86, 0.21, 0.53, 0.31, 0.24, 0.23, 0.50, 0.94, 0.24, 0.71, 0.22, 0.70, 0.52, 0.54,  0.84, 0.06, 0.18, 0.88, 0.04, 0.28),
        lane (0.70, 0.43, 0.86, 0.63, 0.52, 0.28, 0.63, 0.74, 0.55, 0.35, 0.58, 0.60, 0.42, 0.58, 0.40, 0.56, 0.40, 0.56,  0.68, 0.14, 0.36, 0.74, 0.10, 0.44),
        lane (0.74, 0.37, 0.90, 0.68, 0.54, 0.31, 0.56, 0.83, 0.62, 0.29, 0.60, 0.69, 0.38, 0.64, 0.36, 0.58, 0.44, 0.60,  0.66, 0.14, 0.32, 0.76, 0.10, 0.46),
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
      0.77, 0.52, 0.14,
      {{
        lane (0.44, 0.54, 0.44, 0.31, 0.30, 0.19, 0.87, 0.47, 0.88, 0.29, 0.70, 0.36, 0.30, 0.19, 0.34, 0.56, 0.12, 0.48,  0.70, 0.08, 0.40, 0.72, 0.05, 0.36),
        lane (0.57, 0.45, 0.60, 0.54, 0.58, 0.20, 0.69, 0.56, 0.38, 0.39, 0.56, 0.62, 0.46, 0.73, 0.38, 0.58, 0.56, 0.70,  0.66, 0.12, 0.32, 0.72, 0.08, 0.44),
        lane (0.65, 0.30, 0.70, 0.30, 0.82, 0.11, 0.55, 0.33, 0.16, 0.22, 0.48, 0.88, 0.20, 0.63, 0.22, 0.66, 0.48, 0.52,  0.82, 0.06, 0.18, 0.86, 0.04, 0.26),
        lane (0.60, 0.40, 0.63, 0.41, 0.46, 0.18, 0.61, 0.67, 0.45, 0.37, 0.52, 0.56, 0.38, 0.44, 0.36, 0.50, 0.36, 0.48,  0.70, 0.10, 0.34, 0.74, 0.06, 0.38),
        lane (0.67, 0.35, 0.72, 0.47, 0.48, 0.20, 0.58, 0.77, 0.52, 0.33, 0.54, 0.63, 0.36, 0.50, 0.34, 0.54, 0.40, 0.54,  0.72, 0.10, 0.30, 0.76, 0.08, 0.40),
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
  // clang-format on

  return presets;
}

} // namespace Steinberg::WestCoastDrumSynth
