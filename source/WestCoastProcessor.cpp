#include "WestCoastProcessor.h"

#include "presets/FactoryPresets.h"
#include "westcoastdrumcids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <type_traits>

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

inline double clamp01 (double x)
{
  return std::clamp (x, 0.0, 1.0);
}

inline double softClip (double x)
{
  constexpr double kSoftClipDrive = 1.4;
  return std::tanh (x * kSoftClipDrive) / std::tanh (kSoftClipDrive);
}

inline int32 presetIndexFromNormalized (double normalized)
{
  const int32 maxIndex = kFactoryPresetCount - 1;
  return std::clamp (static_cast<int32> (std::lround (clamp01 (normalized) * maxIndex)), 0, maxIndex);
}

inline double normalizedFromPresetIndex (int32 presetIndex)
{
  if (kFactoryPresetCount <= 1)
    return 0.0;
  return static_cast<double> (std::clamp (presetIndex, 0, kFactoryPresetCount - 1)) /
         static_cast<double> (kFactoryPresetCount - 1);
}

inline int32 laneForLegacyDrumMap (int16 pitch)
{
  switch (pitch)
  {
    case 36:
    case 35:
      return 0;
    case 38:
    case 40:
      return 1;
    case 42:
    case 44:
    case 46:
      return 2;
    case 48:
    case 50:
      return 3;
    case 52:
    case 54:
      return 4;
    case 47:
    case 49:
      return 5;
    case 51:
    case 53:
      return 6;
    case 37:
    case 39:
      return 7;
    case 41:
    case 43:
      return 8;
    default:
      return -1;
  }
}

inline int32 laneForMidiPitch (int16 pitch)
{
  if (pitch < 0 || pitch > 127)
    return -1;

  // Chromatic keyzones: C through G# in every octave map directly to the 9 lanes.
  // This keeps the layout predictable while preserving legacy GM-note compatibility.
  constexpr int16 kKeyzoneRoot = 24; // C0
  if (pitch >= kKeyzoneRoot)
  {
    const int16 noteInOctave = static_cast<int16> ((pitch - kKeyzoneRoot) % 12);
    if (noteInOctave >= 0 && noteInOctave < kLaneCount)
      return noteInOctave;
  }

  return laneForLegacyDrumMap (pitch);
}

inline int32 octaveOffsetFromPitch (int16 pitch)
{
  constexpr int16 kTrackingCenterNote = 60; // C3
  const double octavesFromCenter = (static_cast<double> (pitch) - static_cast<double> (kTrackingCenterNote)) / 12.0;
  return static_cast<int32> (std::floor (octavesFromCenter));
}

} // namespace

WestCoastProcessor::WestCoastProcessor ()
{
  setControllerClass (kControllerUID);
  params_.fill (0.0);
  laneLedState_.fill (-1.0);
  laneLedFlashSamples_.fill (0);
}

FUnknown* WestCoastProcessor::createInstance (void*)
{
  return static_cast<Vst::IAudioProcessor*> (new WestCoastProcessor ());
}

tresult PLUGIN_API WestCoastProcessor::initialize (FUnknown* context)
{
  const tresult result = AudioEffect::initialize (context);
  if (result != kResultOk)
    return result;

  addAudioOutput (STR16 ("Stereo Out"), Vst::SpeakerArr::kStereo);
  addEventInput (STR16 ("MIDI In"), 16);

  setParam (kParamMaster, 0.80);
  setParam (kParamInternalTempo, 0.50);
  setParam (kParamSwing, 0.12);
  setParam (kParamRun, 0.0);
  setParam (kParamFollowTransport, 0.0);
  setParam (kParamPresetSelect, 0.0);
  setParam (kParamRandomize, 0.0);
  setParam (kParamOscFilterCutoff, 0.20);
  setParam (kParamOscFilterResonance, 0.34);
  setParam (kParamOscFilterEnv, 0.46);

  loadPresetByIndex (0, nullptr);
  updateLaneFramesFromParameters ();
  return kResultOk;
}

tresult PLUGIN_API WestCoastProcessor::terminate ()
{
  return AudioEffect::terminate ();
}

