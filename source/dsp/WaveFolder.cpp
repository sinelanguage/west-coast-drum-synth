#include "WaveFolder.h"
#include <algorithm>

namespace SineLanguage {
namespace WestCoastDrumSynth {

float WaveFolder::softClip(float x) {
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return 1.5f * x - 0.5f * x * x * x;
}

float WaveFolder::buchlaFold(float x, float amount) {
    if (amount < 0.01f) return x;

    float gain = 1.0f + amount * 15.0f;
    x *= gain;

    float folded = x;
    int numFolds = static_cast<int>(amount * 4.0f) + 1;
    for (int i = 0; i < numFolds; ++i) {
        folded = std::sin(folded * (1.0f + amount * 0.5f));
    }

    float sinFold = std::sin(x * 3.14159265f * (1.0f + amount * 2.0f));
    float mix = amount;
    folded = folded * (1.0f - mix * 0.5f) + sinFold * mix * 0.5f;

    return softClip(folded);
}

float WaveFolder::process(float input) {
    float driven = input * (1.0f + drive_ * 10.0f);

    float symOffset = (symmetry_ - 0.5f) * 2.0f;
    driven += symOffset * 0.3f;

    float folded = buchlaFold(driven, foldAmount_);

    folded -= symOffset * 0.15f;

    return softClip(folded);
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
