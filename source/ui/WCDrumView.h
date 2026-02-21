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

// Main view container with photorealistic Arturia-style rendering
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
    void drawVentSlots(CDrawContext* ctx, const CRect& r);
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
