#pragma once

#include <cstdint>

namespace Steinberg::WestCoastDrumSynth {

enum class LaneCharacter : uint8_t {
  Kick = 0,
  Snare,
  Hat,
  PercA,
  PercB
};

struct LaneFrame {
  LaneCharacter character {LaneCharacter::Kick};
  double frequencyHz {120.0};
  double decaySeconds {0.25};
  double foldAmount {0.3};
  double fmAmount {0.2};
  double noiseAmount {0.15};
  double pitchEnvAmount {0.25};
  double pitchEnvDecaySeconds {0.06};
  double transientAmount {0.2};
  double transientDecaySeconds {0.03};
  double transientMix {0.42};
  double noiseTone {0.0}; // -1 (dark) .. +1 (bright)
  double noiseDecaySeconds {0.12};
  double noiseResonance {0.45};
  double noiseEnvAmount {0.55};
  double snapAmount {0.2};
  double transientDecaySeconds {0.006};
  double transientMix {0.5}; // 0 = pure sine click, 1 = pure noise click
  double noiseFilterReso {0.3};
  double noiseEnvAmount {0.5};
  double driveAmount {0.1};
  double level {0.7};
  double pan {0.0};
};

class DrumVoice {
public:
  void setSampleRate (double sampleRate);
  void trigger (const LaneFrame& frame);
  double process ();
  void reset ();
  bool isActive () const;

private:
  static double wavefold (double x, double amount);
  double processStateVariableLowpass (double input, double cutoffHz, double resonance, double& lowState,
                                      double& bandState);
  double randomBipolar ();
  static double wrapPhase (double phase);

  double sampleRate_ {44100.0};
  LaneFrame frame_ {};

  double carrierPhase_ {0.0};
  double modPhase_ {0.0};
  double transientPhase_ {0.0};

  double ampEnv_ {0.0};
  double toneEnv_ {0.0};
  double pitchEnv_ {0.0};
  double noiseEnv_ {0.0};
  double transientEnv_ {0.0};
  double ampDecayCoef_ {0.9995};
  double toneDecayCoef_ {0.9985};
  double pitchDecayCoef_ {0.995};
  double noiseDecayCoef_ {0.995};
  double transientDecayCoef_ {0.96};

  double noiseLowState_ {0.0};
  double noiseHighState_ {0.0};
  double noiseLpCoef_ {0.15};
  double noiseHpCoef_ {0.15};
  double noiseResLowState_ {0.0};
  double noiseResBandState_ {0.0};
  double bodyLowState_ {0.0};
  double bodyBandState_ {0.0};

  uint32_t noiseState_ {0x9E3779B9U};
  bool active_ {false};
};

} // namespace Steinberg::WestCoastDrumSynth
