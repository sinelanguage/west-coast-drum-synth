#include "engine/DrumVoice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr double kTwoPi = 6.28318530717958647692;
constexpr double kPi = 3.14159265358979323846;

inline double clamp01 (double x)
{
  return std::clamp (x, 0.0, 1.0);
}

inline size_t characterIndex (LaneCharacter character)
{
  return static_cast<size_t> (character);
}

inline double softClip (double x)
{
  constexpr double kSoftClipDrive = 1.65;
  return std::tanh (x * kSoftClipDrive) / std::tanh (kSoftClipDrive);
}

constexpr std::array<double, 5> kPitchSemitoneSpan {66.0, 20.0, 8.0, 22.0, 28.0};
constexpr std::array<double, 5> kTransientBaseHz {1700.0, 2500.0, 7000.0, 3100.0, 4200.0};
constexpr std::array<double, 5> kNoiseBlendGain {0.85, 1.25, 1.45, 1.10, 1.15};
constexpr std::array<double, 5> kBodyGain {1.05, 0.84, 0.42, 0.98, 1.02};
constexpr std::array<double, 5> kFmScale {1.0, 0.85, 0.40, 0.78, 0.85};
constexpr std::array<double, 5> kNoiseTransientBlend {0.40, 0.68, 0.38, 0.54, 0.58};

inline double cutoffFromNormalized (double normalized, double minHz, double maxHz)
{
  return minHz * std::pow (maxHz / minHz, normalized);
}

} // namespace

void DrumVoice::setSampleRate (double sampleRate)
{
  sampleRate_ = std::max (sampleRate, 1000.0);
}

