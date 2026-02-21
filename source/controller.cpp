#include "controller.h"
#include "wcids.h"
#include "presets.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

static const char* kVoiceNames[kNumDrumVoices] = {
    "Kick", "Snare", "Closed Hat", "Open Hat", "Tom", "Perc"
};

Steinberg::tresult PLUGIN_API WCDrumController::initialize(Steinberg::FUnknown* context) {
    Steinberg::tresult result = EditControllerEx1::initialize(context);
    if (result != Steinberg::kResultOk)
        return result;

    initParameters();
    initFactoryPresets();

    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API WCDrumController::terminate() {
    return EditControllerEx1::terminate();
}

void WCDrumController::initParameters() {
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    auto* tempoParam = new RangeParameter(
        STR16("Tempo"), kParamTempo, STR16("BPM"),
        0.0, 1.0, (120.0 - 40.0) / 260.0);
    parameters.addParameter(tempoParam);

    auto* swingParam = new RangeParameter(
        STR16("Swing"), kParamSwing, STR16("%"),
        0.0, 1.0, 0.0);
    parameters.addParameter(swingParam);

    auto* masterParam = new RangeParameter(
        STR16("Master Volume"), kParamMasterVol, STR16(""),
        0.0, 1.0, 0.8);
    parameters.addParameter(masterParam);

    auto* playParam = new RangeParameter(
        STR16("Play"), kParamPlaying, STR16(""),
        0.0, 1.0, 0.0);
    parameters.addParameter(playParam);

    auto* selVoice = new RangeParameter(
        STR16("Selected Voice"), kParamSelectedVoice, STR16(""),
        0.0, 1.0, 0.0);
    parameters.addParameter(selVoice);

    for (int v = 0; v < kNumDrumVoices; ++v) {
        addVoiceParameters(v, kVoiceNames[v]);
    }

    addSequencerParameters();
}

void WCDrumController::addVoiceParameters(int voiceIdx, const char* voiceName) {
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    auto makeTitle = [&](const char* paramName, String128 outTitle) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s %s", voiceName, paramName);
        Steinberg::UString(outTitle, 128).fromAscii(buf);
    };

    struct ParamDef {
        int offset;
        const char* name;
        float defaultVal;
    };

    ParamDef defs[] = {
        { kVoiceLevel,          "Level",        0.8f },
        { kVoicePitch,          "Pitch",        0.5f },
        { kVoiceDecay,          "Decay",        0.3f },
        { kVoiceFMAmount,       "FM Amount",    0.0f },
        { kVoiceFMRatio,        "FM Ratio",     0.5f },
        { kVoiceFoldAmount,     "Fold",         0.0f },
        { kVoiceFoldSymmetry,   "Symmetry",     0.5f },
        { kVoiceLPGAmount,      "LPG Amount",   1.0f },
        { kVoiceLPGDecay,       "LPG Decay",    0.3f },
        { kVoiceLPGResonance,   "LPG Reso",     0.0f },
        { kVoiceNoiseAmount,    "Noise",        0.0f },
        { kVoicePitchEnvAmount, "PEnv Amt",     0.3f },
        { kVoicePitchEnvDecay,  "PEnv Decay",   0.2f },
        { kVoiceDrive,          "Drive",        0.0f },
        { kVoicePan,            "Pan",          0.5f },
        { kVoiceWaveShape,      "WaveShape",    0.0f },
    };

    for (const auto& def : defs) {
        String128 title;
        makeTitle(def.name, title);
        ParamID pid = voiceParam(voiceIdx, def.offset);
        auto* p = new RangeParameter(title, pid, STR16(""), 0.0, 1.0, def.defaultVal);
        parameters.addParameter(p);
    }
}

void WCDrumController::addSequencerParameters() {
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    for (int v = 0; v < kNumDrumVoices; ++v) {
        for (int s = 0; s < kNumSteps; ++s) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s Step %d", kVoiceNames[v], s + 1);
            String128 title;
            Steinberg::UString(title, 128).fromAscii(buf);

            ParamID stepId = seqStep(v, s);
            auto* stepParam = new RangeParameter(title, stepId, STR16(""), 0.0, 1.0, 0.0);
            parameters.addParameter(stepParam);

            snprintf(buf, sizeof(buf), "%s Vel %d", kVoiceNames[v], s + 1);
            Steinberg::UString(title, 128).fromAscii(buf);
            ParamID velId = seqVelocity(v, s);
            auto* velParam = new RangeParameter(title, velId, STR16(""), 0.0, 1.0, 0.8);
            parameters.addParameter(velParam);
        }
    }
}