tresult PLUGIN_API WestCoastProcessor::setState (IBStream* state)
{
  if (!state)
    return kResultFalse;

  IBStreamer streamer (state, kLittleEndian);

  uint32 version = 0;
  if (!streamer.readInt32u (version))
    return kResultFalse;

  int32 savedPreset = 0;
  if (!streamer.readInt32 (savedPreset))
    return kResultFalse;
  loadedPreset_ = std::clamp (savedPreset, 0, kFactoryPresetCount - 1);

  const auto applyMacroDefaults = [this] () {
    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneMacroParamCount; ++parameterOffset)
      {
        setParam (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (parameterOffset)),
                  kLaneMacroDefaults[lane][parameterOffset]);
      }
    }
  };

  const auto applyFilterDefaults = [this] () {
    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 parameterOffset = 0; parameterOffset < kLaneFilterParamCount; ++parameterOffset)
      {
        setParam (laneFilterParamID (lane, static_cast<LaneFilterParamOffset> (parameterOffset)),
                  kLaneFilterDefaults[lane][parameterOffset]);
      }
    }
  };

  if (version == kLegacyStateVersion)
  {
    for (int32 param = 0; param < kPreviousGlobalParamCount; ++param)
    {
      double normalized = 0.0;
      if (!streamer.readDouble (normalized))
        return kResultFalse;
      setParam (static_cast<Vst::ParamID> (param), normalized);
    }

    for (int32 lane = 0; lane < kLegacyLaneCount; ++lane)
    {
      for (int32 param = 0; param < kLaneParamCount; ++param)
      {
        double normalized = 0.0;
        if (!streamer.readDouble (normalized))
          return kResultFalse;
        setParam (laneParamID (lane, static_cast<LaneParamOffset> (param)), normalized);
      }
    }

    for (int32 param = 0; param < kLaneParamCount; ++param)
    {
      const auto offset = static_cast<LaneParamOffset> (param);
      double value = getParam (laneParamID (3, offset));
      if (offset == kLanePan)
        value = clamp01 (value + 0.08);
      setParam (laneParamID (4, offset), value);
    }

    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 param = 0; param < kLaneExtraParamCount; ++param)
        setParam (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (param)),
                  kLaneExtraDefaults[lane][param]);
    }
    applyMacroDefaults ();
    applyFilterDefaults ();

    setParam (kParamOscFilterCutoff, 0.20);
    setParam (kParamOscFilterResonance, 0.34);
    setParam (kParamOscFilterEnv, 0.46);

    const auto& preset = getFactoryPresets ()[loadedPreset_];
    sequencer_.setPattern (preset.pattern);
    updateLaneFramesFromParameters ();
    presetPending_ = false;
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
      double normalized = 0.0;
      if (!streamer.readDouble (normalized))
        return kResultFalse;
      setParam (id, normalized);
    }
    for (int32 lane = kV4LaneCount; lane < kLaneCount; ++lane)
    {
      for (int32 p = 0; p < kLaneParamCount; ++p)
        setParam (laneParamID (lane, static_cast<LaneParamOffset> (p)), 0.5);
      for (int32 p = 0; p < kLaneExtraParamCount; ++p)
        setParam (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p)),
                  kLaneExtraDefaults[lane][p]);
    }
    applyMacroDefaults ();
    applyFilterDefaults ();

    const auto& preset = getFactoryPresets ()[loadedPreset_];
    sequencer_.setPattern (preset.pattern);
    updateLaneFramesFromParameters ();
    presetPending_ = false;
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
      double normalized = 0.0;
      if (!streamer.readDouble (normalized))
        return kResultFalse;
      setParam (id, normalized);
    }
    for (int32 lane = kV4LaneCount; lane < kLaneCount; ++lane)
    {
      for (int32 p = 0; p < kLaneParamCount; ++p)
        setParam (laneParamID (lane, static_cast<LaneParamOffset> (p)), 0.5);
      for (int32 p = 0; p < kLaneExtraParamCount; ++p)
        setParam (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p)),
                  kLaneExtraDefaults[lane][p]);
      for (int32 p = 0; p < kLaneMacroParamCount; ++p)
        setParam (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (p)),
                  kLaneMacroDefaults[lane][p]);
    }
    applyFilterDefaults ();

    setParam (kParamOscFilterCutoff, 0.20);
    setParam (kParamOscFilterResonance, 0.34);
    setParam (kParamOscFilterEnv, 0.46);

    const auto& preset = getFactoryPresets ()[loadedPreset_];
    sequencer_.setPattern (preset.pattern);
    updateLaneFramesFromParameters ();
    presetPending_ = false;
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
      double normalized = 0.0;
      if (!streamer.readDouble (normalized))
        return kResultFalse;
      setParam (allParameterIds ()[i], normalized);
    }
    for (int32 lane = kV4LaneCount; lane < kLaneCount; ++lane)
    {
      for (int32 p = 0; p < kLaneParamCount; ++p)
        setParam (laneParamID (lane, static_cast<LaneParamOffset> (p)), 0.5);
      for (int32 p = 0; p < kLaneExtraParamCount; ++p)
        setParam (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (p)),
                  kLaneExtraDefaults[lane][p]);
      for (int32 p = 0; p < kLaneMacroParamCount; ++p)
        setParam (laneMacroParamID (lane, static_cast<LaneMacroParamOffset> (p)),
                  kLaneMacroDefaults[lane][p]);
      for (int32 p = 0; p < kLaneFilterParamCount; ++p)
        setParam (laneFilterParamID (lane, static_cast<LaneFilterParamOffset> (p)),
                  kLaneFilterDefaults[lane][p]);
    }
    const auto& preset = getFactoryPresets ()[loadedPreset_];
    sequencer_.setPattern (preset.pattern);
    updateLaneFramesFromParameters ();
    presetPending_ = false;
    return kResultOk;
  }

  if (version != kStateVersion)
    return kResultFalse;

  for (const auto id : allParameterIds ())
  {
    double normalized = 0.0;
    if (!streamer.readDouble (normalized))
      return kResultFalse;
    setParam (id, normalized);
  }

  const auto& preset = getFactoryPresets ()[loadedPreset_];
  sequencer_.setPattern (preset.pattern);
  updateLaneFramesFromParameters ();
  presetPending_ = false;
  return kResultOk;
}

