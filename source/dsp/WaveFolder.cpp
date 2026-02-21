#include "WaveFolder.h"
#include <algorithm>

namespace SineLanguage {
namespace WestCoastDrumSynth {

float WaveFolder::softClip(float x) {
    return std::tanh(x);
}

float WaveFolder::buchlaFold(float x, float amount) {
    if (amount < 0.01f) return x;

    float gain = 1.0f + amount * 12.0f;
    x *= gain;

    // Multi-stage folding inspired by Buchla 259 timbre circuit
    float stage1 = std::sin(x);

    float stage2 = x;
    const float threshold = 1.0f;
    while (stage2 > threshold) stage2 = threshold - (stage2 - threshold);
    while (stage2 < -threshold) stage2 = -threshold - (stage2 + threshold);

    // Polynomial waveshaping (Chebyshev-like harmonics)
    float x2 = x * x;
    float poly = x - 0.166667f * x * x2 + 0.00833333f * x * x2 * x2;
    poly = std::max(-1.0f, std::min(1.0f, poly));

    // Blend stages based on amount for evolving timbre
    float blend;
    if (amount < 0.33f) {
        float t = amount / 0.33f;
        blend = stage2 * (1.0f - t) + stage1 * t;
    } else if (amount < 0.66f) {
        float t = (amount - 0.33f) / 0.33f;
        blend = stage1 * (1.0f - t) + poly * t;
    } else {
        float t = (amount - 0.66f) / 0.34f;
        // At extreme settings, combine all stages for complex timbres
        blend = poly * (1.0f - t * 0.5f) + stage1 * t * 0.3f + stage2 * t * 0.2f;
    }

    return softClip(blend);
}

float WaveFolder::process(float input) {
    float driven = input * (1.0f + drive_ * 8.0f);

    // Asymmetric waveshaping based on symmetry control
    float symOffset = (symmetry_ - 0.5f) * 2.0f;
    if (symOffset > 0.0f) {
        float pos = std::max(0.0f, driven);
        float neg = std::min(0.0f, driven);
        driven = pos * (1.0f + symOffset * 0.5f) + neg * (1.0f - symOffset * 0.3f);
    } else {
        float pos = std::max(0.0f, driven);
        float neg = std::min(0.0f, driven);
        driven = pos * (1.0f + symOffset * 0.3f) + neg * (1.0f - symOffset * 0.5f);
    }

    float folded = buchlaFold(driven, foldAmount_);

    return softClip(folded);
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
