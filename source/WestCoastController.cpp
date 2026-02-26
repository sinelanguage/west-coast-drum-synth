#include "WestCoastController.h"

#include "ParameterIds.h"
#include "presets/FactoryPresets.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/lib/controls/cslider.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <array>
#include <utility>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr uint32 kStateVersion = 5;
constexpr uint32 kV4StateVersion = 4;
constexpr uint32 kV3StateVersion = 3;
constexpr int32 kV4LaneCount = 5;
constexpr uint32 kPreviousStateVersion = 2;
constexpr uint32 kLegacyStateVersion = 1;
constexpr int32 kLegacyLaneCount = 4;
constexpr int32 kPreviousGlobalParamCount = 6;

constexpr std::array<std::array<double, kLaneExtraParamCount>, kLaneCount> kLaneExtraDefaults {{
  {{0.84, 0.30, 0.76, 0.36, 0.26, 0.24}},
  {{0.46, 0.48, 0.62, 0.72, 0.58, 0.84}},
  {{0.20, 0.22, 0.38, 0.90, 0.20, 0.72}},
  {{0.42, 0.38, 0.48, 0.52, 0.42, 0.48}},
  {{0.44, 0.36, 0.50, 0.54, 0.44, 0.50}},
  {{0.52, 0.34, 0.54, 0.60, 0.40, 0.56}},
  {{0.54, 0.32, 0.56, 0.62, 0.38, 0.58}},
  {{0.48, 0.40, 0.58, 0.68, 0.52, 0.72}},
  {{0.38, 0.45, 0.62, 0.72, 0.58, 0.65}},
}};

constexpr std::array<std::array<double, kLaneMacroParamCount>, kLaneCount> kLaneMacroDefaults {{
  {{0.28, 0.44, 0.34, 0.56}},
  {{0.38, 0.56, 0.50, 0.72}},
  {{0.20, 0.36, 0.66, 0.86}},
  {{0.32, 0.48, 0.40, 0.54}},
  {{0.34, 0.46, 0.42, 0.56}},
  {{0.36, 0.50, 0.44, 0.58}},
  {{0.38, 0.48, 0.46, 0.60}},
  {{0.30, 0.52, 0.50, 0.62}},
  {{0.28, 0.54, 0.55, 0.68}},
}};

constexpr std::array<std::array<double, kLaneFilterParamCount>, kLaneCount> kLaneFilterDefaults {{
  {{0.65, 0.08, 0.40, 0.70, 0.05, 0.35}},
  {{0.68, 0.12, 0.30, 0.72, 0.08, 0.45}},
  {{0.82, 0.06, 0.20, 0.85, 0.04, 0.30}},
  {{0.72, 0.12, 0.38, 0.76, 0.08, 0.42}},
  {{0.74, 0.11, 0.40, 0.78, 0.07, 0.44}},
  {{0.70, 0.14, 0.36, 0.74, 0.09, 0.40}},
  {{0.72, 0.13, 0.38, 0.76, 0.08, 0.42}},
  {{0.68, 0.16, 0.42, 0.72, 0.10, 0.48}},
  {{0.66, 0.18, 0.45, 0.70, 0.12, 0.52}},
}};

constexpr VSTGUI::CCoord kCompactScaleX = 0.40;
constexpr VSTGUI::CCoord kModuleBaseWidth = 296.0;

