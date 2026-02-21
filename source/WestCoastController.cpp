#include "WestCoastController.h"

#include "ParameterIds.h"
#include "presets/FactoryPresets.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <array>
#include <utility>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr uint32 kStateVersion = 1;

UString128 toString128 (const char* ascii)
{
  return UString128 (ascii ? ascii : "");
}

Vst::RangeParameter* makeRangeParam (const char* title, Vst::ParamID id, const char* unit, double minPlain,
                                     double maxPlain, double defaultPlain)
{
  auto title16 = toString128 (title);
  auto unit16 = toString128 (unit);
  return new Vst::RangeParameter (title16, id, unit16, minPlain, maxPlain, defaultPlain);
}

} // namespace

FUnknown* WestCoastController::createInstance (void*)
{
  return static_cast<Vst::IEditController*> (new WestCoastController ());
}

tresult PLUGIN_API WestCoastController::initialize (FUnknown* context)
{
  const tresult result = EditControllerEx1::initialize (context);
  if (result != kResultOk)
    return result;

  parameters.addParameter (makeRangeParam ("Master", kParamMaster, "%", 0.0, 100.0, 80.0));
  parameters.addParameter (makeRangeParam ("Internal Tempo", kParamInternalTempo, "BPM", 60.0, 180.0, 120.0));
  parameters.addParameter (makeRangeParam ("Swing", kParamSwing, "%", 0.0, 100.0, 12.0));
  parameters.addParameter (STR16 ("Run"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate, kParamRun);
  parameters.addParameter (STR16 ("Follow Host"), nullptr, 1, 1.0, Vst::ParameterInfo::kCanAutomate,
                           kParamFollowTransport);

  auto* presetParam = new Vst::StringListParameter (STR16 ("Preset"), kParamPresetSelect);
  for (const auto& preset : getFactoryPresets ())
  {
    auto presetName = toString128 (preset.name.data ());
    presetParam->appendString (presetName);
  }
  parameters.addParameter (presetParam);

  const std::array<std::array<const char*, kLaneParamCount>, kLaneCount> laneTitles {{
    {{"Kick Tune", "Kick Decay", "Kick Fold", "Kick FM", "Kick Noise", "Kick Drive", "Kick Level", "Kick Pan"}},
    {{"Snare Tune", "Snare Decay", "Snare Fold", "Snare FM", "Snare Noise", "Snare Drive", "Snare Level",
      "Snare Pan"}},
    {{"Hat Tune", "Hat Decay", "Hat Fold", "Hat FM", "Hat Noise", "Hat Drive", "Hat Level", "Hat Pan"}},
    {{"Perc Tune", "Perc Decay", "Perc Fold", "Perc FM", "Perc Noise", "Perc Drive", "Perc Level", "Perc Pan"}},
  }};

  const std::array<const char*, kLaneParamCount> laneUnits {"st", "s", "%", "%", "%", "%", "%", "%"};
  const std::array<std::pair<double, double>, kLaneParamCount> laneRanges {
    std::make_pair (-24.0, 24.0), std::make_pair (0.02, 1.82), std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0), std::make_pair (0.0, 100.0), std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0), std::make_pair (-100.0, 100.0),
  };
  const std::array<double, kLaneParamCount> laneDefaults {0.0, 0.50, 40.0, 35.0, 20.0, 18.0, 75.0, 0.0};

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 parameterOffset = 0; parameterOffset < kLaneParamCount; ++parameterOffset)
    {
      const auto offset = static_cast<LaneParamOffset> (parameterOffset);
      parameters.addParameter (
        makeRangeParam (laneTitles[lane][parameterOffset], laneParamID (lane, offset), laneUnits[parameterOffset],
                        laneRanges[parameterOffset].first, laneRanges[parameterOffset].second,
                        laneDefaults[parameterOffset]));
    }
  }

  return kResultOk;
}

tresult PLUGIN_API WestCoastController::terminate ()
{
  return EditControllerEx1::terminate ();
}

tresult PLUGIN_API WestCoastController::setComponentState (IBStream* state)
{
  if (!state)
    return kResultFalse;

  IBStreamer streamer (state, kLittleEndian);
  uint32 version = 0;
  if (!streamer.readInt32u (version))
    return kResultFalse;
  if (version != kStateVersion)
    return kResultFalse;

  int32 ignoredPreset = 0;
  if (!streamer.readInt32 (ignoredPreset))
    return kResultFalse;

  for (const auto id : allParameterIds ())
  {
    double value = 0.0;
    if (!streamer.readDouble (value))
      return kResultFalse;
    setParamNormalized (id, value);
  }

  return kResultOk;
}

IPlugView* PLUGIN_API WestCoastController::createView (FIDString name)
{
  if (FIDStringsEqual (name, Vst::ViewType::kEditor))
    return new VSTGUI::VST3Editor (this, "Editor", "WestCoastEditor.uidesc");
  return nullptr;
}

} // namespace Steinberg::WestCoastDrumSynth
