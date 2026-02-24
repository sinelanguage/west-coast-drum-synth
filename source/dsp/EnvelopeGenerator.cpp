#include "EnvelopeGenerator.h"
#include <algorithm>

namespace SineLanguage {
namespace WestCoastDrumSynth {

void EnvelopeGenerator::trigger(float velocity) {
    velocity_ = velocity;
    level_ = 1.0f;
    active_ = true;

    float decayTimeSec = 0.005f + decay_ * decay_ * 4.0f;
    if (type_ == Type::Pitch) {
        decayTimeSec = 0.002f + decay_ * decay_ * 0.5f;
    }
    decayCoeff_ = std::exp(-1.0f / (sampleRate_ * decayTimeSec));
}

float EnvelopeGenerator::process() {
    if (!active_) return 0.0f;

    float output = level_;

    // Apply curve shaping
    if (curve_ != 1.0f) {
        output = std::pow(output, curve_);
    }

    output *= amount_ * velocity_;
    level_ *= decayCoeff_;

    if (level_ < 0.0001f) {
        level_ = 0.0f;
        active_ = false;
    }

    return output;
}

void EnvelopeGenerator::reset() {
    level_ = 0.0f;
    active_ = false;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
