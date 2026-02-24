#pragma once

#include <cstdint>

namespace Steinberg::WestCoastDrumSynth {

struct LaneFrame {
  double frequencyHz {120.0};
  double decaySeconds {0.25};
  double foldAmount {0.3};
  double fmAmount {0.2};
  double noiseAmount {0.15};
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

  double ampEnv_ {0.0};
  double toneEnv_ {0.0};
  double ampDecayCoef_ {0.9995};
  double toneDecayCoef_ {0.9985};

  uint32_t noiseState_ {0x9E3779B9U};
  bool active_ {false};
};

} // namespace Steinberg::WestCoastDrumSynth