tresult PLUGIN_API WestCoastProcessor::getState (IBStream* state)
{
  if (!state)
    return kResultFalse;

  IBStreamer streamer (state, kLittleEndian);
  if (!streamer.writeInt32u (kStateVersion))
    return kResultFalse;
  if (!streamer.writeInt32 (loadedPreset_))
    return kResultFalse;

  for (const auto id : allParameterIds ())
  {
    if (!streamer.writeDouble (getParam (id)))
      return kResultFalse;
  }

  return kResultOk;
}

tresult PLUGIN_API WestCoastProcessor::setBusArrangements (Vst::SpeakerArrangement* inputs, int32 numIns,
                                                            Vst::SpeakerArrangement* outputs, int32 numOuts)
{
  if (numIns != 0 || numOuts != 1)
    return kResultFalse;
  if (outputs[0] != Vst::SpeakerArr::kStereo)
    return kResultFalse;
  return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
}

tresult PLUGIN_API WestCoastProcessor::setupProcessing (Vst::ProcessSetup& setup)
{
  const tresult result = AudioEffect::setupProcessing (setup);
  if (result != kResultOk)
    return result;

  sequencer_.setSampleRate (setup.sampleRate);
  for (auto& voice : voices_)
    voice.setSampleRate (setup.sampleRate);
  ledFlashDurationSamples_ = std::max<int32> (1, static_cast<int32> (std::lround (setup.sampleRate * 0.045)));
  return kResultOk;
}

tresult PLUGIN_API WestCoastProcessor::setActive (TBool state)
{
  if (state)
    resetEngine ();
  return AudioEffect::setActive (state);
}

tresult PLUGIN_API WestCoastProcessor::process (Vst::ProcessData& data)
{
  processParameterChanges (data.inputParameterChanges, data.outputParameterChanges);
  if (presetPending_)
    loadPresetByIndex (loadedPreset_, data.outputParameterChanges);

  bool hostPlaying = false;
  bool hostTempoValid = false;
  bool hostProjectTimeValid = false;
  double hostTempo = 120.0;
  double hostPpq = 0.0;

  if (data.processContext)
  {
    hostPlaying = (data.processContext->state & Vst::ProcessContext::kPlaying) != 0;
    hostTempoValid = (data.processContext->state & Vst::ProcessContext::kTempoValid) != 0;
    hostProjectTimeValid = (data.processContext->state & Vst::ProcessContext::kProjectTimeMusicValid) != 0;
    hostTempo = data.processContext->tempo;
    hostPpq = data.processContext->projectTimeMusic;
  }

  const bool followTransport = getParam (kParamFollowTransport) > 0.5;
  const bool run = getParam (kParamRun) > 0.5;
  const double internalTempo = 60.0 + (getParam (kParamInternalTempo) * 120.0);
  const double tempo = (followTransport && hostTempoValid) ? hostTempo : internalTempo;

  sequencer_.setTempo (tempo);
  sequencer_.setSwing (getParam (kParamSwing));
  sequencer_.setRunning (run && (!followTransport || hostPlaying));

  if (followTransport && hostProjectTimeValid && hostPlaying)
    sequencer_.syncToHost (hostPpq, hostPlaying);

  if (data.inputEvents)
  {
    const int32 eventCount = data.inputEvents->getEventCount ();
    for (int32 eventIndex = 0; eventIndex < eventCount; ++eventIndex)
    {
      Vst::Event event {};
      if (data.inputEvents->getEvent (eventIndex, event) != kResultOk)
        continue;
      if (event.type != Vst::Event::kNoteOnEvent || event.noteOn.velocity <= 0.f)
        continue;

      const int32 lane = laneForMidiPitch (event.noteOn.pitch);
      if (lane >= 0 && lane < kLaneCount)
      {
        voices_[lane].trigger (laneFrameForMidiPitch (lane, event.noteOn.pitch));
        laneLedFlashSamples_[lane] = ledFlashDurationSamples_;
      }
    }
  }

  if (data.numOutputs == 0 || data.outputs == nullptr || data.numSamples <= 0)
    return kResultOk;

  const double master = std::pow (clamp01 (getParam (kParamMaster)), 1.35) * 1.15;
  std::array<bool, kLaneCount> triggers {};

  auto render = [&] (auto** outChannels)
  {
    using SampleType = std::remove_pointer_t<std::decay_t<decltype (outChannels[0])>>;

    auto* left = outChannels[0];
    auto* right = (data.outputs[0].numChannels > 1) ? outChannels[1] : outChannels[0];

    for (int32 sampleIndex = 0; sampleIndex < data.numSamples; ++sampleIndex)
    {
      sequencer_.processSample (triggers);
      for (int32 lane = 0; lane < kLaneCount; ++lane)
      {
        if (triggers[lane])
        {
          voices_[lane].trigger (laneFrames_[lane]);
          laneLedFlashSamples_[lane] = ledFlashDurationSamples_;
        }
      }

      double frameL = 0.0;
      double frameR = 0.0;
      int32 activeVoices = 0;
      for (int32 lane = 0; lane < kLaneCount; ++lane)
      {
        if (voices_[lane].isActive ())
          ++activeVoices;
        const double sample = voices_[lane].process ();
        const double pan = std::clamp (laneFrames_[lane].pan, -1.0, 1.0);
        const double gainL = std::sqrt (0.5 * (1.0 - pan));
        const double gainR = std::sqrt (0.5 * (1.0 + pan));
        frameL += sample * gainL;
        frameR += sample * gainR;
      }

      const double voiceNorm = 1.0 / std::sqrt (static_cast<double> (std::max<int32> (activeVoices, 1)));
      const double busL = softClip (frameL * voiceNorm * master);
      const double busR = softClip (frameR * voiceNorm * master);

      left[sampleIndex] = static_cast<SampleType> (busL);
      right[sampleIndex] = static_cast<SampleType> (busR);

      for (int32 lane = 0; lane < kLaneCount; ++lane)
      {
        if (laneLedFlashSamples_[lane] > 0)
          --laneLedFlashSamples_[lane];
      }
    }
  };

  if (data.symbolicSampleSize == Vst::kSample32)
    render (data.outputs[0].channelBuffers32);
  else if (data.symbolicSampleSize == Vst::kSample64)
    render (data.outputs[0].channelBuffers64);
  else
    return kResultFalse;

  data.outputs[0].silenceFlags = 0;

  if (data.outputParameterChanges)
  {
    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      const double ledValue = laneLedFlashSamples_[lane] > 0 ? 1.0 : 0.0;
      if (std::abs (laneLedState_[lane] - ledValue) > 0.5)
      {
        pushParamChange (data.outputParameterChanges, laneLedParamID (lane), ledValue);
        laneLedState_[lane] = ledValue;
      }
    }
  }

  return kResultOk;
}

