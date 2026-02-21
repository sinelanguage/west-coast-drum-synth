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
    // Vactrol-style envelope: fast attack, slow logarithmic release
    float decayTime = 0.01f + decay_ * 2.0f;
    float releaseCoeff = std::exp(-1.0f / (sampleRate_ * decayTime));

    vacTarget_ *= releaseCoeff;
    // Vactrol slew: the response lags behind the target
    float slewUp = 1.0f - std::exp(-1.0f / (sampleRate_ * 0.001f));
    float slewDown = 1.0f - std::exp(-1.0f / (sampleRate_ * 0.02f));
    float slew = (vacTarget_ > vacLevel_) ? slewUp : slewDown;
    vacLevel_ += (vacTarget_ - vacLevel_) * slew;

    float level = std::max(0.0f, std::min(1.0f, vacLevel_));

    // Cutoff frequency modulated by vactrol level
    float minFreq = 60.0f;
    float maxFreq = 18000.0f;
    float cutoff = minFreq + level * level * (maxFreq - minFreq);
    float w = 2.0f * 3.14159265f * cutoff / sampleRate_;
    float g = std::tan(w * 0.5f);
    float k = 2.0f - 2.0f * resonance_ * 0.95f;

    // State-variable filter (2-pole)
    float hp = (input - lp2_ - k * bp_) / (1.0f + k * g + g * g);
    bp_ = bp_ + g * hp;
    lp2_ = lp2_ + g * bp_;

    // Mix between filtered and dry based on amount
    float filtered = lp2_;
    float output = filtered * level;

    return output;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
