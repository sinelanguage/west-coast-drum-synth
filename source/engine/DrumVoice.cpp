#include "engine/DrumVoice.h"

#include <algorithm>
#include <cmath>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr double kTwoPi = 6.28318530717958647692;

inline double clamp01 (double x)
{
  return std::clamp (x, 0.0, 1.0);
}

} // namespace

void DrumVoice::setSampleRate (double sampleRate)
{
  sampleRate_ = std::max (sampleRate, 1000.0);
}

void DrumVoice::trigger (const LaneFrame& frame)
{
  frame_ = frame;
  frame_.level = clamp01 (frame_.level);
  frame_.foldAmount = clamp01 (frame_.foldAmount);
  frame_.fmAmount = clamp01 (frame_.fmAmount);
  frame_.noiseAmount = clamp01 (frame_.noiseAmount);
  frame_.driveAmount = clamp01 (frame_.driveAmount);
  frame_.decaySeconds = std::clamp (frame_.decaySeconds, 0.01, 2.5);

  const double ampTau = frame_.decaySeconds;
  const double toneTau = std::max (0.01, frame_.decaySeconds * 0.38);
  ampDecayCoef_ = std::exp (-1.0 / (ampTau * sampleRate_));
  toneDecayCoef_ = std::exp (-1.0 / (toneTau * sampleRate_));

  ampEnv_ = 1.0;
  toneEnv_ = 1.0;
  active_ = true;
}

double DrumVoice::process ()
{
  if (!active_)
    return 0.0;

  const double modFrequency = frame_.frequencyHz * (1.5 + (frame_.fmAmount * 6.0));
  const double carrierFrequency = frame_.frequencyHz * (1.0 + (toneEnv_ * 0.08));

  modPhase_ += (kTwoPi * modFrequency) / sampleRate_;
  carrierPhase_ += (kTwoPi * carrierFrequency) / sampleRate_;
  modPhase_ = wrapPhase (modPhase_);
  carrierPhase_ = wrapPhase (carrierPhase_);

  const double fmDepth = frame_.fmAmount * (7.0 * toneEnv_);
  const double modSignal = std::sin (modPhase_) * fmDepth;
  double body = std::sin (carrierPhase_ + modSignal);

  const double dynamicFold = frame_.foldAmount * (1.0 + (0.9 * toneEnv_));
  body = wavefold (body, dynamicFold);

  const double noise = randomBipolar () * frame_.noiseAmount * (0.30 + (0.70 * toneEnv_));
  const double lpg = ampEnv_ * (0.40 + (0.60 * toneEnv_));
  const double drive = 1.0 + (frame_.driveAmount * 8.0);
  const double sample = std::tanh ((body + noise) * drive) * lpg * frame_.level;

  ampEnv_ *= ampDecayCoef_;
  toneEnv_ *= toneDecayCoef_;

  if (ampEnv_ < 0.00008)
  {
    active_ = false;
    ampEnv_ = 0.0;
    toneEnv_ = 0.0;
  }

  return sample;
}

void DrumVoice::reset ()
{
  carrierPhase_ = 0.0;
  modPhase_ = 0.0;
  ampEnv_ = 0.0;
  toneEnv_ = 0.0;
  active_ = false;
}

bool DrumVoice::isActive () const
{
  return active_;
}

double DrumVoice::wavefold (double x, double amount)
{
  const double gain = 1.0 + (amount * 7.0);
  x *= gain;

  for (int i = 0; i < 3; ++i)
  {
    if (x > 1.0)
      x = 2.0 - x;
    else if (x < -1.0)
      x = -2.0 - x;
  }
  return x;
}

double DrumVoice::randomBipolar ()
{
  noiseState_ ^= (noiseState_ << 13);
  noiseState_ ^= (noiseState_ >> 17);
  noiseState_ ^= (noiseState_ << 5);
  const double unit = static_cast<double> (noiseState_ & 0x00FFFFFF) / static_cast<double> (0x00FFFFFF);
  return (unit * 2.0) - 1.0;
}

double DrumVoice::wrapPhase (double phase)
{
  while (phase >= kTwoPi)
    phase -= kTwoPi;
  while (phase < 0.0)
    phase += kTwoPi;
  return phase;
}

} // namespace Steinberg::WestCoastDrumSynth
