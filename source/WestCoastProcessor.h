#pragma once

#include "ParameterIds.h"
#include "engine/DrumVoice.h"
#include "engine/StepSequencer.h"

#include "public.sdk/source/vst/vstaudioeffect.h"

#include <array>

namespace Steinberg::WestCoastDrumSynth {

class WestCoastProcessor final : public Vst::AudioEffect {
public:
  WestCoastProcessor ();
  ~WestCoastProcessor () SMTG_OVERRIDE = default;

  static FUnknown* createInstance (void*);

  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API setBusArrangements (Vst::SpeakerArrangement* inputs, int32 numIns,
                                         Vst::SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;
  tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& setup) SMTG_OVERRIDE;
  tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE;

private:
  void resetEngine ();
  void loadPresetByIndex (int32 presetIndex, Vst::IParameterChanges* outputChanges);
  void processParameterChanges (Vst::IParameterChanges* changes, Vst::IParameterChanges* outputChanges);
  void updateLaneFramesFromParameters ();
  void pushParamChange (Vst::IParameterChanges* outputChanges, Vst::ParamID id, double normalizedValue) const;
  double getParam (Vst::ParamID id) const;
  void setParam (Vst::ParamID id, double normalizedValue);

  std::array<double, kParameterStateSize> params_ {};
  std::array<LaneFrame, kLaneCount> laneFrames_ {};
  std::array<DrumVoice, kLaneCount> voices_ {};
  std::array<double, kLaneCount> laneLedState_ {};
  StepSequencer sequencer_ {};

  int32 loadedPreset_ {0};
  bool presetPending_ {true};
};

} // namespace Steinberg::WestCoastDrumSynth
