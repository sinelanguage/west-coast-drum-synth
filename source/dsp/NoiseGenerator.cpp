#include "NoiseGenerator.h"
#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

float NoiseGenerator::nextRandom() {
    seed_ = seed_ * 1664525u + 1013904223u;
    return static_cast<float>(seed_) / static_cast<float>(0xFFFFFFFFu) * 2.0f - 1.0f;
}

float NoiseGenerator::white() {
    return nextRandom();
}

float NoiseGenerator::filtered() {
    float noise = nextRandom();

    float w = 2.0f * 3.14159265f * filterFreq_ / sampleRate_;
    float g = w / (1.0f + w);

    lp_ += g * (noise - lp_);
    float hpOut = noise - lp_;

    // Bandpass-ish: mix of LP and HP
    float output = lp_ * 0.7f + hpOut * 0.3f;
    return output;
}

float NoiseGenerator::metallic(float freq) {
    // Inharmonic metallic tone from multiple detuned square-ish oscillators
    const float ratios[6] = { 1.0f, 1.4983f, 1.7432f, 1.9858f, 2.4876f, 2.9931f };
    const float twoPi = 6.28318530718f;

    float sum = 0.0f;
    for (int i = 0; i < 6; ++i) {
        float f = freq * ratios[i];
        float inc = f / sampleRate_;
        metalPhase_[i] += inc;
        if (metalPhase_[i] >= 1.0f) metalPhase_[i] -= 1.0f;
        float s = std::sin(metalPhase_[i] * twoPi);
        // Square-ish waveshaping
        s = std::tanh(s * 3.0f);
        sum += s;
    }

    return sum / 6.0f;
}

void NoiseGenerator::reset() {
    lp_ = 0.0f;
    hp_ = 0.0f;
    prevSample_ = 0.0f;
    for (int i = 0; i < 6; ++i) metalPhase_[i] = 0.0f;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