void WCDrumController::initFactoryPresets() {
    using namespace Steinberg;
    using namespace Steinberg::Vst;

    auto* unit = new Unit(STR16("Root"), kRootUnitId, kNoParentUnitId);
    addUnit(unit);

    auto* presetParam = new StringListParameter(
        STR16("Preset"), 99999, nullptr,
        Steinberg::Vst::ParameterInfo::kIsProgramChange | Steinberg::Vst::ParameterInfo::kIsList);

    for (int i = 0; i < kNumFactoryPresets; ++i) {
        String128 presetName;
        Steinberg::UString(presetName, 128).fromAscii(kFactoryPresets[i].name);
        presetParam->appendString(presetName);
    }

    parameters.addParameter(presetParam);
}

void WCDrumController::loadPreset(int presetIndex) {
    if (presetIndex < 0 || presetIndex >= kNumFactoryPresets) return;

    const auto& preset = kFactoryPresets[presetIndex];

    for (int v = 0; v < kNumDrumVoices; ++v) {
        const auto& vd = preset.voices[v];
        const float* vals = &vd.level;

        for (int p = 0; p < 16; ++p) {
            Steinberg::Vst::ParamID pid = voiceParam(v, p);
            setParamNormalized(pid, vals[p]);

            if (componentHandler)
                componentHandler->performEdit(pid, vals[p]);
        }
    }

    for (int v = 0; v < kNumDrumVoices; ++v) {
        for (int s = 0; s < kNumSteps; ++s) {
            Steinberg::Vst::ParamID stepId = seqStep(v, s);
            double stepVal = preset.pattern.steps[v][s] ? 1.0 : 0.0;
            setParamNormalized(stepId, stepVal);

            if (componentHandler)
                componentHandler->performEdit(stepId, stepVal);
        }
    }
}

Steinberg::tresult PLUGIN_API WCDrumController::setComponentState(Steinberg::IBStream* state) {
    if (!state) return Steinberg::kResultFalse;

    Steinberg::IBStreamer streamer(state, kLittleEndian);

    float masterVol;
    if (streamer.readFloat(masterVol))
        setParamNormalized(kParamMasterVol, masterVol);

    float tempo;
    if (streamer.readFloat(tempo))
        setParamNormalized(kParamTempo, (tempo - 40.0f) / 260.0f);

    float swing;
    if (streamer.readFloat(swing))
        setParamNormalized(kParamSwing, swing);

    for (int v = 0; v < kNumDrumVoices; ++v) {
        float vals[16];
        for (int p = 0; p < 16; ++p) {
            if (streamer.readFloat(vals[p]))
                setParamNormalized(voiceParam(v, p), vals[p]);
        }
    }

    for (int v = 0; v < kNumDrumVoices; ++v) {
        for (int s = 0; s < kNumSteps; ++s) {
            Steinberg::int32 active;
            if (streamer.readInt32(active))
                setParamNormalized(seqStep(v, s), active ? 1.0 : 0.0);

            float vel;
            if (streamer.readFloat(vel))
                setParamNormalized(seqVelocity(v, s), vel);
        }
    }

    return Steinberg::kResultOk;
}

Steinberg::IPlugView* PLUGIN_API WCDrumController::createView(Steinberg::FIDString name) {
    if (Steinberg::FIDStringsEqual(name, Steinberg::Vst::ViewType::kEditor)) {
        auto* view = new VSTGUI::VST3Editor(this, "view", "wcdrumsynth.uidesc");
        return view;
    }
    return nullptr;
}

Steinberg::tresult PLUGIN_API WCDrumController::setParamNormalized(
    Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) {

    Steinberg::tresult result = EditControllerEx1::setParamNormalized(tag, value);

    // When preset parameter changes, load the preset
    if (tag == 99999) {
        int presetIndex = static_cast<int>(value * (kNumFactoryPresets - 1) + 0.5);
        loadPreset(presetIndex);
    }

    return result;
}

Steinberg::tresult PLUGIN_API WCDrumController::getParamStringByValue(
    Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized,
    Steinberg::Vst::String128 string) {

    if (tag == kParamTempo) {
        float bpm = 40.0f + static_cast<float>(valueNormalized) * 260.0f;
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", bpm);
        Steinberg::UString(string, 128).fromAscii(buf);
        return Steinberg::kResultTrue;
    }

    return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

Steinberg::tresult PLUGIN_API WCDrumController::getParamValueByString(
    Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string,
    Steinberg::Vst::ParamValue& valueNormalized) {
    return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
