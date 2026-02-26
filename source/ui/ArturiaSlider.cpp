#include "ArturiaSlider.h"
#include <cmath>

namespace SineLanguage {
namespace WestCoastDrumSynth {

using namespace VSTGUI;

namespace {

constexpr CCoord kHandleSizeH = 10.0;
constexpr CCoord kHandleSizeV = 12.0;

} // namespace

ArturiaSlider::ArturiaSlider(const CRect& size, IControlListener* listener, int32_t tag,
                             int32_t iMinPos, int32_t iMaxPos, CBitmap* handle,
                             CBitmap* background, const CPoint& offset,
                             CSlider::Styles style)
    : CSlider(size, listener, tag, iMinPos, iMaxPos, handle, background, offset, style) {
    CCoord w = size.getWidth() > 1 ? size.getWidth() - 2 : kHandleSizeH;
    CCoord h = size.getHeight() > 1 ? size.getHeight() - 2 : kHandleSizeV;
    if (style & kHorizontal)
        setHandleSizePrivate(kHandleSizeH, h);
    else
        setHandleSizePrivate(w, kHandleSizeV);
}

CRect ArturiaSlider::getHandleRect() const {
    float norm = getValueNormalized();
    if (isStyleRight() || isStyleBottom())
        norm = 1.f - norm;

    CRect r = getViewSize();
    CCoord hw = getHandleSizePrivate().x;
    CCoord hh = getHandleSizePrivate().y;
    CCoord range = (isStyleHorizontal() ? r.getWidth() : r.getHeight()) - (isStyleHorizontal() ? hw : hh);

    CRect handle;
    if (isStyleHorizontal()) {
        handle.top = r.top + (r.getHeight() - hh) * 0.5;
        handle.bottom = handle.top + hh;
        handle.left = r.left + norm * range;
        handle.right = handle.left + hw;
    } else {
        handle.left = r.left + (r.getWidth() - hw) * 0.5;
        handle.right = handle.left + hw;
        handle.top = r.top + norm * range;
        handle.bottom = handle.top + hh;
    }
    return handle;
}

void ArturiaSlider::draw(CDrawContext* ctx) {
    CRect trackRect = getViewSize();

    ctx->setDrawMode(kAntiAliasing);
    ctx->setLineStyle(kLineSolid);

    // Track: recessed groove with shadow
    CGraphicsPath* trackPath = ctx->createGraphicsPath();
    if (trackPath) {
        CRect groove = trackRect;
        groove.inset(1, 1);
        trackPath->addRoundRect(groove, 2.0);

        // Track fill - dark gradient (recessed look)
        CGradient* trackGrad = CGradient::create(0.0, 1.0,
            CColor(28, 30, 34, 255), CColor(22, 24, 28, 255));
        if (trackGrad) {
            ctx->fillLinearGradient(trackPath, *trackGrad,
                CPoint(groove.left, groove.top), CPoint(groove.right, groove.bottom));
            trackGrad->forget();
        }

        // Inner shadow (top edge)
        ctx->setFrameColor(CColor(0, 0, 0, 60));
        ctx->setLineWidth(0.5);
        ctx->drawGraphicsPath(trackPath, CDrawContext::kPathStroked);

        // Frame
        ctx->setFrameColor(CColor(45, 48, 54, 255));
        ctx->setLineWidth(1.0);
        ctx->drawGraphicsPath(trackPath, CDrawContext::kPathStroked);
        trackPath->forget();
    }

    // Value fill
    CColor valueColor = getValueColor();
    float drawVal = getValueNormalized();
    if (getDrawStyle() & kDrawValueFromCenter) {
        if (getDrawStyle() & kDrawInverted)
            drawVal = 1.f - drawVal;
        drawVal = 0.5f + (drawVal - 0.5f) * 2.f;
    }
    if (getDrawStyle() & kDrawInverted && !(getDrawStyle() & kDrawValueFromCenter))
        drawVal = 1.f - drawVal;

    CRect fillRect = trackRect;
    fillRect.inset(2, 2);
    if (isStyleHorizontal()) {
        CCoord w = fillRect.getWidth() * drawVal;
        if (isStyleRight())
            fillRect.left = fillRect.right - w;
        else
            fillRect.right = fillRect.left + w;
    } else {
        CCoord h = fillRect.getHeight() * drawVal;
        if (isStyleBottom())
            fillRect.top = fillRect.bottom - h;
        else
            fillRect.bottom = fillRect.top + h;
    }

    if (fillRect.getWidth() > 0.5 && fillRect.getHeight() > 0.5) {
        CGraphicsPath* fillPath = ctx->createGraphicsPath();
        if (fillPath) {
            fillPath->addRoundRect(fillRect, 1.5);
            CGradient* fillGrad = CGradient::create(0.0, 1.0,
                valueColor, CColor(
                    (int)(valueColor.red * 0.7),
                    (int)(valueColor.green * 0.7),
                    (int)(valueColor.blue * 0.7),
                    valueColor.alpha));
            if (fillGrad) {
                ctx->fillLinearGradient(fillPath, *fillGrad,
                    CPoint(fillRect.left, fillRect.top), CPoint(fillRect.right, fillRect.bottom));
                fillGrad->forget();
            }
            fillPath->forget();
        }
    }

    // 3D handle
    CRect handleRect = getHandleRect();
    CGraphicsPath* handlePath = ctx->createGraphicsPath();
    if (handlePath) {
        handlePath->addRoundRect(handleRect, 2.0);

        // Handle shadow
        CRect shadowRect = handleRect;
        shadowRect.offset(0.5, 1.0);
        CGraphicsPath* shadowPath = ctx->createGraphicsPath();
        if (shadowPath) {
            shadowPath->addRoundRect(shadowRect, 2.0);
            ctx->setFillColor(CColor(0, 0, 0, 50));
            ctx->drawGraphicsPath(shadowPath, CDrawContext::kPathFilled);
            shadowPath->forget();
        }

        // Handle body gradient (top highlight, bottom shadow)
        CGradient* handleGrad = CGradient::create(0.0, 1.0,
            CColor(95, 100, 110, 255), CColor(55, 58, 65, 255));
        if (handleGrad) {
            ctx->fillLinearGradient(handlePath, *handleGrad,
                CPoint(handleRect.left, handleRect.top), CPoint(handleRect.right, handleRect.bottom));
            handleGrad->forget();
        }

        // Handle highlight (top edge)
        ctx->setFrameColor(CColor(130, 135, 145, 180));
        ctx->setLineWidth(0.5);
        ctx->drawGraphicsPath(handlePath, CDrawContext::kPathStroked);

        // Handle frame
        ctx->setFrameColor(CColor(40, 42, 48, 255));
        ctx->setLineWidth(1.0);
        ctx->drawGraphicsPath(handlePath, CDrawContext::kPathStroked);
        handlePath->forget();
    }
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
