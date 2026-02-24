#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg::WestCoastDrumSynth {

class WestCoastController final : public Vst::EditControllerEx1 {
public:
  WestCoastController () = default;
  ~WestCoastController () SMTG_OVERRIDE = default;

  static FUnknown* createInstance (void*);

  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;
  IPlugView* PLUGIN_API createView (FIDString name) SMTG_OVERRIDE;
};

} // namespace Steinberg::WestCoastDrumSynth