inline VSTGUI::CRect scaleRectX (const VSTGUI::CRect& rect, VSTGUI::CCoord scaleX)
{
  VSTGUI::CRect scaled = rect;
  const VSTGUI::CCoord width = rect.getWidth () * scaleX;
  const VSTGUI::CCoord left = rect.left * scaleX;
  scaled.left = left;
  scaled.right = left + width;
  return scaled;
}

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
  parameters.addParameter (
    makeRangeParam ("Internal Tempo (Seq)", kParamInternalTempo, "BPM", 60.0, 180.0, 120.0));
  parameters.addParameter (makeRangeParam ("Swing", kParamSwing, "%", 0.0, 100.0, 12.0));
  parameters.addParameter (
    makeRangeParam ("Osc Body Cutoff", kParamOscFilterCutoff, "Hz", 80.0, 16000.0, 3200.0));
  parameters.addParameter (
    makeRangeParam ("Osc Body Resonance", kParamOscFilterResonance, "%", 0.0, 100.0, 34.0));
  parameters.addParameter (
    makeRangeParam ("Osc Body Env", kParamOscFilterEnv, "%", 0.0, 200.0, 92.0));
  parameters.addParameter (STR16 ("Run"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate, kParamRun);
  parameters.addParameter (STR16 ("Follow Host"), nullptr, 1, 0.0, Vst::ParameterInfo::kCanAutomate,
                           kParamFollowTransport);

  auto* presetParam = new Vst::StringListParameter (STR16 ("Preset"), kParamPresetSelect);
  for (const auto& preset : getFactoryPresets ())
  {
    auto presetName = toString128 (preset.name.data ());
    presetParam->appendString (presetName);
  }
  parameters.addParameter (presetParam);

  const std::array<std::array<const char*, kLaneParamCount>, kLaneCount> laneTitles {{
    {{"Kick Tune", "Kick Decay", "Kick Fold", "Kick FM", "Kick Noise Level", "Kick Drive", "Kick Output", "Kick Pan"}},
    {{"Snare Tune", "Snare Decay", "Snare Fold", "Snare FM", "Snare Noise Level", "Snare Drive", "Snare Output",
      "Snare Pan"}},
    {{"Hat Tune", "Hat Decay", "Hat Fold", "Hat FM", "Hat Noise Level", "Hat Drive", "Hat Output", "Hat Pan"}},
    {{"Perc A1 Tune", "Perc A1 Decay", "Perc A1 Fold", "Perc A1 FM", "Perc A1 Noise Level", "Perc A1 Drive",
      "Perc A1 Output", "Perc A1 Pan"}},
    {{"Perc A2 Tune", "Perc A2 Decay", "Perc A2 Fold", "Perc A2 FM", "Perc A2 Noise Level", "Perc A2 Drive",
      "Perc A2 Output", "Perc A2 Pan"}},
    {{"Perc B1 Tune", "Perc B1 Decay", "Perc B1 Fold", "Perc B1 FM", "Perc B1 Noise Level", "Perc B1 Drive",
      "Perc B1 Output", "Perc B1 Pan"}},
    {{"Perc B2 Tune", "Perc B2 Decay", "Perc B2 Fold", "Perc B2 FM", "Perc B2 Noise Level", "Perc B2 Drive",
      "Perc B2 Output", "Perc B2 Pan"}},
    {{"RimShot Tune", "RimShot Decay", "RimShot Fold", "RimShot FM", "RimShot Noise Level", "RimShot Drive",
      "RimShot Output", "RimShot Pan"}},
    {{"Clap Tune", "Clap Decay", "Clap Fold", "Clap FM", "Clap Noise Level", "Clap Drive",
      "Clap Output", "Clap Pan"}},
  }};

  const std::array<const char*, kLaneParamCount> laneUnits {"st", "s", "%", "%", "%", "%", "%", "%"};
  const std::array<std::pair<double, double>, kLaneParamCount> laneRanges {
    std::make_pair (-36.0, 36.0), std::make_pair (0.02, 1.82), std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0), std::make_pair (0.0, 100.0), std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0), std::make_pair (-100.0, 100.0),
  };
  const std::array<double, kLaneParamCount> laneDefaults {0.0, 0.50, 40.0, 35.0, 28.0, 18.0, 72.0, 0.0};

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
    {{"Kick Pitch Env", "Kick Pitch Env Decay", "Kick Transient Amt", "Kick Noise Tone", "Kick Noise Decay",
      "Kick Snap"}},
    {{"Snare Pitch Env", "Snare Pitch Env Decay", "Snare Transient Amt", "Snare Noise Tone", "Snare Noise Decay",
      "Snare Snap"}},
    {{"Hat Pitch Env", "Hat Pitch Env Decay", "Hat Transient Amt", "Hat Noise Tone", "Hat Noise Decay", "Hat Snap"}},
    {{"Perc A1 Pitch Env", "Perc A1 Pitch Env Decay", "Perc A1 Transient Amt", "Perc A1 Noise Tone",
      "Perc A1 Noise Decay", "Perc A1 Snap"}},
    {{"Perc A2 Pitch Env", "Perc A2 Pitch Env Decay", "Perc A2 Transient Amt", "Perc A2 Noise Tone",
      "Perc A2 Noise Decay", "Perc A2 Snap"}},
    {{"Perc B1 Pitch Env", "Perc B1 Pitch Env Decay", "Perc B1 Transient Amt", "Perc B1 Noise Tone",
      "Perc B1 Noise Decay", "Perc B1 Snap"}},
    {{"Perc B2 Pitch Env", "Perc B2 Pitch Env Decay", "Perc B2 Transient Amt", "Perc B2 Noise Tone",
      "Perc B2 Noise Decay", "Perc B2 Snap"}},
    {{"RimShot Pitch Env", "RimShot Pitch Env Decay", "RimShot Transient Amt", "RimShot Noise Tone",
      "RimShot Noise Decay", "RimShot Snap"}},
    {{"Clap Pitch Env", "Clap Pitch Env Decay", "Clap Transient Amt", "Clap Noise Tone",
      "Clap Noise Decay", "Clap Snap"}},
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
    {{"Kick Transient Decay", "Kick Transient Level", "Kick Noise Resonance", "Kick Noise Env"}},
    {{"Snare Transient Decay", "Snare Transient Level", "Snare Noise Resonance", "Snare Noise Env"}},
    {{"Hat Transient Decay", "Hat Transient Level", "Hat Noise Resonance", "Hat Noise Env"}},
    {{"Perc A1 Transient Decay", "Perc A1 Transient Level", "Perc A1 Noise Resonance", "Perc A1 Noise Env"}},
    {{"Perc A2 Transient Decay", "Perc A2 Transient Level", "Perc A2 Noise Resonance", "Perc A2 Noise Env"}},
    {{"Perc B1 Transient Decay", "Perc B1 Transient Level", "Perc B1 Noise Resonance", "Perc B1 Noise Env"}},
    {{"Perc B2 Transient Decay", "Perc B2 Transient Level", "Perc B2 Noise Resonance", "Perc B2 Noise Env"}},
    {{"RimShot Transient Decay", "RimShot Transient Level", "RimShot Noise Resonance", "RimShot Noise Env"}},
    {{"Clap Transient Decay", "Clap Transient Level", "Clap Noise Resonance", "Clap Noise Env"}},
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

  const std::array<std::array<const char*, kLaneFilterParamCount>, kLaneCount> laneFilterTitles {{
    {{"Kick Osc Cutoff", "Kick Osc Reso", "Kick Osc Flt Env", "Kick Trans Cutoff", "Kick Trans Reso", "Kick Trans Flt Env"}},
    {{"Snare Osc Cutoff", "Snare Osc Reso", "Snare Osc Flt Env", "Snare Trans Cutoff", "Snare Trans Reso", "Snare Trans Flt Env"}},
    {{"Hat Osc Cutoff", "Hat Osc Reso", "Hat Osc Flt Env", "Hat Trans Cutoff", "Hat Trans Reso", "Hat Trans Flt Env"}},
    {{"Perc A1 Osc Cutoff", "Perc A1 Osc Reso", "Perc A1 Osc Flt Env", "Perc A1 Trans Cutoff", "Perc A1 Trans Reso", "Perc A1 Trans Flt Env"}},
    {{"Perc A2 Osc Cutoff", "Perc A2 Osc Reso", "Perc A2 Osc Flt Env", "Perc A2 Trans Cutoff", "Perc A2 Trans Reso", "Perc A2 Trans Flt Env"}},
    {{"Perc B1 Osc Cutoff", "Perc B1 Osc Reso", "Perc B1 Osc Flt Env", "Perc B1 Trans Cutoff", "Perc B1 Trans Reso", "Perc B1 Trans Flt Env"}},
    {{"Perc B2 Osc Cutoff", "Perc B2 Osc Reso", "Perc B2 Osc Flt Env", "Perc B2 Trans Cutoff", "Perc B2 Trans Reso", "Perc B2 Trans Flt Env"}},
    {{"RimShot Osc Cutoff", "RimShot Osc Reso", "RimShot Osc Flt Env", "RimShot Trans Cutoff", "RimShot Trans Reso", "RimShot Trans Flt Env"}},
    {{"Clap Osc Cutoff", "Clap Osc Reso", "Clap Osc Flt Env", "Clap Trans Cutoff", "Clap Trans Reso", "Clap Trans Flt Env"}},
  }};
  const std::array<const char*, kLaneFilterParamCount> laneFilterUnits {"%", "%", "%", "%", "%", "%"};
  const std::array<std::pair<double, double>, kLaneFilterParamCount> laneFilterRanges {
    std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0),
    std::make_pair (0.0, 100.0),
  };

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    for (int32 parameterOffset = 0; parameterOffset < kLaneFilterParamCount; ++parameterOffset)
    {
      const auto offset = static_cast<LaneFilterParamOffset> (parameterOffset);
      const double minimum = laneFilterRanges[parameterOffset].first;
      const double maximum = laneFilterRanges[parameterOffset].second;
      const double normalizedDefault = kLaneFilterDefaults[lane][parameterOffset];
      const double defaultPlain = minimum + (normalizedDefault * (maximum - minimum));
      parameters.addParameter (makeRangeParam (laneFilterTitles[lane][parameterOffset], laneFilterParamID (lane, offset),
                                               laneFilterUnits[parameterOffset], minimum, maximum, defaultPlain));
    }
  }

  const std::array<const char*, kLaneCount> laneLedTitles {
    "Kick Trigger LED", "Snare Trigger LED", "Hat Trigger LED", "Perc A1 Trigger LED", "Perc A2 Trigger LED",
    "Perc B1 Trigger LED", "Perc B2 Trigger LED", "RimShot Trigger LED", "Clap Trigger LED"};
  for (int32 lane = 0; lane < kLaneCount; ++lane)
    parameters.addParameter (makeRangeParam (laneLedTitles[lane], laneLedParamID (lane), "", 0.0, 1.0, 0.0));

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

  const auto applyFilterDefaults = [this] () {
    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneFilterParamCount; ++parameterOffset)
      {
        setParamNormalized (laneFilterParamID (lane, static_cast<LaneFilterParamOffset> (parameterOffset)),
                            kLaneFilterDefaults[lane][parameterOffset]);
      }
    }
  };

  if (version == kLegacyStateVersion)
  {
    for (int32 param = 0; param < kPreviousGlobalParamCount; ++param)
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
    }
    applyMacroDefaults ();
    applyFilterDefaults ();
    setParamNormalized (kParamOscFilterCutoff, 0.20);
    setParamNormalized (kParamOscFilterResonance, 0.34);
    setParamNormalized (kParamOscFilterEnv, 0.46);
    return kResultOk;
  }

  if (version == kPreviousStateVersion)
  {
    constexpr int32 v2ParamCount =
      kPreviousGlobalParamCount + (kV4LaneCount * kLaneParamCount) + (kV4LaneCount * kLaneExtraParamCount);
    constexpr auto v2Ids = [] ()
    {
      std::array<Vst::ParamID, v2ParamCount> ids {};
      int32 index = 0;
      for (int32 i = 0; i < kPreviousGlobalParamCount; ++i)
        ids[index++] = static_cast<Vst::ParamID> (i);
      for (int32 lane = 0; lane < kV4LaneCount; ++lane)
        for (int32 p = 0; p < kLaneParamCount; ++p)
          ids[index++] = laneParamID (lane, static_cast<LaneParamOffset> (p));
      for (int32 lane = 0; lane < kV4LaneCount; ++lane)
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
    for (int32 lane = kV4LaneCount; lane < kLaneCount; ++lane)
    {
      for (int32 p = 0; p < kLaneParamCount; ++p)
        setParamNormalized (laneParamID (lane, static_cast<LaneParamOffset> (p)), 0.5);
      for (int32 p = 0; p < kLaneExtraParamCount; ++p)
        setParamNormalized (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p)),
                           kLaneExtraDefaults[lane][p]);
    }
    applyMacroDefaults ();
    applyFilterDefaults ();
    setParamNormalized (kParamOscFilterCutoff, 0.20);
    setParamNormalized (kParamOscFilterResonance, 0.34);
    setParamNormalized (kParamOscFilterEnv, 0.46);
    return kResultOk;
  }

  if (version == kV3StateVersion)
  {
    constexpr int32 v3ParamCount = kPreviousGlobalParamCount + (kV4LaneCount * kLaneParamCount) +
                                   (kV4LaneCount * kLaneExtraParamCount) + (kV4LaneCount * kLaneMacroParamCount);
    constexpr auto v3Ids = [] ()
    {
      std::array<Vst::ParamID, v3ParamCount> ids {};
      int32 index = 0;
      for (int32 i = 0; i < kPreviousGlobalParamCount; ++i)
        ids[index++] = static_cast<Vst::ParamID> (i);
      for (int32 lane = 0; lane < kV4LaneCount; ++lane)
        for (int32 p = 0; p < kLaneParamCount; ++p)
          ids[index++] = laneParamID (lane, static_cast<LaneParamOffset> (p));
      for (int32 lane = 0; lane < kV4LaneCount; ++lane)
        for (int32 p = 0; p < kLaneExtraParamCount; ++p)
          ids[index++] = laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p));
      for (int32 lane = 0; lane < kV4LaneCount; ++lane)
        for (int32 p = 0; p < kLaneMacroParamCount; ++p)
          ids[index++] = laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (p));
      return ids;
    }();

    for (const auto id : v3Ids)
    {
      double value = 0.0;
      if (!streamer.readDouble (value))
        return kResultFalse;
      setParamNormalized (id, value);
    }
    for (int32 lane = kV4LaneCount; lane < kLaneCount; ++lane)
    {
      for (int32 p = 0; p < kLaneParamCount; ++p)
        setParamNormalized (laneParamID (lane, static_cast<LaneParamOffset> (p)), 0.5);
      for (int32 p = 0; p < kLaneExtraParamCount; ++p)
        setParamNormalized (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p)),
                           kLaneExtraDefaults[lane][p]);
      for (int32 p = 0; p < kLaneMacroParamCount; ++p)
        setParamNormalized (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (p)),
                           kLaneMacroDefaults[lane][p]);
    }
    applyFilterDefaults ();
    setParamNormalized (kParamOscFilterCutoff, 0.20);
    setParamNormalized (kParamOscFilterResonance, 0.34);
    setParamNormalized (kParamOscFilterEnv, 0.46);
    return kResultOk;
  }

  if (version == kV4StateVersion)
  {
    constexpr int32 v4ParamCount = kParamGlobalCount + (kV4LaneCount * kLaneParamCount) +
                                   (kV4LaneCount * kLaneExtraParamCount) +
                                   (kV4LaneCount * kLaneMacroParamCount) +
                                   (kV4LaneCount * kLaneFilterParamCount);
    for (int32 i = 0; i < v4ParamCount; ++i)
    {
      double value = 0.0;
      if (!streamer.readDouble (value))
        return kResultFalse;
      setParamNormalized (allParameterIds ()[i], value);
    }
    for (int32 lane = kV4LaneCount; lane < kLaneCount; ++lane)
    {
      for (int32 p = 0; p < kLaneParamCount; ++p)
        setParamNormalized (laneParamID (lane, static_cast<LaneParamOffset> (p)), 0.5);
      for (int32 p = 0; p < kLaneExtraParamCount; ++p)
        setParamNormalized (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p)),
                           kLaneExtraDefaults[lane][p]);
      for (int32 p = 0; p < kLaneMacroParamCount; ++p)
        setParamNormalized (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (p)),
                           kLaneMacroDefaults[lane][p]);
      for (int32 p = 0; p < kLaneFilterParamCount; ++p)
        setParamNormalized (laneFilterParamID (lane, static_cast<LaneFilterParamOffset> (p)),
                           kLaneFilterDefaults[lane][p]);
    }
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
  {
    auto* editor = new VSTGUI::AspectRatioVST3Editor (this, "Editor", "WestCoastEditor.uidesc");
    editor->setDelegate (this);
    editor->setMinZoomFactor (0.72);
    editor->setEditorSizeConstrains (VSTGUI::CPoint (980., 420.), VSTGUI::CPoint (2600., 900.));
    return editor;
  }
  return nullptr;
}

