#pragma once

#include <cstdint>

namespace SineLanguage {
namespace WestCoastDrumSynth {

class NoiseGenerator {
public:
    NoiseGenerator() = default;

    void setSampleRate(float sr) { sampleRate_ = sr; }
    void setFilterFreq(float freq) { filterFreq_ = freq; }

    float white();
    float filtered();
    float metallic(float freq);
    void reset();

private:
    float sampleRate_ = 44100.0f;
    float filterFreq_ = 8000.0f;

    uint32_t seed_ = 22222;
    float lp_ = 0.0f;
    float hp_ = 0.0f;
    float prevSample_ = 0.0f;

    float metalPhase_[6] = {};

    float nextRandom();
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
