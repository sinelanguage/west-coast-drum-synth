#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "dsp/DrumVoice.h"
#include "sequencer/StepSequencer.h"
#include "wcparams.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

class WCDrumProcessor : public Steinberg::Vst::AudioEffect {
public:
    WCDrumProcessor();
    ~WCDrumProcessor() override = default;

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new WCDrumProcessor());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;

    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;

private:
    DrumVoice voices_[kNumDrumVoices];
    DrumVoiceParams voiceParams_[kNumDrumVoices];
    StepSequencer sequencer_;

    float masterVolume_ = 0.8f;
    float tempo_ = 120.0f;
    float swing_ = 0.0f;
    bool playing_ = false;

    void processParameterChanges(Steinberg::Vst::IParameterChanges* changes);
    void processEvents(Steinberg::Vst::IEventList* events);
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