void WestCoastProcessor::resetEngine ()
{
  sequencer_.reset ();
  for (auto& voice : voices_)
    voice.reset ();
  laneLedState_.fill (-1.0);
  laneLedFlashSamples_.fill (0);
}

void WestCoastProcessor::loadPresetByIndex (int32 presetIndex, Vst::IParameterChanges* outputChanges)
{
  const auto& presets = getFactoryPresets ();
  loadedPreset_ = std::clamp (presetIndex, 0, kFactoryPresetCount - 1);
  const auto& preset = presets[loadedPreset_];

  setParam (kParamMaster, preset.master);
  setParam (kParamInternalTempo, preset.internalTempo);
  setParam (kParamSwing, preset.swing);
  setParam (kParamPresetSelect, normalizedFromPresetIndex (loadedPreset_));

  pushParamChange (outputChanges, kParamMaster, getParam (kParamMaster));
  pushParamChange (outputChanges, kParamInternalTempo, getParam (kParamInternalTempo));
  pushParamChange (outputChanges, kParamSwing, getParam (kParamSwing));
  pushParamChange (outputChanges, kParamPresetSelect, getParam (kParamPresetSelect));

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    const auto& lanePreset = preset.lanes[lane];
    const double transientDecayMacro = std::clamp (0.18 + (lanePreset.decay * 0.62), 0.0, 1.0);
    const double transientMixMacro = std::clamp (0.24 + (lanePreset.transientAttack * 0.70), 0.0, 1.0);
    const double noiseResMacro = std::clamp (0.20 + (lanePreset.snap * 0.68), 0.0, 1.0);
    const double noiseEnvMacro = std::clamp (0.28 + (lanePreset.noiseDecay * 0.56), 0.0, 1.0);

    setParam (laneParamID (lane, kLaneTune), lanePreset.tune);
    setParam (laneParamID (lane, kLaneDecay), lanePreset.decay);
    setParam (laneParamID (lane, kLaneFold), lanePreset.fold);
    setParam (laneParamID (lane, kLaneFm), lanePreset.fm);
    setParam (laneParamID (lane, kLaneNoise), lanePreset.noise);
    setParam (laneParamID (lane, kLaneDrive), lanePreset.drive);
    setParam (laneParamID (lane, kLaneLevel), lanePreset.level);
    setParam (laneParamID (lane, kLanePan), lanePreset.pan);
    setParam (laneExtraParamID (lane, kLanePitchEnvAmount), lanePreset.pitchEnvAmount);
    setParam (laneExtraParamID (lane, kLanePitchEnvDecay), lanePreset.pitchEnvDecay);
    setParam (laneExtraParamID (lane, kLaneTransientAttack), lanePreset.transientAttack);
    setParam (laneExtraParamID (lane, kLaneNoiseTone), lanePreset.noiseTone);
    setParam (laneExtraParamID (lane, kLaneNoiseDecay), lanePreset.noiseDecay);
    setParam (laneExtraParamID (lane, kLaneSnap), lanePreset.snap);
    setParam (laneMacroParamID (lane, kLaneTransientDecay), transientDecayMacro);
    setParam (laneMacroParamID (lane, kLaneTransientMix), transientMixMacro);
    setParam (laneMacroParamID (lane, kLaneNoiseResonance), noiseResMacro);
    setParam (laneMacroParamID (lane, kLaneNoiseEnvAmount), noiseEnvMacro);
    setParam (laneFilterParamID (lane, kLaneOscFilterCutoff), lanePreset.oscFilterCutoff);
    setParam (laneFilterParamID (lane, kLaneOscFilterRes), lanePreset.oscFilterRes);
    setParam (laneFilterParamID (lane, kLaneOscFilterEnv), lanePreset.oscFilterEnv);
    setParam (laneFilterParamID (lane, kLaneTransFilterCutoff), lanePreset.transFilterCutoff);
    setParam (laneFilterParamID (lane, kLaneTransFilterRes), lanePreset.transFilterRes);
    setParam (laneFilterParamID (lane, kLaneTransFilterEnv), lanePreset.transFilterEnv);

    pushParamChange (outputChanges, laneParamID (lane, kLaneTune), lanePreset.tune);
    pushParamChange (outputChanges, laneParamID (lane, kLaneDecay), lanePreset.decay);
    pushParamChange (outputChanges, laneParamID (lane, kLaneFold), lanePreset.fold);
    pushParamChange (outputChanges, laneParamID (lane, kLaneFm), lanePreset.fm);
    pushParamChange (outputChanges, laneParamID (lane, kLaneNoise), lanePreset.noise);
    pushParamChange (outputChanges, laneParamID (lane, kLaneDrive), lanePreset.drive);
    pushParamChange (outputChanges, laneParamID (lane, kLaneLevel), lanePreset.level);
    pushParamChange (outputChanges, laneParamID (lane, kLanePan), lanePreset.pan);
    pushParamChange (outputChanges, laneExtraParamID (lane, kLanePitchEnvAmount), lanePreset.pitchEnvAmount);
    pushParamChange (outputChanges, laneExtraParamID (lane, kLanePitchEnvDecay), lanePreset.pitchEnvDecay);
    pushParamChange (outputChanges, laneExtraParamID (lane, kLaneTransientAttack), lanePreset.transientAttack);
    pushParamChange (outputChanges, laneExtraParamID (lane, kLaneNoiseTone), lanePreset.noiseTone);
    pushParamChange (outputChanges, laneExtraParamID (lane, kLaneNoiseDecay), lanePreset.noiseDecay);
    pushParamChange (outputChanges, laneExtraParamID (lane, kLaneSnap), lanePreset.snap);
    pushParamChange (outputChanges, laneMacroParamID (lane, kLaneTransientDecay), transientDecayMacro);
    pushParamChange (outputChanges, laneMacroParamID (lane, kLaneTransientMix), transientMixMacro);
    pushParamChange (outputChanges, laneMacroParamID (lane, kLaneNoiseResonance), noiseResMacro);
    pushParamChange (outputChanges, laneMacroParamID (lane, kLaneNoiseEnvAmount), noiseEnvMacro);
    pushParamChange (outputChanges, laneFilterParamID (lane, kLaneOscFilterCutoff), lanePreset.oscFilterCutoff);
    pushParamChange (outputChanges, laneFilterParamID (lane, kLaneOscFilterRes), lanePreset.oscFilterRes);
    pushParamChange (outputChanges, laneFilterParamID (lane, kLaneOscFilterEnv), lanePreset.oscFilterEnv);
    pushParamChange (outputChanges, laneFilterParamID (lane, kLaneTransFilterCutoff), lanePreset.transFilterCutoff);
    pushParamChange (outputChanges, laneFilterParamID (lane, kLaneTransFilterRes), lanePreset.transFilterRes);
    pushParamChange (outputChanges, laneFilterParamID (lane, kLaneTransFilterEnv), lanePreset.transFilterEnv);
  }

  sequencer_.setPattern (preset.pattern);
  updateLaneFramesFromParameters ();
  presetPending_ = false;
}

