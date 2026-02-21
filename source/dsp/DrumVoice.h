#pragma once

#include "WaveFolder.h"
#include "LowPassGate.h"
#include "EnvelopeGenerator.h"
#include "FMOperator.h"
#include "NoiseGenerator.h"
#include "wcparams.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

struct DrumVoiceParams {
    float level = 0.8f;
    float pitch = 0.5f;
    float decay = 0.3f;
    float fmAmount = 0.0f;
    float fmRatio = 0.5f;
    float foldAmount = 0.0f;
    float foldSymmetry = 0.5f;
    float lpgAmount = 1.0f;
    float lpgDecay = 0.3f;
    float lpgResonance = 0.0f;
    float noiseAmount = 0.0f;
    float pitchEnvAmount = 0.3f;
    float pitchEnvDecay = 0.2f;
    float drive = 0.0f;
    float pan = 0.5f;
    float waveShape = 0.0f;
};

class DrumVoice {
public:
    DrumVoice() = default;

    void setSampleRate(float sr);
    void setVoiceType(DrumVoiceType type) { voiceType_ = type; }
    void setParams(const DrumVoiceParams& p) { params_ = p; }

    void trigger(float velocity);
    void process(float& outL, float& outR);
    bool isActive() const;
    void reset();

    static DrumVoiceParams getDefaultParams(DrumVoiceType type);

private:
    DrumVoiceType voiceType_ = kKick;
    DrumVoiceParams params_;
    float sampleRate_ = 44100.0f;

    FMOperator fmOp_;
    WaveFolder waveFolder_;
    LowPassGate lpg_;
    EnvelopeGenerator ampEnv_;
    EnvelopeGenerator pitchEnv_;
    NoiseGenerator noise_;

    float getBaseFrequency() const;
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
