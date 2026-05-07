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
  constexpr double kSoftClipDrive = 1.42;
  return std::tanh (x * kSoftClipDrive) / std::tanh (kSoftClipDrive);
}

// Indexed by LaneCharacter: Kick, Snare, Hat, PercA, PercB, RimShot, Clap
constexpr std::array<double, 7> kPitchSemitoneSpan {66.0, 20.0, 8.0, 36.0, 32.0, 18.0, 14.0};
constexpr std::array<double, 7> kTransientBaseHz {1100.0, 2200.0, 7000.0, 1500.0, 2800.0, 2800.0, 2200.0};
constexpr std::array<double, 7> kTransientPitchRatio {8.0, 10.0, 28.0, 9.0, 13.0, 15.0, 10.0};
constexpr std::array<double, 7> kNoiseBlendGain {1.85, 2.45, 2.55, 2.10, 2.20, 2.30, 2.55};
constexpr std::array<double, 7> kBodyGain {1.05, 0.84, 0.42, 1.02, 0.96, 0.92, 0.88};
constexpr std::array<double, 7> kFmScale {1.0, 0.85, 0.40, 0.92, 0.88, 0.65, 0.45};
constexpr std::array<double, 7> kNoiseTransientBlend {0.62, 0.68, 0.38, 0.65, 0.58, 0.72, 0.55};
constexpr std::array<double, 7> kTransientGainBoost {2.40, 1.55, 1.05, 2.05, 1.85, 1.35, 1.30};
// Per-character thump (low-frequency knock) weighting and pitch ratios.
// Kick/PercA dominate; hat/clap/rim are minimal so the click stays bright.
constexpr std::array<double, 7> kThumpGain {1.55, 0.95, 0.0, 1.30, 1.10, 0.30, 0.40};
constexpr std::array<double, 7> kThumpPitchRatio {1.55, 1.20, 1.00, 1.40, 1.30, 1.10, 1.10};
constexpr std::array<double, 7> kThumpBaseFloorHz {38.0, 95.0, 0.0, 60.0, 75.0, 110.0, 130.0};
constexpr std::array<double, 7> kThumpDecayBase {0.040, 0.022, 0.001, 0.030, 0.026, 0.012, 0.012};
constexpr std::array<double, 7> kThumpPitchDecay {0.0065, 0.0045, 0.0010, 0.0055, 0.0050, 0.0030, 0.0030};

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
  // Store previous output for anti-click crossfade if voice was active
  if (active_ && ampEnv_ > 0.001)
  {
    antiClickLength_ = std::max (32, static_cast<int32_t> (sampleRate_ * 0.0008));
    antiClickSamples_ = antiClickLength_;
  }
  else
  {
    antiClickSamples_ = 0;
    antiClickLength_ = 0;
    antiClickPrevSample_ = 0.0;
  }

  frame_ = frame;
  frame_.frequencyHz = std::clamp (frame_.frequencyHz, 8.0, 18000.0);
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
    std::clamp (frame_.transientDecaySeconds * (1.02 - (0.38 * frame_.snapAmount)), 0.0012, 0.5);
  ampDecayCoef_ = std::exp (-1.0 / (ampTau * sampleRate_));
  toneDecayCoef_ = std::exp (-1.0 / (toneTau * sampleRate_));
  pitchDecayCoef_ = std::exp (-1.0 / (pitchTau * sampleRate_));
  noiseDecayCoef_ = std::exp (-1.0 / (noiseTau * sampleRate_));
  transientDecayCoef_ = std::exp (-1.0 / (transientTau * sampleRate_));
  const double clickTauSec = std::clamp (0.00028 + (frame_.snapAmount * 0.00045), 0.00015, 0.0016);
  clickDecayCoef_ = std::exp (-1.0 / (clickTauSec * sampleRate_));
  clickEnv_ = 1.0;

  // Low-frequency knock/thump layer (for 909-style kick knock, 808 click body).
  // Tied to the voice's fundamental so it remains audible at sub frequencies.
  const size_t thumpChar = std::min (characterIndex (frame_.character), size_t {6});
  thumpBaseHz_ = std::max (kThumpBaseFloorHz[thumpChar], frame_.frequencyHz);
  thumpPeakHz_ = thumpBaseHz_ * kThumpPitchRatio[thumpChar];
  const double thumpTau =
    std::clamp (kThumpDecayBase[thumpChar] * (0.55 + (frame_.transientAmount * 1.10)),
                0.002, 0.080);
  const double thumpPitchTau =
    std::clamp (kThumpPitchDecay[thumpChar] * (0.85 + (frame_.snapAmount * 0.50)), 0.0008, 0.020);
  thumpDecayCoef_ = std::exp (-1.0 / (thumpTau * sampleRate_));
  thumpPitchDecayCoef_ = std::exp (-1.0 / (thumpPitchTau * sampleRate_));
  thumpPhase_ = 0.0;
  thumpEnv_ = (kThumpGain[thumpChar] > 0.0) ? 1.0 : 0.0;
  thumpPitchEnv_ = 1.0;

  const double tone01 = clamp01 ((frame_.noiseTone + 1.0) * 0.5);
  const double baseNoiseCutoff = frame_.noiseFilterCutoffHz * (0.40 + (tone01 * 0.70));
  const double lowCutoffHz = std::clamp (baseNoiseCutoff * 0.95, 180.0, 14000.0);
  const double highCutoffHz = std::clamp (baseNoiseCutoff * 0.50, 100.0, 9000.0);
  noiseLpCoef_ = 1.0 - std::exp (-(kTwoPi * lowCutoffHz) / sampleRate_);
  noiseHpCoef_ = 1.0 - std::exp (-(kTwoPi * highCutoffHz) / sampleRate_);
  noiseLpCoef_ = std::clamp (noiseLpCoef_, 0.0, 1.0);
  noiseHpCoef_ = std::clamp (noiseHpCoef_, 0.0, 1.0);

  // Clear filter states on trigger to prevent stale resonance
  oscFilterLowState_ = 0.0;
  oscFilterBandState_ = 0.0;
  transFilterLowState_ = 0.0;
  transFilterBandState_ = 0.0;
  noiseResLowState_ = 0.0;
  noiseResBandState_ = 0.0;

  carrierPhase_ = 0.0;
  modPhase_ = 0.0;
  transientPhase_ = 0.0;

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

  const size_t character = std::min (characterIndex (frame_.character), size_t {6});

  // --- OSCILLATOR PATH ---
  const double pitchSemitoneSweep = frame_.pitchEnvAmount * pitchEnv_ * kPitchSemitoneSpan[character];
  const double pitchRatio = std::pow (2.0, pitchSemitoneSweep / 12.0);

  const double modFrequency =
    frame_.frequencyHz * pitchRatio * (1.0 + (frame_.fmAmount * (5.5 * kFmScale[character])));
  const double carrierFrequency = frame_.frequencyHz * pitchRatio * (1.0 + (toneEnv_ * 0.07));

  modPhase_ += (kTwoPi * modFrequency) / sampleRate_;
  carrierPhase_ += (kTwoPi * carrierFrequency) / sampleRate_;
  const double transFreqSweep = 0.85 + (frame_.transientAmount * 1.4);
  const double transientFrequency =
    std::max (kTransientBaseHz[character], frame_.frequencyHz * kTransientPitchRatio[character]) * transFreqSweep;
  transientPhase_ += (kTwoPi * transientFrequency) / sampleRate_;
  modPhase_ = wrapPhase (modPhase_);
  carrierPhase_ = wrapPhase (carrierPhase_);
  transientPhase_ = wrapPhase (transientPhase_);

  // Thru-zero FM: modulation depth allows phase reversal (negative instantaneous freq)
  const double fmDepth = frame_.fmAmount * ((12.0 * toneEnv_ * kFmScale[character]) + 0.5);
  const double modSignal = std::sin (modPhase_) * fmDepth;
  double body = std::sin (carrierPhase_ + modSignal);

  // Wavefolding: more dynamic range (1 + amount*16), 5 folds for richer harmonics
  const double dynamicFold = frame_.foldAmount * (1.2 + (1.2 * toneEnv_));
  body = wavefold (body, dynamicFold);

  // Body filter: now actually closes down to a useful low minimum and modulates
  // gently so the cutoff knob is audibly effective across its full range.
  // The global Osc Body Cutoff (frame_.bodyFilterCutoffHz) defines the upper
  // ceiling; the per-lane Osc Cutoff knob (frame_.oscFilterCutoff) sweeps from
  // a real closed value (60 Hz) up to that ceiling.
  const double oscClosedHz = 60.0;
  const double oscOpenHz = std::max (oscClosedHz + 20.0, frame_.bodyFilterCutoffHz);
  const double oscNorm = clamp01 (frame_.oscFilterCutoff);
  // Exponential sweep so the lower half of the knob still spans 60..1000 Hz audibly.
  const double oscBaseCutoff = oscClosedHz * std::pow (oscOpenHz / oscClosedHz, oscNorm);
  // Reduce env-mod multiplier so the filter doesn't blast wide open at trigger.
  const double oscEnvMod =
    oscBaseCutoff * frame_.bodyFilterEnvAmount * frame_.oscFilterEnvAmount * toneEnv_ * 0.95;
  const double oscCutoffHz = std::clamp (oscBaseCutoff + oscEnvMod, 30.0, sampleRate_ * 0.47);
  const double oscResScaled =
    std::min (0.92, (frame_.bodyFilterResonance * 0.5 + frame_.oscFilterResonance * 0.5) * 0.92);
  body = processStateVariableLowpass (body, oscCutoffHz, oscResScaled,
                                      oscFilterLowState_, oscFilterBandState_);
  const double oscGate = ampEnv_ * (0.38 + (0.72 * toneEnv_));
  const double oscOut = body * frame_.oscLevel * oscGate * kBodyGain[character] * 1.41;

  // Global tone-tilt derived from the Osc Body Cutoff position. Maps the body
  // cutoff to a 0..1 "darkness" amount; at fully closed body the noise/transient
  // get gently darkened too, so the global cutoff knob always changes character.
  // Smooth, lightweight, no per-sample state cost — just attenuates highs.
  const double bodyCeilingNorm =
    clamp01 (std::log (oscOpenHz / 200.0) / std::log (16000.0 / 200.0));
  const double globalToneTilt = clamp01 (bodyCeilingNorm * 0.55 + oscNorm * 0.45);

  // --- TRANSIENT PATH ---
  const double transientOsc = std::sin (transientPhase_);
  const double transientNoise = randomBipolar ();
  const double transientBlend =
    std::clamp ((frame_.transientMix * 0.65) + (frame_.snapAmount * kNoiseTransientBlend[character]), 0.0, 1.0);
  const double transientCore = (transientOsc * (1.0 - transientBlend)) + (transientNoise * transientBlend);
  const double transientExciter = softClip (transientCore * (1.8 + (frame_.snapAmount * 1.3)));

  const double transBaseCutoff = cutoffFromNormalized (frame_.transFilterCutoff, 95.0, 20000.0);
  const double transEnvMod =
    transBaseCutoff * (0.35 + (frame_.transientAmount * 0.45)) * frame_.transFilterEnvAmount * transientEnv_ * 2.8;
  const double transCutoffHz = std::clamp (transBaseCutoff + transEnvMod, 70.0, sampleRate_ * 0.43);
  const double transResScaled =
    std::min (0.985, 0.35 + (frame_.transFilterResonance * 0.52) + (frame_.transientAmount * 0.18));
  const double filteredTransient = processStateVariableLowpass (transientExciter, transCutoffHz,
                                                                transResScaled,
                                                                transFilterLowState_, transFilterBandState_);

  // Low-frequency thump layer: a pitched sine that sweeps from peak->base over
  // a few ms. Drives the audible "knock" body for kicks at any fundamental and
  // gives 808/909-style click weight. Disabled (gain=0) for hat to keep it crisp.
  double thumpOut = 0.0;
  if (kThumpGain[character] > 0.0)
  {
    const double thumpFreq =
      thumpBaseHz_ + (thumpPeakHz_ - thumpBaseHz_) * std::pow (std::max (thumpPitchEnv_, 0.0), 0.85);
    thumpPhase_ += (kTwoPi * thumpFreq) / sampleRate_;
    thumpPhase_ = wrapPhase (thumpPhase_);
    const double thumpOsc = std::sin (thumpPhase_);
    // Soft saturate the thump for a touch of body & to keep peaks tame.
    const double thumpDriven = std::tanh (thumpOsc * (1.25 + frame_.snapAmount * 0.85));
    thumpOut = thumpDriven * thumpEnv_ * kThumpGain[character];
    thumpEnv_ *= thumpDecayCoef_;
    thumpPitchEnv_ *= thumpPitchDecayCoef_;
  }

  const double transientGain =
    (0.72 + (frame_.transientAmount * 1.9)) * frame_.transientLevel * kTransientGainBoost[character];
  const double knockWeight = 1.0 + (std::pow (1.0 - clamp01 (frame_.transFilterCutoff), 0.72) * 1.8);
  const double clickAmt = (0.62 + (frame_.snapAmount * 0.72)) * (0.68 + (frame_.transientAmount * 1.35));
  const double clickRaw = transientExciter * clickEnv_ * clickAmt;
  clickEnv_ *= clickDecayCoef_;
  // Sum the transient bus pre-saturation, then drive through tanh AFTER applying
  // gain so loud settings audibly saturate (musical click bite) while bounding
  // the per-voice peak. Output stays roughly under ~1.5 regardless of user gain.
  const double transientBus =
    (filteredTransient * transientEnv_ * knockWeight) + thumpOut + clickRaw;
  // Mild global tone tilt on transient (less than noise so percussive bite stays).
  // tilt=0 -> 0.72x, tilt=1 -> 1.00x. Thump component is mostly low-end so
  // it stays prominent even when the body filter is closed.
  const double transientToneTilt = 0.72 + (globalToneTilt * 0.28);
  const double transientDriven = transientBus * transientGain;
  const double transOut = std::tanh (transientDriven * 0.42) * 1.55 * transientToneTilt;

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
  // Resonant LPFs with high Q lose perceived energy on the unfiltered noise floor.
  // Add ~+6 dB make-up at high resonance to keep the noise audible everywhere.
  const double noiseResMakeup = 1.0 + (noiseResonance * noiseResonance * 1.10);
  // Loudness law tuned so the user's noise-level knob has obvious effect at any
  // setting. Floor raised vs. previous (0.40 vs 0.24) so a small noise value is
  // still audible against body+transient. Slope steeper so the knob has reach.
  // Product still goes to 0 if both factor inputs are 0.
  const double noisePresence =
    (0.40 + (frame_.noiseAmount * 1.05)) * (0.78 + (frame_.noiseLevel * 0.55));
  // Apply the global tone-tilt so the body cutoff knob darkens noise too.
  // tilt=0 (body closed) -> attenuate noise to 0.55x; tilt=1 (open) -> 1.00x.
  const double noiseToneAttenuation = 0.55 + (globalToneTilt * 0.45);
  const double noiseOut = resonantNoise * noisePresence * noiseContour * kNoiseBlendGain[character] *
                          noiseResMakeup * noiseToneAttenuation;

  // --- SUMMING (per-voice, no dynamic normalization) ---
  const double drive = 1.0 + (frame_.driveAmount * 6.0);
  const double rawMix = oscOut + noiseOut + transOut;
  double sample = softClip (rawMix * drive) * frame_.outputLevel;

  // Anti-click crossfade: blend previous tail with new attack
  if (antiClickSamples_ > 0 && antiClickLength_ > 0)
  {
    const double fade = static_cast<double> (antiClickSamples_) / static_cast<double> (antiClickLength_);
    sample = sample * (1.0 - fade) + antiClickPrevSample_ * fade;
    antiClickPrevSample_ *= 0.92;
    --antiClickSamples_;
  }

  // DC blocker (leaky integrator, ~5 Hz cutoff)
  constexpr double kDcCoef = 0.9995;
  const double dcIn = sample;
  dcY_ = dcIn - dcX_ + kDcCoef * dcY_;
  dcX_ = dcIn;
  sample = dcY_;

  // --- ENVELOPE DECAY ---
  ampEnv_ *= ampDecayCoef_;
  toneEnv_ *= toneDecayCoef_;
  pitchEnv_ *= pitchDecayCoef_;
  noiseEnv_ *= noiseDecayCoef_;
  transientEnv_ *= transientDecayCoef_;

  if (ampEnv_ < 0.00008 && noiseEnv_ < 0.00008 && transientEnv_ < 0.00008 &&
      clickEnv_ < 0.00002 && thumpEnv_ < 0.00008)
  {
    active_ = false;
    ampEnv_ = 0.0;
    toneEnv_ = 0.0;
    pitchEnv_ = 0.0;
    noiseEnv_ = 0.0;
    transientEnv_ = 0.0;
    thumpEnv_ = 0.0;
    thumpPitchEnv_ = 0.0;
    clickEnv_ = 0.0;
  }

  return sample;
}

