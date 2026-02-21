#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "wcparams.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

struct FactoryPreset {
    const char* name;
    float voiceParams[kNumDrumVoices][16];
    bool steps[kNumDrumVoices][kNumSteps];
};

class WCDrumController : public Steinberg::Vst::EditControllerEx1,
                          public VSTGUI::VST3EditorDelegate {
public:
    WCDrumController() = default;
    ~WCDrumController() override = default;

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IEditController*>(new WCDrumController());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;

    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;

    Steinberg::tresult PLUGIN_API setParamNormalized(
        Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
    Steinberg::tresult PLUGIN_API getParamStringByValue(
        Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized,
        Steinberg::Vst::String128 string) override;
    Steinberg::tresult PLUGIN_API getParamValueByString(
        Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string,
        Steinberg::Vst::ParamValue& valueNormalized) override;

private:
    void initParameters();
    void addVoiceParameters(int voiceIdx, const char* voiceName);
    void addSequencerParameters();
    void initFactoryPresets();
    void loadPreset(int presetIndex);
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