VSTGUI::CView* WestCoastController::verifyView (VSTGUI::CView* view, const VSTGUI::UIAttributes& /*attributes*/,
                                                const VSTGUI::IUIDescription* /*description*/,
                                                VSTGUI::VST3Editor* /*editor*/)
{
  auto bounds = view->getViewSize ();
  bool boundsChanged = false;

  VSTGUI::CCoord layoutScale = 1.0;
  const bool isMainWideLayout = bounds.getWidth () > 2500.0;
  const bool isModulePanel =
    bounds.getWidth () > 280.0 && bounds.getWidth () < 320.0 && bounds.getHeight () > 500.0;

  if (isMainWideLayout || isModulePanel)
  {
    layoutScale = kCompactScaleX;
  }
  else if (auto* parent = view->getParentView ())
  {
    const VSTGUI::CCoord parentWidth = parent->getViewSize ().getWidth ();
    if (parentWidth >= 270.0 && parentWidth <= 330.0)
      layoutScale = kCompactScaleX;
    else if (parentWidth > 120.0 && parentWidth < 190.0)
      layoutScale = parentWidth / kModuleBaseWidth;
  }

  if (layoutScale < 0.999 || layoutScale > 1.001)
  {
    bounds = scaleRectX (bounds, layoutScale);
    boundsChanged = true;
  }

  if (auto* slider = dynamic_cast<VSTGUI::CSlider*> (view))
  {
    slider->setFrameColor (VSTGUI::CColor (75, 82, 91, 255));
    slider->setBackColor (VSTGUI::CColor (34, 37, 41, 255));

    if (slider->isStyleHorizontal ())
    {
      if (!slider->getMouseEnabled () || bounds.getHeight () <= 12.0)
        slider->setValueColor (VSTGUI::CColor (128, 212, 153, 255));
      else
        slider->setValueColor (VSTGUI::CColor (137, 182, 214, 255));
    }
    else
    {
      slider->setValueColor (VSTGUI::CColor (255, 159, 91, 255));

      constexpr VSTGUI::CCoord kTargetSliderWidth = 20.0;
      if (bounds.getWidth () > kTargetSliderWidth + 0.1)
      {
        const VSTGUI::CCoord inset = (bounds.getWidth () - kTargetSliderWidth) * 0.5;
        bounds.left += inset;
        bounds.right -= inset;
        boundsChanged = true;
      }
    }
  }

  if (boundsChanged)
  {
    view->setViewSize (bounds, true);
    view->setMouseableArea (bounds);
    view->invalid ();
  }

  return view;
}

bool WestCoastController::isPrivateParameter (const Vst::ParamID paramID)
{
  return isLaneLedParamID (paramID);
}

} // namespace Steinberg::WestCoastDrumSynth
