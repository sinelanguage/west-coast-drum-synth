#pragma once

#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

class FMOperator {
public:
    FMOperator() = default;

    void setSampleRate(float sr) { sampleRate_ = sr; }
    void setFrequency(float freq) { frequency_ = freq; }
    void setRatio(float ratio) { ratio_ = ratio; }
    void setAmount(float amount) { fmAmount_ = amount; }

    float process(float modulation = 0.0f);
    void reset();

private:
    float sampleRate_ = 44100.0f;
    float frequency_ = 100.0f;
    float ratio_ = 1.0f;
    float fmAmount_ = 0.0f;

    float carrierPhase_ = 0.0f;
    float modPhase_ = 0.0f;
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
