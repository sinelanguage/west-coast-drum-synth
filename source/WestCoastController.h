#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"

namespace Steinberg::WestCoastDrumSynth {

class WestCoastController final : public Vst::EditControllerEx1, public VSTGUI::VST3EditorDelegate {
public:
  WestCoastController () = default;
  ~WestCoastController () SMTG_OVERRIDE = default;

  static FUnknown* createInstance (void*);

  tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate () SMTG_OVERRIDE;
  tresult PLUGIN_API setComponentState (IBStream* state) SMTG_OVERRIDE;
  IPlugView* PLUGIN_API createView (FIDString name) SMTG_OVERRIDE;

  VSTGUI::CView* verifyView (VSTGUI::CView* view, const VSTGUI::UIAttributes& attributes,
                             const VSTGUI::IUIDescription* description,
                             VSTGUI::VST3Editor* editor) SMTG_OVERRIDE;
  bool isPrivateParameter (const Vst::ParamID paramID) SMTG_OVERRIDE;
};

} // namespace Steinberg::WestCoastDrumSynth
