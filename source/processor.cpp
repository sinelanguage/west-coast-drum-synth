#include "processor.h"
#include "wcids.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstring>

namespace SineLanguage {
namespace WestCoastDrumSynth {

WCDrumProcessor::WCDrumProcessor() {
    setControllerClass(kControllerUID);

    for (int i = 0; i < kNumDrumVoices; ++i) {
        voices_[i].setVoiceType(static_cast<DrumVoiceType>(i));
        voiceParams_[i] = DrumVoice::getDefaultParams(static_cast<DrumVoiceType>(i));
    }

    sequencer_.setTriggerCallback([this](int voice, float velocity) {
        if (voice >= 0 && voice < kNumDrumVoices) {
            voices_[voice].setParams(voiceParams_[voice]);
            voices_[voice].trigger(velocity);
        }
    });
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::initialize(Steinberg::FUnknown* context) {
    Steinberg::tresult result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;

    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
    addEventInput(STR16("Event In"), 1);

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::terminate() {
    return AudioEffect::terminate();
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::setActive(Steinberg::TBool state) {
    if (state) {
        for (int i = 0; i < kNumDrumVoices; ++i) {
            voices_[i].setSampleRate(static_cast<float>(processSetup.sampleRate));
        }
        sequencer_.setSampleRate(static_cast<float>(processSetup.sampleRate));
    } else {
        for (int i = 0; i < kNumDrumVoices; ++i) {
            voices_[i].reset();
        }
        sequencer_.reset();
    }
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) {
    return AudioEffect::setupProcessing(newSetup);
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::canProcessSampleSize(Steinberg::int32 symbolicSampleSize) {
    if (symbolicSampleSize == Steinberg::Vst::kSample32)
        return Steinberg::kResultTrue;
    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
    Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) {
    if (numIns == 0 && numOuts == 1 && outputs[0] == Steinberg::Vst::SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    return Steinberg::kResultFalse;
}

void WCDrumProcessor::processParameterChanges(Steinberg::Vst::IParameterChanges* changes) {
    if (!changes) return;

    Steinberg::int32 numParamsChanged = changes->getParameterCount();
    for (Steinberg::int32 i = 0; i < numParamsChanged; ++i) {
        auto* paramQueue = changes->getParameterData(i);
        if (!paramQueue) continue;

        Steinberg::Vst::ParamID id = paramQueue->getParameterId();
        Steinberg::int32 numPoints = paramQueue->getPointCount();
        Steinberg::Vst::ParamValue value;
        Steinberg::int32 sampleOffset;

        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) != Steinberg::kResultTrue)
            continue;

        float fval = static_cast<float>(value);

        if (id == kParamTempo) {
            tempo_ = 40.0f + fval * 260.0f;
            sequencer_.setTempo(tempo_);
        } else if (id == kParamSwing) {
            swing_ = fval;
            sequencer_.setSwing(swing_);
        } else if (id == kParamMasterVol) {
            masterVolume_ = fval;
        } else if (id == kParamPlaying) {
            bool newPlaying = fval > 0.5f;
            if (newPlaying != playing_) {
                playing_ = newPlaying;
                sequencer_.setPlaying(playing_);
            }
        } else if (id >= kVoiceParamBase && id < kSeqStepBase) {
            int voiceIdx = (id - kVoiceParamBase) / kVoiceParamStride;
            int paramOff = (id - kVoiceParamBase) % kVoiceParamStride;
            if (voiceIdx >= 0 && voiceIdx < kNumDrumVoices) {
                auto& vp = voiceParams_[voiceIdx];
                switch (paramOff) {
                    case kVoiceLevel: vp.level = fval; break;
                    case kVoicePitch: vp.pitch = fval; break;
                    case kVoiceDecay: vp.decay = fval; break;
                    case kVoiceFMAmount: vp.fmAmount = fval; break;
                    case kVoiceFMRatio: vp.fmRatio = fval; break;
                    case kVoiceFoldAmount: vp.foldAmount = fval; break;
                    case kVoiceFoldSymmetry: vp.foldSymmetry = fval; break;
                    case kVoiceLPGAmount: vp.lpgAmount = fval; break;
                    case kVoiceLPGDecay: vp.lpgDecay = fval; break;
                    case kVoiceLPGResonance: vp.lpgResonance = fval; break;
                    case kVoiceNoiseAmount: vp.noiseAmount = fval; break;
                    case kVoicePitchEnvAmount: vp.pitchEnvAmount = fval; break;
                    case kVoicePitchEnvDecay: vp.pitchEnvDecay = fval; break;
                    case kVoiceDrive: vp.drive = fval; break;
                    case kVoicePan: vp.pan = fval; break;
                    case kVoiceWaveShape: vp.waveShape = fval; break;
                }
            }
        } else if (id >= kSeqStepBase && id < kSeqVelocityBase) {
            int idx = id - kSeqStepBase;
            int voice = idx / kNumSteps;
            int step = idx % kNumSteps;
            sequencer_.setStep(voice, step, fval > 0.5f);
        } else if (id >= kSeqVelocityBase && id < kParamSelectedVoice) {
            int idx = id - kSeqVelocityBase;
            int voice = idx / kNumSteps;
            int step = idx % kNumSteps;
            sequencer_.setStepVelocity(voice, step, fval);
        }
    }
}

void WCDrumProcessor::processEvents(Steinberg::Vst::IEventList* events) {
    if (!events) return;

    Steinberg::int32 numEvents = events->getEventCount();
    for (Steinberg::int32 i = 0; i < numEvents; ++i) {
        Steinberg::Vst::Event event;
        if (events->getEvent(i, event) != Steinberg::kResultOk) continue;

        if (event.type == Steinberg::Vst::Event::kNoteOnEvent) {
            int note = event.noteOn.pitch;
            float vel = event.noteOn.velocity;
            // Map MIDI notes 36-41 (C2-F2) to voices
            int voiceIdx = note - 36;
            if (voiceIdx >= 0 && voiceIdx < kNumDrumVoices) {
                voices_[voiceIdx].setParams(voiceParams_[voiceIdx]);
                voices_[voiceIdx].trigger(vel);
            }
        }
    }
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::process(Steinberg::Vst::ProcessData& data) {
    processParameterChanges(data.inputParameterChanges);

    if (data.inputEvents)
        processEvents(data.inputEvents);

    // Handle host transport
    if (data.processContext) {
        if (data.processContext->state & Steinberg::Vst::ProcessContext::kTempoValid) {
            tempo_ = static_cast<float>(data.processContext->tempo);
            sequencer_.setTempo(tempo_);
        }
    }

    if (data.numOutputs == 0 || data.numSamples == 0)
        return Steinberg::kResultOk;

    float* outL = data.outputs[0].channelBuffers32[0];
    float* outR = data.outputs[0].channelBuffers32[1];

    if (!outL || !outR)
        return Steinberg::kResultOk;

    std::memset(outL, 0, sizeof(float) * data.numSamples);
    std::memset(outR, 0, sizeof(float) * data.numSamples);

    sequencer_.process(data.numSamples);

    for (int s = 0; s < data.numSamples; ++s) {
        float mixL = 0.0f, mixR = 0.0f;
        for (int v = 0; v < kNumDrumVoices; ++v) {
            float vL, vR;
            voices_[v].process(vL, vR);
            mixL += vL;
            mixR += vR;
        }
        outL[s] = mixL * masterVolume_;
        outR[s] = mixR * masterVolume_;
    }

    data.outputs[0].silenceFlags = 0;
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::setState(Steinberg::IBStream* state) {
    if (!state) return Steinberg::kResultFalse;

    Steinberg::IBStreamer streamer(state, kLittleEndian);

    float savedMasterVol;
    if (!streamer.readFloat(savedMasterVol)) return Steinberg::kResultFalse;
    masterVolume_ = savedMasterVol;

    float savedTempo;
    if (!streamer.readFloat(savedTempo)) return Steinberg::kResultFalse;
    tempo_ = savedTempo;

    float savedSwing;
    if (!streamer.readFloat(savedSwing)) return Steinberg::kResultFalse;
    swing_ = savedSwing;

    for (int v = 0; v < kNumDrumVoices; ++v) {
        auto& vp = voiceParams_[v];
        streamer.readFloat(vp.level);
        streamer.readFloat(vp.pitch);
        streamer.readFloat(vp.decay);
        streamer.readFloat(vp.fmAmount);
        streamer.readFloat(vp.fmRatio);
        streamer.readFloat(vp.foldAmount);
        streamer.readFloat(vp.foldSymmetry);
        streamer.readFloat(vp.lpgAmount);
        streamer.readFloat(vp.lpgDecay);
        streamer.readFloat(vp.lpgResonance);
        streamer.readFloat(vp.noiseAmount);
        streamer.readFloat(vp.pitchEnvAmount);
        streamer.readFloat(vp.pitchEnvDecay);
        streamer.readFloat(vp.drive);
        streamer.readFloat(vp.pan);
        streamer.readFloat(vp.waveShape);
    }

    for (int v = 0; v < kNumDrumVoices; ++v) {
        for (int s = 0; s < kNumSteps; ++s) {
            Steinberg::int32 active;
            streamer.readInt32(active);
            sequencer_.setStep(v, s, active != 0);

            float vel;
            streamer.readFloat(vel);
            sequencer_.setStepVelocity(v, s, vel);
        }
    }

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API WCDrumProcessor::getState(Steinberg::IBStream* state) {
    if (!state) return Steinberg::kResultFalse;

    Steinberg::IBStreamer streamer(state, kLittleEndian);

    streamer.writeFloat(masterVolume_);
    streamer.writeFloat(tempo_);
    streamer.writeFloat(swing_);

    for (int v = 0; v < kNumDrumVoices; ++v) {
        const auto& vp = voiceParams_[v];
        streamer.writeFloat(vp.level);
        streamer.writeFloat(vp.pitch);
        streamer.writeFloat(vp.decay);
        streamer.writeFloat(vp.fmAmount);
        streamer.writeFloat(vp.fmRatio);
        streamer.writeFloat(vp.foldAmount);
        streamer.writeFloat(vp.foldSymmetry);
        streamer.writeFloat(vp.lpgAmount);
        streamer.writeFloat(vp.lpgDecay);
        streamer.writeFloat(vp.lpgResonance);
        streamer.writeFloat(vp.noiseAmount);
        streamer.writeFloat(vp.pitchEnvAmount);
        streamer.writeFloat(vp.pitchEnvDecay);
        streamer.writeFloat(vp.drive);
        streamer.writeFloat(vp.pan);
        streamer.writeFloat(vp.waveShape);
    }

    for (int v = 0; v < kNumDrumVoices; ++v) {
        for (int s = 0; s < kNumSteps; ++s) {
            streamer.writeInt32(sequencer_.getStep(v, s) ? 1 : 0);
            streamer.writeFloat(sequencer_.getStepVelocity(v, s));
        }
    }

    return Steinberg::kResultOk;
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
