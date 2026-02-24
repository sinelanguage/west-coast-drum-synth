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
  double noiseTone {0.0}; // -1 (dark) .. +1 (bright)
  double noiseDecaySeconds {0.12};
  double snapAmount {0.2};
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

  uint32_t noiseState_ {0x9E3779B9U};
  bool active_ {false};
};

} // namespace Steinberg::WestCoastDrumSynth
