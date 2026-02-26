#pragma once

#include "vstgui/lib/controls/cslider.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cgradient.h"

namespace SineLanguage {
namespace WestCoastDrumSynth {

using namespace VSTGUI;

// 3D-style fader with gradients, shadows, and highlights (Arturia-inspired)
class ArturiaSlider : public CSlider {
public:
    ArturiaSlider(const CRect& size, IControlListener* listener, int32_t tag,
                  int32_t iMinPos, int32_t iMaxPos, CBitmap* handle,
                  CBitmap* background, const CPoint& offset,
                  CSlider::Styles style = {{CSlider::kLeft, CSlider::kHorizontal}});

    void draw(CDrawContext* context) override;

    CLASS_METHODS(ArturiaSlider, CSlider)

private:
    CRect getHandleRect() const;
};

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
