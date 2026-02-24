#pragma once

#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

// Emulates a Buchla-style Low Pass Gate (combined VCA/VCF)
// using a vactrol-modeled response with resonance
class LowPassGate {
public:
    LowPassGate() = default;

    void setSampleRate(float sr) { sampleRate_ = sr; }
    void setAmount(float amt) { amount_ = amt; }
    void setDecay(float decay) { decay_ = decay; }
    void setResonance(float res) { resonance_ = res; }

    void trigger(float velocity);
    float process(float input);
    void reset();

private:
    float sampleRate_ = 44100.0f;
    float amount_ = 1.0f;
    float decay_ = 0.3f;
    float resonance_ = 0.0f;

    // Vactrol envelope state
    float vacLevel_ = 0.0f;
    float vacTarget_ = 0.0f;

    // Filter state
    float lp1_ = 0.0f;
    float lp2_ = 0.0f;
    float bp_ = 0.0f;
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
