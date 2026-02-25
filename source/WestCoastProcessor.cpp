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
    case 52:
      return 3; // Perc A
    case 47:
    case 49:
    case 51:
    case 53:
      return 4; // Perc B
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

  if (version == kLegacyStateVersion)
  {
    for (int32 param = 0; param < kParamGlobalCount; ++param)
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

    // Legacy sessions had one percussion lane. Seed Perc B from Perc A.
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
      {
        setParam (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (param)),
                  kLaneExtraDefaults[lane][param]);
      }
    }
    applyMacroDefaults ();

    const auto& preset = getFactoryPresets ()[loadedPreset_];
    sequencer_.setPattern (preset.pattern);
    updateLaneFramesFromParameters ();
    presetPending_ = false;
    return kResultOk;
  }

  if (version == kPreviousStateVersion)
  {
    for (int32 param = 0; param < kParamGlobalCount; ++param)
    {
      double normalized = 0.0;
      if (!streamer.readDouble (normalized))
        return kResultFalse;
      setParam (static_cast<Vst::ParamID> (param), normalized);
    }

    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 param = 0; param < kLaneParamCount; ++param)
      {
        double normalized = 0.0;
        if (!streamer.readDouble (normalized))
          return kResultFalse;
        setParam (laneParamID (lane, static_cast<LaneParamOffset> (param)), normalized);
      }
    }

    for (int32 lane = 0; lane < kLaneCount; ++lane)
    {
      for (int32 param = 0; param < kLaneExtraParamCount; ++param)
      {
        double normalized = 0.0;
        if (!streamer.readDouble (normalized))
          return kResultFalse;
        setParam (laneExtraParamID (lane, static_cast<LaneExtraParamOffset> (param)), normalized);
      }
    }

    applyMacroDefaults ();
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
    using SampleType = std::remove_pointer_t<std::decay_t<decltype (outChannels[0])>>;

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
  static constexpr std::array<double, kLaneCount> kBaseFrequencies {52.0, 185.0, 3800.0, 420.0, 620.0};
  static constexpr std::array<LaneCharacter, kLaneCount> kLaneCharacters {
    LaneCharacter::Kick, LaneCharacter::Snare, LaneCharacter::Hat, LaneCharacter::PercA, LaneCharacter::PercB};
  static constexpr std::array<double, kLaneCount> kPitchEnvScale {1.0, 0.58, 0.24, 0.64, 0.70};
  static constexpr std::array<double, kLaneCount> kTransientScale {1.0, 0.96, 0.62, 0.86, 0.92};
  static constexpr std::array<double, kLaneCount> kTransientDecayScale {1.0, 1.22, 0.62, 0.94, 1.02};
  static constexpr std::array<double, kLaneCount> kTransientMixScale {1.0, 1.10, 0.82, 0.96, 1.0};
  static constexpr std::array<double, kLaneCount> kNoiseScale {0.74, 1.85, 1.28, 1.14, 1.24};
  static constexpr std::array<double, kLaneCount> kNoiseDecayScale {0.75, 1.70, 0.90, 1.18, 1.26};
  static constexpr std::array<double, kLaneCount> kNoiseResScale {0.86, 1.00, 1.18, 0.94, 1.02};
  static constexpr std::array<double, kLaneCount> kNoiseEnvScale {0.82, 1.00, 1.26, 0.94, 1.06};
  static constexpr std::array<double, kLaneCount> kSnapScale {0.24, 1.0, 0.86, 0.58, 0.64};

  for (int32 lane = 0; lane < kLaneCount; ++lane)
  {
    LaneFrame frame {};
    frame.character = kLaneCharacters[lane];

    const double tune = getParam (laneParamID (lane, kLaneTune));
    const double semitones = (tune * 2.0 - 1.0) * 24.0;
    frame.frequencyHz = kBaseFrequencies[lane] * std::pow (2.0, semitones / 12.0);
    frame.frequencyHz = std::clamp (frame.frequencyHz, 20.0, 12000.0);

    const double decay = getParam (laneParamID (lane, kLaneDecay));
    frame.decaySeconds = 0.02 + (decay * decay * 1.8);

    frame.foldAmount = getParam (laneParamID (lane, kLaneFold));
    frame.fmAmount = getParam (laneParamID (lane, kLaneFm));
    frame.noiseAmount = std::pow (getParam (laneParamID (lane, kLaneNoise)), 0.82) * kNoiseScale[lane];
    frame.pitchEnvAmount = std::clamp (getParam (laneExtraParamID (lane, kLanePitchEnvAmount)) *
                                         kPitchEnvScale[lane],
                                       0.0, 1.0);
    const double pitchDecay = getParam (laneExtraParamID (lane, kLanePitchEnvDecay));
    frame.pitchEnvDecaySeconds = 0.006 + (pitchDecay * pitchDecay * 0.55);
    frame.transientAmount = std::clamp (std::pow (getParam (laneExtraParamID (lane, kLaneTransientAttack)), 0.72) *
                                          kTransientScale[lane],
                                        0.0, 1.0);
    const double transientDecay = getParam (laneMacroParamID (lane, kLaneTransientDecay));
    frame.transientDecaySeconds =
      std::clamp ((0.002 + (transientDecay * transientDecay * 0.40)) * kTransientDecayScale[lane], 0.0015, 0.5);
    const double transientMix = getParam (laneMacroParamID (lane, kLaneTransientMix));
    frame.transientMix =
      std::clamp (std::pow (transientMix, 0.82) * 1.25 * kTransientMixScale[lane], 0.0, 1.4);
    frame.noiseTone = (getParam (laneExtraParamID (lane, kLaneNoiseTone)) * 2.0) - 1.0;
    const double noiseDecay = getParam (laneExtraParamID (lane, kLaneNoiseDecay));
    frame.noiseDecaySeconds = (0.008 + (noiseDecay * noiseDecay * 1.3)) * kNoiseDecayScale[lane];
    frame.noiseResonance =
      std::clamp ((0.08 + (getParam (laneMacroParamID (lane, kLaneNoiseResonance)) * 0.86)) * kNoiseResScale[lane],
                  0.0, 0.98);
    frame.noiseEnvAmount =
      std::clamp ((0.20 + (getParam (laneMacroParamID (lane, kLaneNoiseEnvAmount)) * 1.20)) * kNoiseEnvScale[lane],
                  0.0, 1.5);
    frame.snapAmount = std::clamp (getParam (laneExtraParamID (lane, kLaneSnap)) * kSnapScale[lane], 0.0, 1.0);
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