void DrumVoice::trigger (const LaneFrame& frame)
{
  frame_ = frame;
  frame_.frequencyHz = std::clamp (frame_.frequencyHz, 20.0, 18000.0);
  frame_.oscLevel = std::clamp (frame_.oscLevel, 0.0, 2.0);
  frame_.bodyFilterCutoffHz = std::clamp (frame_.bodyFilterCutoffHz, 80.0, 18000.0);
  frame_.bodyFilterResonance = std::clamp (frame_.bodyFilterResonance, 0.0, 0.98);
  frame_.bodyFilterEnvAmount = std::clamp (frame_.bodyFilterEnvAmount, 0.0, 2.5);
  frame_.outputLevel = std::clamp (frame_.outputLevel, 0.0, 1.5);
  frame_.noiseLevel = std::clamp (frame_.noiseLevel, 0.0, 2.5);
  frame_.pitchEnvAmount = clamp01 (frame_.pitchEnvAmount);
  frame_.noiseAmount = std::clamp (frame_.noiseAmount, 0.0, 2.5);
  frame_.noiseFilterCutoffHz = std::clamp (frame_.noiseFilterCutoffHz, 120.0, 18000.0);
  frame_.transientAmount = clamp01 (frame_.transientAmount);
  frame_.transientLevel = std::clamp (frame_.transientLevel, 0.0, 2.5);
  frame_.transientMix = std::clamp (frame_.transientMix, 0.0, 1.4);
  frame_.snapAmount = clamp01 (frame_.snapAmount);
  frame_.noiseTone = std::clamp (frame_.noiseTone, -1.0, 1.0);
  frame_.noiseResonance = std::clamp (frame_.noiseResonance, 0.0, 0.98);
  frame_.noiseEnvAmount = std::clamp (frame_.noiseEnvAmount, 0.0, 1.5);
  frame_.level = std::clamp (frame_.level, 0.0, 1.5);
  frame_.foldAmount = clamp01 (frame_.foldAmount);
  frame_.fmAmount = clamp01 (frame_.fmAmount);
  frame_.driveAmount = clamp01 (frame_.driveAmount);
  frame_.decaySeconds = std::clamp (frame_.decaySeconds, 0.01, 2.5);
  frame_.pitchEnvDecaySeconds = std::clamp (frame_.pitchEnvDecaySeconds, 0.004, 0.8);
  frame_.noiseDecaySeconds = std::clamp (frame_.noiseDecaySeconds, 0.004, 1.8);
  frame_.transientDecaySeconds = std::clamp (frame_.transientDecaySeconds, 0.0015, 0.5);
  frame_.oscFilterCutoff = clamp01 (frame_.oscFilterCutoff);
  frame_.oscFilterResonance = std::clamp (frame_.oscFilterResonance, 0.0, 0.96);
  frame_.oscFilterEnvAmount = clamp01 (frame_.oscFilterEnvAmount);
  frame_.transFilterCutoff = clamp01 (frame_.transFilterCutoff);
  frame_.transFilterResonance = std::clamp (frame_.transFilterResonance, 0.0, 0.96);
  frame_.transFilterEnvAmount = clamp01 (frame_.transFilterEnvAmount);

  const double ampTau = frame_.decaySeconds;
  const double toneTau = std::max (0.01, frame_.decaySeconds * 0.28);
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
  const double baseNoiseCutoff = frame_.noiseFilterCutoffHz * (0.40 + (tone01 * 0.70));
  const double lowCutoffHz = std::clamp (baseNoiseCutoff * 0.95, 180.0, 14000.0);
  const double highCutoffHz = std::clamp (baseNoiseCutoff * 0.50, 100.0, 9000.0);
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

  // --- OSCILLATOR PATH ---
  const double pitchSemitoneSweep = frame_.pitchEnvAmount * pitchEnv_ * kPitchSemitoneSpan[character];
  const double pitchRatio = std::pow (2.0, pitchSemitoneSweep / 12.0);

  const double modFrequency =
    frame_.frequencyHz * pitchRatio * (1.15 + (frame_.fmAmount * (4.8 * kFmScale[character])));
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

  const double oscBaseCutoff = cutoffFromNormalized (frame_.oscFilterCutoff, 80.0, 18000.0);
  const double oscEnvMod = oscBaseCutoff * frame_.oscFilterEnvAmount * toneEnv_ * 3.0;
  const double oscCutoffHz = std::clamp (oscBaseCutoff + oscEnvMod, 20.0, sampleRate_ * 0.47);
  body = processStateVariableLowpass (body, oscCutoffHz, frame_.oscFilterResonance,
                                      oscFilterLowState_, oscFilterBandState_);
  const double oscGate = ampEnv_ * (0.32 + (0.68 * toneEnv_));
  const double oscOut = body * frame_.oscLevel * oscGate * kBodyGain[character];

  // --- TRANSIENT PATH ---
  const double transientOsc = std::sin (transientPhase_);
  const double transientNoise = randomBipolar ();
  const double transientBlend =
    std::clamp ((frame_.transientMix * 0.65) + (frame_.snapAmount * kNoiseTransientBlend[character]), 0.0, 1.0);
  const double transientCore = (transientOsc * (1.0 - transientBlend)) + (transientNoise * transientBlend);

  const double transBaseCutoff = cutoffFromNormalized (frame_.transFilterCutoff, 400.0, 18000.0);
  const double transEnvMod = transBaseCutoff * frame_.transFilterEnvAmount * transientEnv_ * 2.5;
  const double transCutoffHz = std::clamp (transBaseCutoff + transEnvMod, 20.0, sampleRate_ * 0.47);
  const double filteredTransient = processStateVariableLowpass (transientCore, transCutoffHz,
                                                                frame_.transFilterResonance,
                                                                transFilterLowState_, transFilterBandState_);

  const double transientGain = (0.35 + (frame_.transientAmount * 1.2)) * frame_.transientLevel;
  const double transOut = filteredTransient * transientEnv_ * transientGain;

  // --- NOISE PATH ---
  const double rawNoise = randomBipolar ();
  noiseLowState_ += noiseLpCoef_ * (rawNoise - noiseLowState_);
  noiseHighState_ += noiseHpCoef_ * (rawNoise - noiseHighState_);
  const double highNoise = rawNoise - noiseHighState_;
  const double toneBlend = clamp01 ((frame_.noiseTone + 1.0) * 0.5);
  const double shapedNoise = ((1.0 - toneBlend) * noiseLowState_) + (toneBlend * highNoise);

  const double snapExponent = std::clamp (0.85 - (frame_.snapAmount * 0.55), 0.25, 1.0);
  const double snappyEnv = std::pow (std::max (noiseEnv_, 0.0), snapExponent);
  const double noiseContour = ((1.0 - frame_.snapAmount) * noiseEnv_) + (frame_.snapAmount * snappyEnv);
  const double noiseCutoffBase = frame_.noiseFilterCutoffHz * (0.45 + (toneBlend * 0.90));
  const double noiseCutoffEnv = std::clamp (noiseCutoffBase * (0.60 + (noiseContour * (1.1 + (frame_.noiseEnvAmount * 2.4)))),
                                            180.0, 19000.0);
  const double noiseResonance = std::clamp (frame_.noiseResonance + (frame_.snapAmount * 0.16), 0.0, 0.98);
  const double resonantNoise = processStateVariableLowpass (shapedNoise, noiseCutoffEnv, noiseResonance,
                                                            noiseResLowState_, noiseResBandState_);
  const double noiseOut =
    resonantNoise * frame_.noiseAmount * frame_.noiseLevel * noiseContour * kNoiseBlendGain[character];

  // --- SUMMING ---
  const double noiseGainWeight = std::max (0.15, frame_.noiseLevel);
  const double transientGainWeight = std::max (0.12, frame_.transientLevel);
  const double componentPower =
    std::max ((frame_.oscLevel * frame_.oscLevel) + (noiseGainWeight * noiseGainWeight) +
                (transientGainWeight * transientGainWeight),
              1e-6);
  const double normalization = 1.0 / std::sqrt (componentPower);

  const double drive = 1.0 + (frame_.driveAmount * 8.0);
  const double dryMix = (oscOut + noiseOut + transOut) * normalization;
  const double sample = softClip (dryMix * drive) * frame_.outputLevel;

  // --- ENVELOPE DECAY ---
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
  oscFilterLowState_ = 0.0;
  oscFilterBandState_ = 0.0;
  transFilterLowState_ = 0.0;
  transFilterBandState_ = 0.0;
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
  const double f = std::clamp ((2.0 * std::sin ((kPi * clippedCutoff) / sampleRate_)), 0.0, 1.0);
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
