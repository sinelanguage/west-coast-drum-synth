#include "engine/DrumVoice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr double kTwoPi = 6.28318530717958647692;

inline double clamp01 (double x)
{
  return std::clamp (x, 0.0, 1.0);
}

inline size_t characterIndex (LaneCharacter character)
{
  return static_cast<size_t> (character);
}

constexpr std::array<double, 5> kPitchSemitoneSpan {66.0, 20.0, 8.0, 22.0, 28.0};
constexpr std::array<double, 5> kTransientBaseHz {1700.0, 2500.0, 7000.0, 3100.0, 4200.0};
constexpr std::array<double, 5> kNoiseBlendGain {0.45, 1.0, 1.2, 0.72, 0.85};
constexpr std::array<double, 5> kBodyGain {1.05, 0.70, 0.30, 0.88, 0.92};
constexpr std::array<double, 5> kFmScale {1.0, 0.85, 0.40, 0.78, 0.85};
constexpr std::array<double, 5> kNoiseTransientBlend {0.45, 0.75, 0.35, 0.58, 0.62};

} // namespace

void DrumVoice::setSampleRate (double sampleRate)
{
  sampleRate_ = std::max (sampleRate, 1000.0);
}

void DrumVoice::trigger (const LaneFrame& frame)
{
  frame_ = frame;
  frame_.frequencyHz = std::clamp (frame_.frequencyHz, 20.0, 15000.0);
  frame_.pitchEnvAmount = clamp01 (frame_.pitchEnvAmount);
  frame_.transientAmount = clamp01 (frame_.transientAmount);
  frame_.transientMix = std::clamp (frame_.transientMix, 0.0, 1.4);
  frame_.snapAmount = clamp01 (frame_.snapAmount);
  frame_.noiseTone = std::clamp (frame_.noiseTone, -1.0, 1.0);
  frame_.noiseResonance = std::clamp (frame_.noiseResonance, 0.0, 0.98);
  frame_.noiseEnvAmount = std::clamp (frame_.noiseEnvAmount, 0.0, 1.5);
  frame_.level = clamp01 (frame_.level);
  frame_.foldAmount = clamp01 (frame_.foldAmount);
  frame_.fmAmount = clamp01 (frame_.fmAmount);
  frame_.noiseAmount = std::clamp (frame_.noiseAmount, 0.0, 2.2);
  frame_.driveAmount = clamp01 (frame_.driveAmount);
  frame_.transientDecaySeconds = std::clamp (frame_.transientDecaySeconds, 0.001, 0.05);
  frame_.transientMix = clamp01 (frame_.transientMix);
  frame_.noiseFilterReso = clamp01 (frame_.noiseFilterReso);
  frame_.noiseEnvAmount = clamp01 (frame_.noiseEnvAmount);
  frame_.decaySeconds = std::clamp (frame_.decaySeconds, 0.01, 2.5);
  frame_.pitchEnvDecaySeconds = std::clamp (frame_.pitchEnvDecaySeconds, 0.004, 0.8);
  frame_.noiseDecaySeconds = std::clamp (frame_.noiseDecaySeconds, 0.004, 1.8);
  frame_.transientDecaySeconds = std::clamp (frame_.transientDecaySeconds, 0.0015, 0.5);

  const double ampTau = frame_.decaySeconds;
  const double toneTau = std::max (0.01, frame_.decaySeconds * 0.33);
  const double pitchTau = frame_.pitchEnvDecaySeconds;
  const double noiseTau = frame_.noiseDecaySeconds;
  const double transientTau =
    std::clamp (frame_.transientDecaySeconds * (1.08 - (0.45 * frame_.snapAmount)), 0.0015, 0.5);
  ampDecayCoef_ = std::exp (-1.0 / (ampTau * sampleRate_));
  toneDecayCoef_ = std::exp (-1.0 / (toneTau * sampleRate_));
  pitchDecayCoef_ = std::exp (-1.0 / (pitchTau * sampleRate_));
  noiseDecayCoef_ = std::exp (-1.0 / (noiseTau * sampleRate_));
  transientDecayCoef_ = std::exp (-1.0 / (transientTau * sampleRate_));

  const double tone01 = clamp01 ((frame_.noiseTone + 1.0) * 0.5);
  const double lowCutoffHz = 240.0 + (tone01 * 9200.0);
  const double highCutoffHz = 120.0 + (tone01 * 5200.0);
  noiseLpCoef_ = 1.0 - std::exp (-(kTwoPi * lowCutoffHz) / sampleRate_);
  noiseHpCoef_ = 1.0 - std::exp (-(kTwoPi * highCutoffHz) / sampleRate_);
  noiseLpCoef_ = std::clamp (noiseLpCoef_, 0.0, 1.0);
  noiseHpCoef_ = std::clamp (noiseHpCoef_, 0.0, 1.0);

  ampEnv_ = 1.0;
  toneEnv_ = 1.0;
  pitchEnv_ = 1.0;
  noiseEnv_ = 1.0;
  transientEnv_ = 1.0;
  active_ = true;
}

