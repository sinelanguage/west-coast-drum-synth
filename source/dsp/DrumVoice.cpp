#include "DrumVoice.h"
#include <algorithm>
#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

void DrumVoice::setSampleRate(float sr) {
    sampleRate_ = sr;
    fmOp_.setSampleRate(sr);
    lpg_.setSampleRate(sr);
    ampEnv_.setSampleRate(sr);
    pitchEnv_.setSampleRate(sr);
    noise_.setSampleRate(sr);
}

float DrumVoice::getBaseFrequency() const {
    switch (voiceType_) {
        case kKick:      return 30.0f + params_.pitch * 80.0f;
        case kSnare:     return 100.0f + params_.pitch * 200.0f;
        case kClosedHat: return 800.0f + params_.pitch * 6000.0f;
        case kOpenHat:   return 600.0f + params_.pitch * 5000.0f;
        case kTom:       return 60.0f + params_.pitch * 300.0f;
        case kPerc:      return 150.0f + params_.pitch * 800.0f;
        default:         return 100.0f;
    }
}

DrumVoiceParams DrumVoice::getDefaultParams(DrumVoiceType type) {
    DrumVoiceParams p;
    switch (type) {
        case kKick:
            p.pitch = 0.35f; p.decay = 0.45f; p.fmAmount = 0.1f;
            p.fmRatio = 0.5f; p.foldAmount = 0.15f; p.lpgAmount = 0.9f;
            p.lpgDecay = 0.5f; p.pitchEnvAmount = 0.6f; p.pitchEnvDecay = 0.15f;
            p.level = 0.9f; p.drive = 0.1f;
            break;
        case kSnare:
            p.pitch = 0.4f; p.decay = 0.25f; p.fmAmount = 0.3f;
            p.fmRatio = 0.6f; p.foldAmount = 0.3f; p.lpgAmount = 0.8f;
            p.lpgDecay = 0.3f; p.noiseAmount = 0.55f; p.pitchEnvAmount = 0.3f;
            p.pitchEnvDecay = 0.08f; p.level = 0.85f; p.drive = 0.15f;
            break;
        case kClosedHat:
            p.pitch = 0.6f; p.decay = 0.08f; p.fmAmount = 0.7f;
            p.fmRatio = 0.73f; p.foldAmount = 0.5f; p.lpgAmount = 0.7f;
            p.lpgDecay = 0.08f; p.noiseAmount = 0.4f; p.pitchEnvAmount = 0.1f;
            p.pitchEnvDecay = 0.02f; p.level = 0.7f;
            break;
        case kOpenHat:
            p.pitch = 0.55f; p.decay = 0.4f; p.fmAmount = 0.65f;
            p.fmRatio = 0.71f; p.foldAmount = 0.45f; p.lpgAmount = 0.75f;
            p.lpgDecay = 0.4f; p.noiseAmount = 0.35f; p.pitchEnvAmount = 0.05f;
            p.pitchEnvDecay = 0.02f; p.level = 0.7f;
            break;
        case kTom:
            p.pitch = 0.5f; p.decay = 0.35f; p.fmAmount = 0.15f;
            p.fmRatio = 0.5f; p.foldAmount = 0.2f; p.lpgAmount = 0.85f;
            p.lpgDecay = 0.4f; p.pitchEnvAmount = 0.5f; p.pitchEnvDecay = 0.12f;
            p.level = 0.8f;
            break;
        case kPerc:
            p.pitch = 0.5f; p.decay = 0.2f; p.fmAmount = 0.5f;
            p.fmRatio = 0.62f; p.foldAmount = 0.6f; p.foldSymmetry = 0.6f;
            p.lpgAmount = 0.8f; p.lpgDecay = 0.2f; p.lpgResonance = 0.3f;
            p.pitchEnvAmount = 0.2f; p.pitchEnvDecay = 0.05f; p.level = 0.75f;
            p.waveShape = 0.4f;
            break;
    }
    return p;
}

void DrumVoice::trigger(float velocity) {
    fmOp_.reset();

    ampEnv_.setDecay(params_.decay);
    ampEnv_.setAmount(1.0f);
    ampEnv_.setType(EnvelopeGenerator::Type::Amplitude);
    ampEnv_.setCurve(1.5f);
    ampEnv_.trigger(velocity);

    pitchEnv_.setDecay(params_.pitchEnvDecay);
    pitchEnv_.setAmount(params_.pitchEnvAmount);
    pitchEnv_.setType(EnvelopeGenerator::Type::Pitch);
    pitchEnv_.setCurve(2.0f);
    pitchEnv_.trigger(velocity);

    lpg_.setAmount(params_.lpgAmount);
    lpg_.setDecay(params_.lpgDecay);
    lpg_.setResonance(params_.lpgResonance);
    lpg_.trigger(velocity);

    waveFolder_.setFoldAmount(params_.foldAmount);
    waveFolder_.setSymmetry(params_.foldSymmetry);
    waveFolder_.setDrive(params_.drive);
}

void DrumVoice::process(float& outL, float& outR) {
    if (!isActive()) {
        outL = 0.0f;
        outR = 0.0f;
        return;
    }

    float baseFreq = getBaseFrequency();
    float pitchMod = pitchEnv_.process();
    float currentFreq = baseFreq * (1.0f + pitchMod * 8.0f);

    fmOp_.setFrequency(currentFreq);
    fmOp_.setRatio(0.5f + params_.fmRatio * 4.0f);
    fmOp_.setAmount(params_.fmAmount);

    float osc = fmOp_.process();

    // Additional waveshaping for variety
    if (params_.waveShape > 0.01f) {
        float shaped = std::tanh(osc * (1.0f + params_.waveShape * 5.0f));
        osc = osc * (1.0f - params_.waveShape) + shaped * params_.waveShape;
    }

    float folded = waveFolder_.process(osc);

    float noiseSig = 0.0f;
    if (params_.noiseAmount > 0.01f) {
        if (voiceType_ == kClosedHat || voiceType_ == kOpenHat) {
            noiseSig = noise_.metallic(currentFreq);
        } else {
            noiseSig = noise_.filtered();
        }
    }

    float mixed = folded * (1.0f - params_.noiseAmount) + noiseSig * params_.noiseAmount;

    float output = lpg_.process(mixed);

    float amp = ampEnv_.process();
    output *= amp * params_.level;

    float panR = params_.pan;
    float panL = 1.0f - params_.pan;
    panL = std::sqrt(panL);
    panR = std::sqrt(panR);

    outL = output * panL;
    outR = output * panR;
}

bool DrumVoice::isActive() const {
    return ampEnv_.isActive();
}

void DrumVoice::reset() {
    fmOp_.reset();
    ampEnv_.reset();
    pitchEnv_.reset();
    lpg_.reset();
    noise_.reset();
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
