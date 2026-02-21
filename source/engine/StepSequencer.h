#pragma once

#include "ParameterIds.h"

#include <array>

namespace Steinberg::WestCoastDrumSynth {

class StepSequencer {
public:
  void setSampleRate (double sampleRate);
  void setTempo (double bpm);
  void setSwing (double swing);
  void setRunning (bool running);
  void setPattern (const PatternGrid& pattern);
  void reset ();

  // Keeps the sequencer phase hard-locked to host position.
  void syncToHost (double projectPpq, bool hostPlaying);

  void processSample (std::array<bool, kLaneCount>& triggers);
  int32 getCurrentStep () const;

private:
  double stepDurationSamplesFor (int32 stepIndex) const;

  double sampleRate_ {44100.0};
  double bpm_ {120.0};
  double swing_ {0.0};
  bool running_ {true};

  PatternGrid pattern_ {};
  int32 currentStep_ {0};
  double samplesToNextStep_ {0.0};
};

} // namespace Steinberg::WestCoastDrumSynth
