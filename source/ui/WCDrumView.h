#pragma once

#include "vstgui/lib/cview.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/cslider.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cbitmap.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/uidescription/icontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "wcparams.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

using namespace VSTGUI;

// Photorealistic metal knob inspired by Arturia hardware
class WCMetalKnob : public CKnob {
public:
    WCMetalKnob(const CRect& size, IControlListener* listener,
                int32_t tag, float defaultValue = 0.5f);

    void draw(CDrawContext* context) override;

    CLASS_METHODS(WCMetalKnob, CKnob)

private:
    void drawMetalBody(CDrawContext* ctx, const CRect& r);
    void drawIndicator(CDrawContext* ctx, const CRect& r);
    void drawShadow(CDrawContext* ctx, const CRect& r);
    void drawHighlight(CDrawContext* ctx, const CRect& r);

    float defaultValue_;
};

// Illuminated step button for sequencer
class WCStepButton : public COnOffButton {
public:
    WCStepButton(const CRect& size, IControlListener* listener,
                 int32_t tag, bool isActive = false);

    void draw(CDrawContext* context) override;
    void setStepActive(bool active) { stepActive_ = active; }
    void setCurrentStep(bool isCurrent) { isCurrentStep_ = isCurrent; }

    CLASS_METHODS(WCStepButton, COnOffButton)

private:
    bool stepActive_ = false;
    bool isCurrentStep_ = false;
};

// Voice selector button with LED indicator
class WCVoiceButton : public COnOffButton {
public:
    WCVoiceButton(const CRect& size, IControlListener* listener,
                  int32_t tag, const char* label);

    void draw(CDrawContext* context) override;
    void setSelected(bool sel) { selected_ = sel; }

    CLASS_METHODS(WCVoiceButton, COnOffButton)

private:
    std::string label_;
    bool selected_ = false;
};

// Main view container assembling the full interface
class WCDrumView : public CViewContainer {
public:
    WCDrumView(const CRect& size);

    void drawBackgroundRect(CDrawContext* ctx, const CRect& rect) override;

    CLASS_METHODS(WCDrumView, CViewContainer)

private:
    void drawPanelBackground(CDrawContext* ctx, const CRect& r);
    void drawBrushedMetal(CDrawContext* ctx, const CRect& r);
    void drawSectionPanel(CDrawContext* ctx, const CRect& r, const char* title);
    void drawLogo(CDrawContext* ctx, const CRect& r);
    void drawScrewHead(CDrawContext* ctx, CCoord x, CCoord y, CCoord radius);
};


} // namespace WestCoastDrumSynth
} // namespace SineLanguage
