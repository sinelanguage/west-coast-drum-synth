#include "LowPassGate.h"
#include <algorithm>

namespace SineLanguage {
namespace WestCoastDrumSynth {

void LowPassGate::trigger(float velocity) {
    vacTarget_ = velocity * amount_;
    vacLevel_ = velocity * amount_;
}

void LowPassGate::reset() {
    vacLevel_ = 0.0f;
    vacTarget_ = 0.0f;
    lp1_ = 0.0f;
    lp2_ = 0.0f;
    bp_ = 0.0f;
}

float LowPassGate::process(float input) {
    // Vactrol model: LED/photoresistor with nonlinear response
    // The vactrol has a fast attack (~0.5ms) and slow, frequency-dependent release
    float decayTime = 0.01f + decay_ * decay_ * 3.0f;
    float releaseCoeff = std::exp(-1.0f / (sampleRate_ * decayTime));

    // Nonlinear vactrol decay - slows down as level decreases
    float levelScaled = vacTarget_ * vacTarget_;
    float adaptiveRelease = releaseCoeff + (1.0f - releaseCoeff) * (1.0f - levelScaled) * 0.3f;
    vacTarget_ *= adaptiveRelease;

    // Vactrol slew rate asymmetry: fast up, slow down with variable curve
    float attackTime = 0.0005f;
    float releaseSlew = 0.015f + decay_ * 0.05f;
    float slewUp = 1.0f - std::exp(-1.0f / (sampleRate_ * attackTime));
    float slewDown = 1.0f - std::exp(-1.0f / (sampleRate_ * releaseSlew));
    float slew = (vacTarget_ > vacLevel_) ? slewUp : slewDown;
    vacLevel_ += (vacTarget_ - vacLevel_) * slew;

    float level = std::max(0.0f, std::min(1.0f, vacLevel_));

    // Combined VCA + VCF behavior (the signature of a Low Pass Gate)
    // Cutoff tracks the vactrol level with a squared response
    float minFreq = 40.0f;
    float maxFreq = 16000.0f;
    float cutoff = minFreq + level * level * (maxFreq - minFreq);
    float w = 2.0f * 3.14159265f * cutoff / sampleRate_;
    float g = std::tan(std::min(w * 0.5f, 1.5f));
    float k = 2.0f - 2.0f * resonance_ * 0.9f;
    k = std::max(0.1f, k);

    // State-variable filter
    float hp = (input - lp2_ - k * bp_) / (1.0f + k * g + g * g);
    float newBp = bp_ + g * hp;
    float newLp = lp2_ + g * newBp;
    bp_ = newBp;
    lp2_ = newLp;

    // VCA portion controlled by vactrol
    float output = lp2_ * level;

    return output;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
