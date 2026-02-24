#pragma once

#include "pluginterfaces/vst/vsttypes.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

static const int kNumDrumVoices = 6;
static const int kNumSteps = 16;

enum DrumVoiceType {
    kKick = 0,
    kSnare,
    kClosedHat,
    kOpenHat,
    kTom,
    kPerc
};

enum ParamIDs : Steinberg::Vst::ParamID {
    // Global
    kParamTempo = 0,
    kParamSwing = 1,
    kParamMasterVol = 2,
    kParamPlaying = 3,

    // Per-voice parameters: base + voiceIndex * kVoiceParamStride
    kVoiceParamBase = 100,
    kVoiceParamStride = 30,

    // Offsets within each voice
    kVoiceLevel = 0,
    kVoicePitch = 1,
    kVoiceDecay = 2,
    kVoiceFMAmount = 3,
    kVoiceFMRatio = 4,
    kVoiceFoldAmount = 5,
    kVoiceFoldSymmetry = 6,
    kVoiceLPGAmount = 7,
    kVoiceLPGDecay = 8,
    kVoiceLPGResonance = 9,
    kVoiceNoiseAmount = 10,
    kVoicePitchEnvAmount = 11,
    kVoicePitchEnvDecay = 12,
    kVoiceDrive = 13,
    kVoicePan = 14,
    kVoiceWaveShape = 15,

    // Sequencer steps: base + voiceIndex * kNumSteps + stepIndex
    kSeqStepBase = 1000,
    kSeqVelocityBase = 2000,

    // Voice select for editing
    kParamSelectedVoice = 3000,
};

inline Steinberg::Vst::ParamID voiceParam(int voice, int offset) {
    return kVoiceParamBase + voice * kVoiceParamStride + offset;
}

inline Steinberg::Vst::ParamID seqStep(int voice, int step) {
    return kSeqStepBase + voice * kNumSteps + step;
}

inline Steinberg::Vst::ParamID seqVelocity(int voice, int step) {
    return kSeqVelocityBase + voice * kNumSteps + step;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
