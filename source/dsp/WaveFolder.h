#pragma once

#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

class WaveFolder {
public:
    WaveFolder() = default;

    void setFoldAmount(float amount) { foldAmount_ = amount; }
    void setSymmetry(float sym) { symmetry_ = sym; }
    void setDrive(float drive) { drive_ = drive; }

    float process(float input);

private:
    float foldAmount_ = 0.0f;
    float symmetry_ = 0.5f;
    float drive_ = 0.0f;

    float buchlaFold(float x, float amount);
    float softClip(float x);
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
