#include "WestCoastProcessor.h"

#include "presets/FactoryPresets.h"
#include "westcoastdrumcids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace Steinberg::WestCoastDrumSynth {

namespace {

constexpr uint32 kStateVersion = 1;

inline double clamp01 (double x)
{
  return std::clamp (x, 0.0, 1.0);
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

inline int32 laneForMidiPitch (int16 pitch)
{
  switch (pitch)
  {
    case 36:
    case 35:
      return 0; // Kick
    case 38:
    case 40:
      return 1; // Snare
    case 42:
    case 44:
    case 46:
      return 2; // Hat
    case 48:
    case 50:
      return 3; // Perc
    default:
      return -1;
  }
}

} // namespace

WestCoastProcessor::WestCoastProcessor ()
{
  setControllerClass (kControllerUID);
  params_.fill (0.0);
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
  setParam (kParamRun, 1.0);
  setParam (kParamFollowTransport, 1.0);
  setParam (kParamPresetSelect, 0.0);

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
  if (version != kStateVersion)
    return kResultFalse;

  int32 savedPreset = 0;
  if (!streamer.readInt32 (savedPreset))
    return kResultFalse;
  loadedPreset_ = std::clamp (savedPreset, 0, kFactoryPresetCount - 1);

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
        voices_[lane].trigger (laneFrames_[lane]);
    }
  }

  if (data.numOutputs == 0 || data.outputs == nullptr || data.numSamples <= 0)
    return kResultOk;

  const double master = std::pow (clamp01 (getParam (kParamMaster)), 1.35) * 1.35;
  std::array<bool, kLaneCount> triggers {};

  auto render = [&] (auto** outChannels)
  {
    using SampleType = std::remove_pointer_t<decltype (outChannels[0])>;

    auto* left = outChannels[0];
    auto* right = (data.outputs[0].numChannels > 1) ? outChannels[1] : outChannels[0];

    for (int32 sampleIndex = 0; sampleIndex < data.numSamples; ++sampleIndex)
    {
      sequencer_.processSample (triggers);
      for (int32 lane = 0; lane < kLaneCount; ++lane)
      {
        if (triggers[lane])
          voices_[lane].trigger (laneFrames_[lane]);
      }

      double frameL = 0.0;
      double frameR = 0.0;
      for (int32 lane = 0; lane < kLaneCount; ++lane)
      {
        const double sample = voices_[lane].process ();
        const double pan = std::clamp (laneFrames_[lane].pan, -1.0, 1.0);
        const double gainL = std::sqrt (0.5 * (1.0 - pan));
        const double gainR = std::sqrt (0.5 * (1.0 + pan));
        frameL += sample * gainL;
        frameR += sample * gainR;
      }

      left[sampleIndex] = static_cast<SampleType> (frameL * master);
      right[sampleIndex] = static_cast<SampleType> (frameR * master);
    }
  };

  if (data.symbolicSampleSize == Vst::kSample32)
    render (data.outputs[0].channelBuffers32);
  else if (data.symbolicSampleSize == Vst::kSample64)
    render (data.outputs[0].channelBuffers64);
  else
    return kResultFalse;

  data.outputs[0].silenceFlags = 0;
  return kResultOk;
}

void WestCoastProcessor::resetEngine ()
{
  sequencer_.reset ();
  for (auto& voice : voices_)
    voice.reset ();
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
    setParam (laneParamID (lane, kLaneTune), preset.lanes[lane].tune);
    setParam (laneParamID (lane, kLaneDecay), preset.lanes[lane].decay);
    setParam (laneParamID (lane, kLaneFold), preset.lanes[lane].fold);
    setParam (laneParamID (lane, kLaneFm), preset.lanes[lane].fm);
    setParam (laneParamID (lane, kLaneNoise), preset.lanes[lane].noise);
    setParam (laneParamID (lane, kLaneDrive), preset.lanes[lane].drive);
    setParam (laneParamID (lane, kLaneLevel), preset.lanes[lane].level);
    setParam (laneParamID (lane, kLanePan), preset.lanes[lane].pan);

    pushParamChange (outputChanges, laneParamID (lane, kLaneTune), preset.lanes[lane].tune);
    pushParamChange (outputChanges, laneParamID (lane, kLaneDecay), preset.lanes[lane].decay);
    pushParamChange (outputChanges, laneParamID (lane, kLaneFold), preset.lanes[lane].fold);
    pushParamChange (outputChanges, laneParamID (lane, kLaneFm), preset.lanes[lane].fm);
    pushParamChange (outputChanges, laneParamID (lane, kLaneNoise), preset.lanes[lane].noise);
    pushParamChange (outputChanges, laneParamID (lane, kLaneDrive), preset.lanes[lane].drive);
    pushParamChange (outputChanges, laneParamID (lane, kLaneLevel), preset.lanes[lane].level);
    pushParamChange (outputChanges, laneParamID (lane, kLanePan), preset.lanes[lane].pan);
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

    setParam (queue->getParameterId (), value);

    if (queue->getParameterId () == kParamPresetSelect)
    {
      loadedPreset_ = presetIndexFromNormalized (value);
      presetPending_ = true;
    }
  }

  updateLaneFramesFromParameters ();

  // If transport follow is turned off while host is stopped, force-run behavior immediately.
  if (presetPending_ && outputChanges)
    pushParamChange (outputChanges, kParamPresetSelect, getParam (kParamPresetSelect));
}

void WestCoastProcessor::updateLaneFramesFromParameters ()
{
  static constexpr std::array<double, kLaneCount> kBaseFrequencies {52.0, 185.0, 3800.0, 420.0};

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    LaneFrame frame {};

    const double tune = getParam (laneParamID (lane, kLaneTune));
    const double semitones = (tune * 2.0 - 1.0) * 24.0;
    frame.frequencyHz = kBaseFrequencies[lane] * std::pow (2.0, semitones / 12.0);
    frame.frequencyHz = std::clamp (frame.frequencyHz, 20.0, 12000.0);

    const double decay = getParam (laneParamID (lane, kLaneDecay));
    frame.decaySeconds = 0.02 + (decay * decay * 1.8);

    frame.foldAmount = getParam (laneParamID (lane, kLaneFold));
    frame.fmAmount = getParam (laneParamID (lane, kLaneFm));
    frame.noiseAmount = getParam (laneParamID (lane, kLaneNoise));
    frame.driveAmount = getParam (laneParamID (lane, kLaneDrive));
    frame.level = std::pow (getParam (laneParamID (lane, kLaneLevel)), 1.15);
    frame.pan = (getParam (laneParamID (lane, kLanePan)) * 2.0) - 1.0;

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