double DrumVoice::process ()
{
  if (!active_)
    return 0.0;

  const size_t character = characterIndex (frame_.character);
  const double pitchSemitoneSweep = frame_.pitchEnvAmount * pitchEnv_ * kPitchSemitoneSpan[character];
  const double pitchRatio = std::pow (2.0, pitchSemitoneSweep / 12.0);

  const double modFrequency =
    frame_.frequencyHz * pitchRatio * (1.25 + (frame_.fmAmount * (5.2 * kFmScale[character])));
  const double carrierFrequency = frame_.frequencyHz * pitchRatio * (1.0 + (toneEnv_ * 0.07));

  modPhase_ += (kTwoPi * modFrequency) / sampleRate_;
  carrierPhase_ += (kTwoPi * carrierFrequency) / sampleRate_;
  transientPhase_ += (kTwoPi * kTransientBaseHz[character] * (0.85 + (frame_.transientAmount * 0.8))) / sampleRate_;
  modPhase_ = wrapPhase (modPhase_);
  carrierPhase_ = wrapPhase (carrierPhase_);
  transientPhase_ = wrapPhase (transientPhase_);

  const double fmDepth = frame_.fmAmount * ((6.4 * toneEnv_ * kFmScale[character]) + 0.3);
  const double modSignal = std::sin (modPhase_) * fmDepth;
  double body = std::sin (carrierPhase_ + modSignal);

  const double dynamicFold = frame_.foldAmount * (1.0 + (0.9 * toneEnv_));
  body = wavefold (body, dynamicFold);

  const double bodyCutoffBase = std::clamp (frame_.frequencyHz * (2.0 + (frame_.foldAmount * 6.0)), 120.0, 12000.0);
  const double bodyCutoffEnv =
    std::clamp (bodyCutoffBase * (1.0 + (toneEnv_ * (0.85 + (0.9 * frame_.fmAmount)))), 120.0, 16000.0);
  const double bodyResonance = std::clamp (0.08 + (0.58 * frame_.driveAmount) + (0.22 * frame_.foldAmount), 0.0, 0.95);
  body = processStateVariableLowpass (body, bodyCutoffEnv, bodyResonance, bodyLowState_, bodyBandState_);

  const double rawNoise = randomBipolar ();
  noiseLowState_ += noiseLpCoef_ * (rawNoise - noiseLowState_);
  noiseHighState_ += noiseHpCoef_ * (rawNoise - noiseHighState_);
  const double highNoise = rawNoise - noiseHighState_;
  const double toneBlend = clamp01 ((frame_.noiseTone + 1.0) * 0.5);
  const double shapedNoise = ((1.0 - toneBlend) * noiseLowState_) + (toneBlend * highNoise);

  const double snapExponent = std::clamp (0.85 - (frame_.snapAmount * 0.55), 0.25, 1.0);
  const double snappyEnv = std::pow (std::max (noiseEnv_, 0.0), snapExponent);
  const double noiseContour = ((1.0 - frame_.snapAmount) * noiseEnv_) + (frame_.snapAmount * snappyEnv);
  const double noiseCutoffBase = 280.0 + (toneBlend * 13000.0);
  const double noiseCutoffEnv =
    std::clamp (noiseCutoffBase * (1.0 + (noiseContour * frame_.noiseEnvAmount * 2.9)), 180.0, 18000.0);
  const double noiseResonance = std::clamp (frame_.noiseResonance + (frame_.snapAmount * 0.16), 0.0, 0.98);
  const double resonantNoise = processStateVariableLowpass (shapedNoise, noiseCutoffEnv, noiseResonance, noiseResLowState_,
                                                            noiseResBandState_);
  const double noise = resonantNoise * frame_.noiseAmount * noiseContour * kNoiseBlendGain[character];

  const double transientOsc = std::sin (transientPhase_);
  const double transientNoise = randomBipolar ();
  const double transientBlend = std::clamp (frame_.transientMix * kNoiseTransientBlend[character] * 2.0, 0.0, 1.0);
  const double transientCore = (transientOsc * (1.0 - transientBlend)) + (transientNoise * transientBlend);
  const double transientGain = std::pow (frame_.transientAmount, 0.75) * frame_.transientMix * 1.45;
  const double transient = transientCore * transientEnv_ * transientGain;

  const double lpg = ampEnv_ * (0.40 + (0.60 * toneEnv_));
  const double drive = 1.0 + (frame_.driveAmount * 8.0);
  const double sample = std::tanh (((body * kBodyGain[character]) + noise + transient) * drive) * lpg * frame_.level;

  ampEnv_ *= ampDecayCoef_;
  toneEnv_ *= toneDecayCoef_;
  pitchEnv_ *= pitchDecayCoef_;
  noiseEnv_ *= noiseDecayCoef_;
  transientEnv_ *= transientDecayCoef_;

  if (ampEnv_ < 0.00008 && noiseEnv_ < 0.00008 && transientEnv_ < 0.00008)
  {
    active_ = false;
    ampEnv_ = 0.0;
    toneEnv_ = 0.0;
    pitchEnv_ = 0.0;
    noiseEnv_ = 0.0;
    transientEnv_ = 0.0;
  }

  return sample;
}

void DrumVoice::reset ()
{
  carrierPhase_ = 0.0;
  modPhase_ = 0.0;
  transientPhase_ = 0.0;
  ampEnv_ = 0.0;
  toneEnv_ = 0.0;
  pitchEnv_ = 0.0;
  noiseEnv_ = 0.0;
  transientEnv_ = 0.0;
  noiseLowState_ = 0.0;
  noiseHighState_ = 0.0;
  noiseResLowState_ = 0.0;
  noiseResBandState_ = 0.0;
  bodyLowState_ = 0.0;
  bodyBandState_ = 0.0;
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

double DrumVoice::processStateVariableLowpass (double input, double cutoffHz, double resonance, double& lowState,
                                               double& bandState)
{
  const double clippedCutoff = std::clamp (cutoffHz, 20.0, sampleRate_ * 0.47);
  const double f = std::clamp ((2.0 * std::sin ((3.14159265358979323846 * clippedCutoff) / sampleRate_)), 0.0, 1.0);
  const double q = std::clamp (resonance, 0.0, 0.99);
  const double damping = 1.0 - q;
  const double high = input - lowState - (damping * bandState);
  bandState += f * high;
  lowState += f * bandState;
  return lowState;
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