void WestCoastProcessor::processParameterChanges (Vst::IParameterChanges* changes,
                                                  Vst::IParameterChanges* outputChanges)
{
  if (!changes)
    return;

  const int32 numParamsChanged = changes->getParameterCount ();
  for (int32 paramIdx = 0; paramIdx < numParamsChanged; ++paramIdx)
  {
    Vst::IParamValueQueue* queue = changes->getParameterData (paramIdx);
    if (!queue)
      continue;

    const int32 pointCount = queue->getPointCount ();
    if (pointCount <= 0)
      continue;

    int32 sampleOffset = 0;
    Vst::ParamValue value = 0.0;
    if (queue->getPoint (pointCount - 1, sampleOffset, value) != kResultOk)
      continue;

    const auto paramId = queue->getParameterId ();
    setParam (paramId, value);

    if (paramId == kParamPresetSelect)
    {
      loadedPreset_ = presetIndexFromNormalized (value);
      presetPending_ = true;
    }
    else if (paramId == kParamRandomize && value > 0.5)
    {
      performRandomization (outputChanges);
      setParam (kParamRandomize, 0.0);
      if (outputChanges)
        pushParamChange (outputChanges, kParamRandomize, 0.0);
    }
  }

  updateLaneFramesFromParameters ();

  if (presetPending_ && outputChanges)
    pushParamChange (outputChanges, kParamPresetSelect, getParam (kParamPresetSelect));
}