void DrumVoice::reset ()
{
  carrierPhase_ = 0.0;
  modPhase_ = 0.0;
  transientPhase_ = 0.0;
  thumpPhase_ = 0.0;
  ampEnv_ = 0.0;
  toneEnv_ = 0.0;
  pitchEnv_ = 0.0;
  noiseEnv_ = 0.0;
  transientEnv_ = 0.0;
  thumpEnv_ = 0.0;
  thumpPitchEnv_ = 0.0;
  clickEnv_ = 0.0;
  noiseLowState_ = 0.0;
  noiseHighState_ = 0.0;
  noiseResLowState_ = 0.0;
  noiseResBandState_ = 0.0;
  bodyLowState_ = 0.0;
  bodyBandState_ = 0.0;
  oscFilterLowState_ = 0.0;
  oscFilterBandState_ = 0.0;
  transFilterLowState_ = 0.0;
  transFilterBandState_ = 0.0;
  antiClickSamples_ = 0;
  antiClickLength_ = 0;
  antiClickPrevSample_ = 0.0;
  dcX_ = 0.0;
  dcY_ = 0.0;
  active_ = false;
}

bool DrumVoice::isActive () const
{
  return active_;
}

double DrumVoice::wavefold (double x, double amount)
{
  const double gain = 1.0 + (amount * 16.0);
  x *= gain;

  for (int i = 0; i < 5; ++i)
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
  const double maxCutoff = sampleRate_ * 0.43;
  const double clippedCutoff = std::clamp (cutoffHz, 20.0, maxCutoff);
  const double f = std::clamp (2.0 * std::sin (kPi * clippedCutoff / sampleRate_), 0.0, 0.95);
  const double q = std::clamp (resonance, 0.0, 0.96);
  const double damping = 1.0 - q;

  // Two-pass (2x oversampled) for stability at high cutoffs
  for (int pass = 0; pass < 2; ++pass)
  {
    const double high = input - lowState - (damping * bandState);
    bandState += 0.5 * f * high;
    lowState += 0.5 * f * bandState;
  }

  // Flush denormals + clamp filter states to prevent runaway. Adding a tiny
  // anti-denormal offset keeps the IEEE-754 path on normalized values when
  // the voice is silent for long stretches.
  constexpr double kDenormalGuard = 1.0e-25;
  bandState = std::clamp (bandState + kDenormalGuard, -8.0, 8.0);
  lowState = std::clamp (lowState + kDenormalGuard, -8.0, 8.0);

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
