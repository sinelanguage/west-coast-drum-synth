#include "FMOperator.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

float FMOperator::process(float modulation) {
    const float twoPi = 6.28318530718f;

    float modFreq = frequency_ * ratio_;
    float modInc = modFreq / sampleRate_;
    modPhase_ += modInc;
    if (modPhase_ >= 1.0f) modPhase_ -= 1.0f;

    float modOut = std::sin(modPhase_ * twoPi);
    float fmMod = modOut * fmAmount_ * frequency_ * 4.0f;

    float carrierFreq = frequency_ + fmMod + modulation;
    float carrierInc = carrierFreq / sampleRate_;
    carrierPhase_ += carrierInc;
    if (carrierPhase_ >= 1.0f) carrierPhase_ -= 1.0f;
    if (carrierPhase_ < 0.0f) carrierPhase_ += 1.0f;

    return std::sin(carrierPhase_ * twoPi);
}

void FMOperator::reset() {
    carrierPhase_ = 0.0f;
    modPhase_ = 0.0f;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