void WestCoastProcessor::performRandomization (Vst::IParameterChanges* outputChanges)
{
  if (!outputChanges)
    return;

  std::random_device rd;
  std::mt19937 gen (rd ());
  const auto rnd = [&gen] (double lo, double hi) {
    return std::uniform_real_distribution<double> (lo, hi) (gen);
  };
  const auto jitter = [&rnd] (double center, double radius) {
    return clamp01 (center + rnd (-radius, radius));
  };

  constexpr double decayJitter = 0.10;
  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    const double curDecay = getParam (laneParamID (lane, kLaneDecay));
    const double curPitchEnvDecay = getParam (laneExtraParamID (lane, kLanePitchEnvDecay));
    const double curNoiseDecay = getParam (laneExtraParamID (lane, kLaneNoiseDecay));
    const double curTransientDecay = getParam (laneMacroParamID (lane, kLaneTransientDecay));

    setParam (laneParamID (lane, kLaneTune), rnd (0.32, 0.58));
    setParam (laneParamID (lane, kLaneDecay), jitter (curDecay, decayJitter));
    setParam (laneParamID (lane, kLaneFold), rnd (0.25, 0.75));
    setParam (laneParamID (lane, kLaneFm), rnd (0.20, 0.65));
    setParam (laneParamID (lane, kLaneNoise), rnd (0.15, 0.55));
    setParam (laneParamID (lane, kLaneDrive), rnd (0.10, 0.45));
    setParam (laneParamID (lane, kLaneLevel), rnd (0.55, 0.92));
    setParam (laneParamID (lane, kLanePan), rnd (0.25, 0.75));

    setParam (laneExtraParamID (lane, kLanePitchEnvAmount), rnd (0.25, 0.75));
    setParam (laneExtraParamID (lane, kLanePitchEnvDecay), jitter (curPitchEnvDecay, decayJitter));
    setParam (laneExtraParamID (lane, kLaneTransientAttack), rnd (0.18, 0.55));
    setParam (laneExtraParamID (lane, kLaneNoiseTone), rnd (0.35, 0.75));
    setParam (laneExtraParamID (lane, kLaneNoiseDecay), jitter (curNoiseDecay, decayJitter));
    setParam (laneExtraParamID (lane, kLaneSnap), rnd (0.20, 0.65));

    setParam (laneMacroParamID (lane, kLaneTransientDecay), jitter (curTransientDecay, decayJitter));
    setParam (laneMacroParamID (lane, kLaneTransientMix), rnd (0.30, 0.70));
    setParam (laneMacroParamID (lane, kLaneNoiseResonance), rnd (0.25, 0.65));
    setParam (laneMacroParamID (lane, kLaneNoiseEnvAmount), rnd (0.35, 0.75));

    setParam (laneFilterParamID (lane, kLaneOscFilterCutoff), rnd (0.50, 0.85));
    setParam (laneFilterParamID (lane, kLaneOscFilterRes), rnd (0.02, 0.18));
    setParam (laneFilterParamID (lane, kLaneOscFilterEnv), rnd (0.20, 0.55));
    setParam (laneFilterParamID (lane, kLaneTransFilterCutoff), rnd (0.55, 0.88));
    setParam (laneFilterParamID (lane, kLaneTransFilterRes), rnd (0.02, 0.12));
    setParam (laneFilterParamID (lane, kLaneTransFilterEnv), rnd (0.22, 0.55));
  }

  for (const auto id : allParameterIds ())
  {
    if (id != kParamRandomize && id != kParamPresetSelect && id != kParamMaster && id != kParamInternalTempo &&
        id != kParamSwing && id != kParamRun && id != kParamFollowTransport &&
        id < kLaneParamBase)
      continue;
    if (id >= kLaneParamBase && id <= kLaneFilterMaxParamId)
      pushParamChange (outputChanges, id, getParam (id));
  }
}

LaneFrame WestCoastProcessor::laneFrameForMidiPitch (int32 lane, int16 pitch) const
{
  LaneFrame frame = laneFrames_[lane];
  if (pitch < 0 || pitch > 127)
    return frame;

  // Keep lane mapping stable, but shape timbre by played octave so lower notes
  // stay cleaner/rounder and higher notes stay brighter.
  const int32 octaveOffset = std::clamp (octaveOffsetFromPitch (pitch), -3, 2);
  if (octaveOffset == 0)
    return frame;

  frame.frequencyHz =
    std::clamp (frame.frequencyHz * std::pow (2.0, static_cast<double> (octaveOffset)), 8.0, 20000.0);

  const double lowBias = clamp01 (static_cast<double> (-std::min (octaveOffset, 0)) / 3.0);
  const double highBias = clamp01 (static_cast<double> (std::max (octaveOffset, 0)) / 2.0);

  const double harmonicScale = std::clamp ((1.0 - (0.55 * lowBias)) + (0.20 * highBias), 0.35, 1.25);
  const double noiseScale = std::clamp ((1.0 - (0.65 * lowBias)) + (0.25 * highBias), 0.25, 1.35);
  const double transientScale = std::clamp ((1.0 - (0.25 * lowBias)) + (0.15 * highBias), 0.45, 1.25);

  frame.fmAmount = clamp01 (frame.fmAmount * harmonicScale);
  frame.foldAmount = clamp01 (frame.foldAmount * harmonicScale);
  frame.noiseAmount = std::clamp (frame.noiseAmount * noiseScale, 0.0, 2.5);
  frame.noiseLevel = std::clamp (frame.noiseLevel * noiseScale, 0.0, 2.5);
  frame.transientAmount = clamp01 (frame.transientAmount * transientScale);
  frame.transientLevel = std::clamp (frame.transientLevel * transientScale, 0.0, 2.5);
  frame.pitchEnvAmount = clamp01 (frame.pitchEnvAmount * (1.0 - (0.30 * lowBias)));
  frame.decaySeconds =
    std::clamp (frame.decaySeconds * (1.0 + (0.30 * lowBias) - (0.08 * highBias)), 0.01, 2.5);

  const double bodyCutoffScale = std::clamp ((1.0 - (0.32 * lowBias)) + (0.18 * highBias), 0.55, 1.20);
  const double noiseCutoffScale = std::clamp ((1.0 - (0.45 * lowBias)) + (0.25 * highBias), 0.40, 1.35);
  frame.bodyFilterCutoffHz = std::clamp (frame.bodyFilterCutoffHz * bodyCutoffScale, 80.0, 18000.0);
  frame.noiseFilterCutoffHz = std::clamp (frame.noiseFilterCutoffHz * noiseCutoffScale, 120.0, 18000.0);

  return frame;
}

