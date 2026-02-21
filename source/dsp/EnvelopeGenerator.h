#pragma once

#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

// Percussive envelope with exponential decay and optional pitch modulation
class EnvelopeGenerator {
public:
    enum class Type {
        Amplitude,
        Pitch
    };

    EnvelopeGenerator() = default;

    void setSampleRate(float sr) { sampleRate_ = sr; }
    void setDecay(float decay) { decay_ = decay; }
    void setAmount(float amount) { amount_ = amount; }
    void setType(Type t) { type_ = t; }
    void setCurve(float curve) { curve_ = curve; }

    void trigger(float velocity);
    float process();
    bool isActive() const { return level_ > 0.0001f; }
    void reset();

private:
    float sampleRate_ = 44100.0f;
    float decay_ = 0.2f;
    float amount_ = 1.0f;
    float curve_ = 1.0f;
    Type type_ = Type::Amplitude;

    float level_ = 0.0f;
    float velocity_ = 1.0f;
    float decayCoeff_ = 0.999f;
    bool active_ = false;
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
