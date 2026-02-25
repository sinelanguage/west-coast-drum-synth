#include "WestCoastController.h"

#include "ParameterIds.h"
#include "presets/FactoryPresets.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <array>
#include <utility>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr uint32 kStateVersion = 3;
constexpr uint32 kPreviousStateVersion = 2;
constexpr uint32 kLegacyStateVersion = 1;
constexpr int32 kLegacyLaneCount = 4;

constexpr std::array<std::array<double, kLaneExtraParamCount>, kLaneCount> kLaneExtraDefaults {{
  {{0.84, 0.30, 0.76, 0.36, 0.26, 0.24}}, // Kick
  {{0.46, 0.48, 0.62, 0.72, 0.58, 0.84}}, // Snare
  {{0.20, 0.22, 0.38, 0.90, 0.20, 0.72}}, // Hat
  {{0.48, 0.34, 0.50, 0.58, 0.40, 0.52}}, // Perc A
  {{0.56, 0.30, 0.56, 0.66, 0.38, 0.60}}, // Perc B
}};

constexpr std::array<std::array<double, kLaneMacroParamCount>, kLaneCount> kLaneMacroDefaults {{
  {{0.28, 0.44, 0.34, 0.56}}, // Kick
  {{0.38, 0.56, 0.50, 0.72}}, // Snare
  {{0.20, 0.36, 0.66, 0.86}}, // Hat
  {{0.34, 0.46, 0.42, 0.58}}, // Perc A
  {{0.30, 0.48, 0.46, 0.62}}, // Perc B
}};

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
    {{"Perc A Tune", "Perc A Decay", "Perc A Fold", "Perc A FM", "Perc A Noise", "Perc A Drive", "Perc A Level",
      "Perc A Pan"}},
    {{"Perc B Tune", "Perc B Decay", "Perc B Fold", "Perc B FM", "Perc B Noise", "Perc B Drive", "Perc B Level",
      "Perc B Pan"}},
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

  const std::array<std::array<const char*, kLaneExtraParamCount>, kLaneCount> laneExtraTitles {{
    {{"Kick Pitch Env", "Kick Pitch Env Decay", "Kick Attack Click", "Kick Noise Tone", "Kick Noise Decay",
      "Kick Snap"}},
    {{"Snare Pitch Env", "Snare Pitch Env Decay", "Snare Attack Click", "Snare Noise Tone", "Snare Noise Decay",
      "Snare Snap"}},
    {{"Hat Pitch Env", "Hat Pitch Env Decay", "Hat Attack Click", "Hat Noise Tone", "Hat Noise Decay", "Hat Snap"}},
    {{"Perc A Pitch Env", "Perc A Pitch Env Decay", "Perc A Attack Click", "Perc A Noise Tone",
      "Perc A Noise Decay", "Perc A Snap"}},
    {{"Perc B Pitch Env", "Perc B Pitch Env Decay", "Perc B Attack Click", "Perc B Noise Tone",
      "Perc B Noise Decay", "Perc B Snap"}},
  }};
  const std::array<const char*, kLaneExtraParamCount> laneExtraUnits {"st", "ms", "%", "%", "ms", "%"};
  const std::array<std::pair<double, double>, kLaneExtraParamCount> laneExtraRanges {
    std::make_pair (0.0, 72.0), std::make_pair (6.0, 560.0), std::make_pair (0.0, 100.0),
    std::make_pair (-100.0, 100.0), std::make_pair (8.0, 1400.0), std::make_pair (0.0, 100.0),
  };

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 parameterOffset = 0; parameterOffset < kLaneExtraParamCount; ++parameterOffset)
    {
      const auto offset = static_cast<LaneExtraParamOffset> (parameterOffset);
      const double minimum = laneExtraRanges[parameterOffset].first;
      const double maximum = laneExtraRanges[parameterOffset].second;
      const double normalizedDefault = kLaneExtraDefaults[lane][parameterOffset];
      const double defaultPlain = minimum + (normalizedDefault * (maximum - minimum));
      parameters.addParameter (makeRangeParam (laneExtraTitles[lane][parameterOffset], laneExtraParamID (lane, offset),
                                               laneExtraUnits[parameterOffset], minimum, maximum, defaultPlain));
    }
  }

  const std::array<std::array<const char*, kLaneMacroParamCount>, kLaneCount> laneMacroTitles {{
    {{"Kick Transient Decay", "Kick Transient Mix", "Kick Noise Resonance", "Kick Noise Env"}},
    {{"Snare Transient Decay", "Snare Transient Mix", "Snare Noise Resonance", "Snare Noise Env"}},
    {{"Hat Transient Decay", "Hat Transient Mix", "Hat Noise Resonance", "Hat Noise Env"}},
    {{"Perc A Transient Decay", "Perc A Transient Mix", "Perc A Noise Resonance", "Perc A Noise Env"}},
    {{"Perc B Transient Decay", "Perc B Transient Mix", "Perc B Noise Resonance", "Perc B Noise Env"}},
  }};
  const std::array<const char*, kLaneMacroParamCount> laneMacroUnits {"ms", "%", "%", "%"};
  const std::array<std::pair<double, double>, kLaneMacroParamCount> laneMacroRanges {
    std::make_pair (2.0, 420.0),
    std::make_pair (0.0, 140.0),
    std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0),
  };

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 parameterOffset = 0; parameterOffset < kLaneMacroParamCount; ++parameterOffset)
    {
      const auto offset = static_cast<LaneMacroParamOffset> (parameterOffset);
      const double minimum = laneMacroRanges[parameterOffset].first;
      const double maximum = laneMacroRanges[parameterOffset].second;
      const double normalizedDefault = kLaneMacroDefaults[lane][parameterOffset];
      const double defaultPlain = minimum + (normalizedDefault * (maximum - minimum));
      parameters.addParameter (makeRangeParam (laneMacroTitles[lane][parameterOffset], laneMacroParamID (lane, offset),
                                               laneMacroUnits[parameterOffset], minimum, maximum, defaultPlain));
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

  int32 ignoredPreset = 0;
  if (!streamer.readInt32 (ignoredPreset))
    return kResultFalse;

  const auto applyMacroDefaults = [this] () {
    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneMacroParamCount; ++parameterOffset)
      {
        setParamNormalized (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (parameterOffset)),
                            kLaneMacroDefaults[lane][parameterOffset]);
      }
    }
  };

  if (version == kLegacyStateVersion)
  {
    for (int32 param = 0; param < kParamGlobalCount; ++param)
    {
      double value = 0.0;
      if (!streamer.readDouble (value))
        return kResultFalse;
      setParamNormalized (static_cast<Vst::ParamID> (param), value);
    }

    for (int32 lane = 0; lane < kLegacyLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneParamCount; ++parameterOffset)
      {
        double value = 0.0;
        if (!streamer.readDouble (value))
          return kResultFalse;
        setParamNormalized (laneParamID (lane, static_cast<LaneParamOffset> (parameterOffset)), value);
      }
    }

    for (int32 parameterOffset = 0; parameterOffset < kLaneParamCount; ++parameterOffset)
    {
      const auto offset = static_cast<LaneParamOffset> (parameterOffset);
      double value = getParamNormalized (laneParamID (3, offset));
      if (offset == kLanePan)
        value = std::clamp (value + 0.08, 0.0, 1.0);
      setParamNormalized (laneParamID (4, offset), value);
    }

    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneExtraParamCount; ++parameterOffset)
        setParamNormalized (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (parameterOffset)),
                            kLaneExtraDefaults[lane][parameterOffset]);
      for (int32 parameterOffset = 0; parameterOffset < kLaneMacroParamCount; ++parameterOffset)
        setParamNormalized (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (parameterOffset)),
                            kLaneMacroDefaults[lane][parameterOffset]);
    }
    return kResultOk;
  }

  if (version == kPreviousStateVersion)
  {
    constexpr int32 v2ParamCount =
      kParamGlobalCount + (kLaneCount * kLaneParamCount) + (kLaneCount * kLaneExtraParamCount);
    constexpr auto v2Ids = [] ()
    {
      std::array<Vst::ParamID, v2ParamCount> ids {};
      int32 index = 0;
      for (int32 i = 0; i < kParamGlobalCount; ++i)
        ids[index++] = static_cast<Vst::ParamID> (i);
      for (int32 lane = 0; lane < kLaneCount; ++lane)
        for (int32 p = 0; p < kLaneParamCount; ++p)
          ids[index++] = laneParamID (lane, static_cast<LaneParamOffset> (p));
      for (int32 lane = 0; lane < kLaneCount; ++lane)
        for (int32 p = 0; p < kLaneExtraParamCount; ++p)
          ids[index++] = laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p));
      return ids;
    }();

    for (const auto id : v2Ids)
    {
      double value = 0.0;
      if (!streamer.readDouble (value))
        return kResultFalse;
      setParamNormalized (id, value);
    }

    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneMacroParamCount; ++parameterOffset)
        setParamNormalized (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (parameterOffset)),
                            kLaneMacroDefaults[lane][parameterOffset]);
    }
    applyMacroDefaults ();
    return kResultOk;
  }

  if (version != kStateVersion)
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