void WestCoastProcessor::updateLaneFramesFromParameters ()
{
  // Kick, Snare, Hat, PercA1, PercA2 (low bass), PercB1, PercB2 (higher), RimShot, Clap
  static constexpr std::array<double, kLaneCount> kBaseFrequencies {
    52.0, 185.0, 3800.0, 45.0, 65.0, 520.0, 820.0, 950.0, 650.0};
  static constexpr std::array<LaneCharacter, kLaneCount> kLaneCharacters {
    LaneCharacter::Kick, LaneCharacter::Snare, LaneCharacter::Hat,
    LaneCharacter::PercA, LaneCharacter::PercA, LaneCharacter::PercB, LaneCharacter::PercB,
    LaneCharacter::RimShot, LaneCharacter::Clap};
  static constexpr std::array<double, kLaneCount> kPitchEnvScale {
    1.0, 0.60, 0.28, 0.85, 0.82, 0.72, 0.68, 0.58, 0.48};
  static constexpr std::array<double, kLaneCount> kTransientAttackScale {
    1.0, 0.92, 0.74, 0.90, 0.88, 0.92, 0.90, 0.95, 0.88};
  static constexpr std::array<double, kLaneCount> kTransientDecayScale {
    1.0, 1.16, 0.68, 1.05, 1.02, 0.94, 0.90, 0.85, 0.78};
  static constexpr std::array<double, kLaneCount> kTransientLevelScale {
    1.0, 1.10, 0.88, 1.00, 0.98, 1.02, 1.00, 1.08, 1.12};
  static constexpr std::array<double, kLaneCount> kNoiseLevelScale {
    1.05, 2.05, 1.65, 1.35, 1.38, 1.42, 1.45, 1.55, 1.75};
  static constexpr std::array<double, kLaneCount> kNoiseDecayScale {
    0.82, 1.58, 0.94, 1.15, 1.12, 1.08, 1.05, 0.95, 0.88};
  static constexpr std::array<double, kLaneCount> kNoiseResScale {
    0.90, 1.04, 1.12, 0.98, 1.00, 1.02, 1.04, 1.08, 1.12};
  static constexpr std::array<double, kLaneCount> kNoiseEnvScale {
    0.88, 1.02, 1.18, 0.98, 1.00, 1.05, 1.08, 1.12, 1.18};
  static constexpr std::array<double, kLaneCount> kSnapScale {
    0.24, 1.0, 0.86, 0.55, 0.52, 0.62, 0.60, 0.75, 0.68};
  static constexpr std::array<double, kLaneCount> kOscCutoffScale {
    0.62, 0.88, 1.70, 1.08, 1.06, 1.12, 1.10, 1.15, 1.18};
  static constexpr std::array<double, kLaneCount> kOscResScale {
    1.08, 1.0, 0.82, 0.96, 0.98, 1.00, 1.02, 1.05, 1.08};
  static constexpr std::array<double, kLaneCount> kOscEnvScale {
    1.24, 1.02, 0.58, 0.92, 0.90, 0.88, 0.86, 0.82, 0.78};
  static constexpr std::array<double, kLaneCount> kOscBalance {
    1.0, 0.92, 0.76, 0.94, 0.92, 0.90, 0.88, 0.86, 0.82};
  // Wider tune travel for sub rumbles and bright metallic percussion.
  static constexpr std::array<double, kLaneCount> kPitchSemitoneRange {
    60.0, 52.0, 48.0, 60.0, 58.0, 62.0, 64.0, 56.0, 54.0};

  const double oscFilterCutoffNorm = getParam (kParamOscFilterCutoff);
  const double oscFilterResNorm = getParam (kParamOscFilterResonance);
  const double oscFilterEnvNorm = getParam (kParamOscFilterEnv);
  const double globalOscCutoffHz = 90.0 + (std::pow (oscFilterCutoffNorm, 1.80) * 15000.0);
  const double globalOscResonance = 0.04 + (oscFilterResNorm * 0.88);
  const double globalOscEnv = 0.10 + (oscFilterEnvNorm * 2.1);

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    LaneFrame frame {};
    frame.character = kLaneCharacters[lane];

    const double tune = getParam (laneParamID (lane, kLaneTune));
    const double semitoneRange = kPitchSemitoneRange[lane];
    const double semitones = (tune * 2.0 - 1.0) * semitoneRange;
    frame.frequencyHz = kBaseFrequencies[lane] * std::pow (2.0, semitones / 12.0);
    frame.frequencyHz = std::clamp (frame.frequencyHz, 8.0, 20000.0);

    const double decay = getParam (laneParamID (lane, kLaneDecay));
    frame.decaySeconds = 0.02 + (decay * decay * 1.95);
    const double level = getParam (laneParamID (lane, kLaneLevel));
    frame.outputLevel = std::pow (level, 1.05);
    frame.oscLevel = std::clamp ((0.40 + (std::pow (level, 0.80) * 1.25)) * kOscBalance[lane], 0.0, 2.0);

    frame.foldAmount = getParam (laneParamID (lane, kLaneFold));
    frame.fmAmount = getParam (laneParamID (lane, kLaneFm));
    frame.bodyFilterCutoffHz = std::clamp (globalOscCutoffHz * kOscCutoffScale[lane], 80.0, 18000.0);
    frame.bodyFilterResonance =
      std::clamp ((globalOscResonance + (frame.foldAmount * 0.12)) * kOscResScale[lane], 0.0, 0.98);
    frame.bodyFilterEnvAmount = std::clamp (globalOscEnv * kOscEnvScale[lane], 0.0, 2.5);

    const double noise = getParam (laneParamID (lane, kLaneNoise));
    frame.noiseAmount = std::clamp (std::pow (noise, 0.82) * 1.35, 0.0, 2.5);
    frame.noiseLevel = std::clamp (std::pow (noise, 0.58) * kNoiseLevelScale[lane], 0.0, 2.5);
    frame.pitchEnvAmount = std::clamp (getParam (laneExtraParamID (lane, kLanePitchEnvAmount)) *
                                         kPitchEnvScale[lane],
                                       0.0, 1.0);
    const double pitchDecay = getParam (laneExtraParamID (lane, kLanePitchEnvDecay));
    frame.pitchEnvDecaySeconds = 0.006 + (pitchDecay * pitchDecay * 0.55);
    frame.transientAmount = std::clamp (std::pow (getParam (laneExtraParamID (lane, kLaneTransientAttack)), 0.72) *
                                          kTransientAttackScale[lane],
                                        0.0, 1.0);
    const double transientDecay = getParam (laneMacroParamID (lane, kLaneTransientDecay));
    frame.transientDecaySeconds =
      std::clamp ((0.003 + (transientDecay * transientDecay * 0.46)) * kTransientDecayScale[lane], 0.0015, 0.5);
    const double transientLevel = getParam (laneMacroParamID (lane, kLaneTransientMix));
    frame.transientLevel =
      std::clamp ((0.18 + (std::pow (transientLevel, 0.74) * 1.65)) * kTransientLevelScale[lane], 0.0, 2.5);
    frame.transientMix = std::clamp (0.18 + (transientLevel * 1.05), 0.0, 1.4);
    const double noiseTone = getParam (laneExtraParamID (lane, kLaneNoiseTone));
    frame.noiseTone = (noiseTone * 2.0) - 1.0;
    frame.noiseFilterCutoffHz = 220.0 + (std::pow (noiseTone, 1.40) * 16000.0);
    const double noiseDecay = getParam (laneExtraParamID (lane, kLaneNoiseDecay));
    frame.noiseDecaySeconds = (0.008 + (noiseDecay * noiseDecay * 1.3));
    frame.noiseResonance =
      std::clamp ((0.05 + (getParam (laneMacroParamID (lane, kLaneNoiseResonance)) * 0.90)) * kNoiseResScale[lane],
                  0.0, 0.98);
    frame.noiseEnvAmount =
      std::clamp ((0.18 + (getParam (laneMacroParamID (lane, kLaneNoiseEnvAmount)) * 1.25)) * kNoiseEnvScale[lane],
                  0.0, 1.5);
    frame.snapAmount = std::clamp (getParam (laneExtraParamID (lane, kLaneSnap)) * kSnapScale[lane], 0.0, 1.0);

    frame.driveAmount = getParam (laneParamID (lane, kLaneDrive));
    frame.level = frame.outputLevel;
    frame.pan = (getParam (laneParamID (lane, kLanePan)) * 2.0) - 1.0;

    frame.oscFilterCutoff = getParam (laneFilterParamID (lane, kLaneOscFilterCutoff));
    frame.oscFilterResonance = std::clamp (getParam (laneFilterParamID (lane, kLaneOscFilterRes)), 0.0, 1.0);
    frame.oscFilterEnvAmount = getParam (laneFilterParamID (lane, kLaneOscFilterEnv));
    frame.transFilterCutoff = getParam (laneFilterParamID (lane, kLaneTransFilterCutoff));
    frame.transFilterResonance = std::clamp (getParam (laneFilterParamID (lane, kLaneTransFilterRes)), 0.0, 1.0);
    frame.transFilterEnvAmount = getParam (laneFilterParamID (lane, kLaneTransFilterEnv));

    laneFrames_[lane] = frame;
  }
}

void WestCoastProcessor::pushParamChange (Vst::IParameterChanges* outputChanges, Vst::ParamID id,
                                          double normalizedValue) const
{
  if (!outputChanges)
    return;
  int32 queueIndex = 0;
  Vst::IParamValueQueue* queue = outputChanges->addParameterData (id, queueIndex);
  if (!queue)
    return;
  int32 pointIndex = 0;
  queue->addPoint (0, clamp01 (normalizedValue), pointIndex);
}

double WestCoastProcessor::getParam (Vst::ParamID id) const
{
  if (id >= static_cast<Vst::ParamID> (kParameterStateSize))
    return 0.0;
  return params_[id];
}

void WestCoastProcessor::setParam (Vst::ParamID id, double normalizedValue)
{
  if (id >= static_cast<Vst::ParamID> (kParameterStateSize))
    return;
  params_[id] = clamp01 (normalizedValue);
}

} // namespace Steinberg::WestCoastDrumSynth
