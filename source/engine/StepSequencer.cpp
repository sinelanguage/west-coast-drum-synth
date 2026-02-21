#include "engine/StepSequencer.h"

#include <algorithm>
#include <cmath>

namespace Steinberg::WestCoastDrumSynth {

namespace {

inline double clamp01 (double x)
{
  return std::clamp (x, 0.0, 1.0);
}

} // namespace

void StepSequencer::setSampleRate (double sampleRate)
{
  sampleRate_ = std::max (sampleRate, 1000.0);
}

void StepSequencer::setTempo (double bpm)
{
  bpm_ = std::clamp (bpm, 20.0, 300.0);
}

void StepSequencer::setSwing (double swing)
{
  swing_ = clamp01 (swing);
}

void StepSequencer::setRunning (bool running)
{
  running_ = running;
}

void StepSequencer::setPattern (const PatternGrid& pattern)
{
  pattern_ = pattern;
}

void StepSequencer::reset ()
{
  currentStep_ = 0;
  samplesToNextStep_ = 0.0;
}

void StepSequencer::syncToHost (double projectPpq, bool hostPlaying)
{
  if (!hostPlaying)
    return;

  const double absoluteSteps = projectPpq * 4.0;
  const double wrapped = std::fmod (absoluteSteps, static_cast<double> (kPatternSteps));
  const int32 stepIndex = static_cast<int32> (std::floor (wrapped < 0.0 ? (wrapped + kPatternSteps) : wrapped));
  const double frac = absoluteSteps - std::floor (absoluteSteps);

  currentStep_ = std::clamp (stepIndex, 0, kPatternSteps - 1);
  const double stepDur = stepDurationSamplesFor (currentStep_);
  samplesToNextStep_ = std::max (0.0, stepDur * (1.0 - frac));
}

void StepSequencer::processSample (std::array<bool, kLaneCount>& triggers)
{
  triggers.fill (false);
  if (!running_)
    return;

  if (samplesToNextStep_ <= 0.0)
  {
    for (int32 lane = 0; lane < kLaneCount; ++lane)
      triggers[lane] = pattern_[lane][currentStep_];

    samplesToNextStep_ = stepDurationSamplesFor (currentStep_);
    currentStep_ = (currentStep_ + 1) % kPatternSteps;
  }

  samplesToNextStep_ -= 1.0;
}

int32 StepSequencer::getCurrentStep () const
{
  return currentStep_;
}

double StepSequencer::stepDurationSamplesFor (int32 stepIndex) const
{
  const double quarterNoteSeconds = 60.0 / std::max (bpm_, 1.0);
  const double sixteenthSeconds = quarterNoteSeconds * 0.25;
  const double baseSamples = sixteenthSeconds * sampleRate_;

  const double skew = clamp01 (swing_) * 0.45;
  const bool swungStep = (stepIndex % 2) != 0;
  return baseSamples * (swungStep ? (1.0 + skew) : (1.0 - skew));
}

} // namespace Steinberg::WestCoastDrumSynth
