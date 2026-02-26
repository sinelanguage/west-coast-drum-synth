#include "WCDrumView.h"
#include <cmath>
#include <algorithm>

namespace SineLanguage {
namespace WestCoastDrumSynth {

using namespace VSTGUI;

WCDrumView::WCDrumView(const CRect& size)
    : CViewContainer(size) {
    setBackgroundColor(CColor(0, 0, 0, 0));
}

void WCDrumView::drawScrewHead(CDrawContext* ctx, CCoord x, CCoord y, CCoord radius) {
    // Shadow under screw
    CGraphicsPath* shadowPath = ctx->createGraphicsPath();
    if (shadowPath) {
        shadowPath->addEllipse(CRect(x - radius - 1, y - radius + 1, x + radius + 1, y + radius + 3));
        ctx->setFillColor(CColor(0, 0, 0, 40));
        ctx->drawGraphicsPath(shadowPath, CDrawContext::kPathFilled);
        shadowPath->forget();
    }

    // Screw recess
    CGraphicsPath* recessPath = ctx->createGraphicsPath();
    if (recessPath) {
        recessPath->addEllipse(CRect(x - radius - 1, y - radius - 1, x + radius + 1, y + radius + 1));
        ctx->setFillColor(CColor(30, 30, 33, 255));
        ctx->drawGraphicsPath(recessPath, CDrawContext::kPathFilled);
        recessPath->forget();
    }

    // Screw body with metal gradient
    CGraphicsPath* bodyPath = ctx->createGraphicsPath();
    if (bodyPath) {
        bodyPath->addEllipse(CRect(x - radius, y - radius, x + radius, y + radius));
        CGradient* grad = CGradient::create(0.0, 1.0,
            CColor(170, 170, 175, 255), CColor(100, 100, 108, 255));
        if (grad) {
            ctx->fillLinearGradient(bodyPath, *grad,
                CPoint(x - radius, y - radius), CPoint(x + radius, y + radius));
            grad->forget();
        }
        bodyPath->forget();
    }

    // Phillips cross
    ctx->setLineWidth(1.2);
    ctx->setFrameColor(CColor(60, 60, 65, 180));
    ctx->drawLine(CPoint(x - radius * 0.5, y), CPoint(x + radius * 0.5, y));
    ctx->drawLine(CPoint(x, y - radius * 0.5), CPoint(x, y + radius * 0.5));

    // Specular highlight
    CGraphicsPath* specPath = ctx->createGraphicsPath();
    if (specPath) {
        CCoord sr = radius * 0.5;
        specPath->addEllipse(CRect(x - sr, y - radius * 0.7, x + sr, y - radius * 0.1));
        ctx->setFillColor(CColor(255, 255, 255, 25));
        ctx->drawGraphicsPath(specPath, CDrawContext::kPathFilled);
        specPath->forget();
    }
}

void WCDrumView::drawBrushedMetal(CDrawContext* ctx, const CRect& r) {
    ctx->setLineWidth(0.3);
    for (CCoord y = r.top + 1; y < r.bottom - 1; y += 1.5) {
        int hash = static_cast<int>(y * 17.31) & 0xFF;
        uint8_t base = 42 + (hash % 12);
        ctx->setFrameColor(CColor(base, base, base + 2, 18));
        ctx->drawLine(CPoint(r.left + 2, y), CPoint(r.right - 2, y));
    }
}

void WCDrumView::drawVentSlots(CDrawContext* ctx, const CRect& r) {
    CCoord slotWidth = 18;
    CCoord slotHeight = 3;
    CCoord spacing = 5;
    int numSlots = 6;

    CCoord startX = r.right - 45;
    CCoord startY = r.top + 12;

    ctx->setLineWidth(0.5);
    for (int i = 0; i < numSlots; ++i) {
        CCoord y = startY + i * spacing;
        CRect slot(startX, y, startX + slotWidth, y + slotHeight);

        CGraphicsPath* slotPath = ctx->createGraphicsPath();
        if (slotPath) {
            slotPath->addRoundRect(slot, 1.0);
            ctx->setFillColor(CColor(20, 20, 22, 200));
            ctx->drawGraphicsPath(slotPath, CDrawContext::kPathFilled);

            ctx->setFrameColor(CColor(55, 55, 60, 150));
            ctx->drawGraphicsPath(slotPath, CDrawContext::kPathStroked);
            slotPath->forget();
        }
    }
}

void WCDrumView::drawSectionPanel(CDrawContext* ctx, const CRect& r, const char* title) {
    // Outer shadow
    CGraphicsPath* shadowPath = ctx->createGraphicsPath();
    if (shadowPath) {
        CRect sr = r;
        sr.offset(1, 2);
        shadowPath->addRoundRect(sr, 6.0);
        ctx->setFillColor(CColor(0, 0, 0, 30));
        ctx->drawGraphicsPath(shadowPath, CDrawContext::kPathFilled);
        shadowPath->forget();
    }

    // Panel body
    CGraphicsPath* panelPath = ctx->createGraphicsPath();
    if (panelPath) {
        panelPath->addRoundRect(r, 5.0);

        CGradient* grad = CGradient::create(0.0, 1.0,
            CColor(32, 32, 37, 255), CColor(24, 24, 28, 255));
        if (grad) {
            ctx->fillLinearGradient(panelPath, *grad,
                CPoint(r.left, r.top), CPoint(r.left, r.bottom));
            grad->forget();
        }

        // Inner bevel highlight (top)
        ctx->setFrameColor(CColor(50, 50, 55, 100));
        ctx->setLineWidth(0.5);
        ctx->drawGraphicsPath(panelPath, CDrawContext::kPathStroked);

        // Outer border
        ctx->setFrameColor(CColor(15, 15, 18, 200));
        ctx->setLineWidth(1.5);
        ctx->drawGraphicsPath(panelPath, CDrawContext::kPathStroked);

        panelPath->forget();
    }

    if (title && title[0]) {
        ctx->setFontColor(CColor(200, 160, 80, 200));
        ctx->setFont(kNormalFontSmaller);
        CRect titleR(r.left + 12, r.top + 5, r.right - 12, r.top + 19);
        ctx->drawString(title, titleR, kLeftText);

        // Title underline
        ctx->setFrameColor(CColor(200, 160, 80, 40));
        ctx->setLineWidth(0.5);
        ctx->drawLine(CPoint(r.left + 12, r.top + 20), CPoint(r.left + 200, r.top + 20));
    }
}

void WCDrumView::drawLogo(CDrawContext* ctx, const CRect& r) {
    // Gold logo text
    ctx->setFontColor(CColor(220, 180, 100, 255));
    ctx->setFont(kNormalFontBig);
    CRect logoR(r.left + 22, r.top + 10, r.left + 450, r.top + 34);
    ctx->drawString("WEST COAST DRUM SYNTH", logoR, kLeftText);

    // Subtitle
    ctx->setFontColor(CColor(140, 140, 145, 200));
    ctx->setFont(kNormalFontVerySmall);
    CRect subR(r.left + 22, r.top + 33, r.left + 500, r.top + 45);
    ctx->drawString("SINE LANGUAGE  |  BUCHLA-INSPIRED PERCUSSION SYNTHESIS  |  6 VOICES  |  16 STEPS", subR, kLeftText);

    // Decorative gold line
    ctx->setFrameColor(CColor(200, 150, 60, 100));
    ctx->setLineWidth(1.0);
    ctx->drawLine(CPoint(r.left + 22, r.top + 48), CPoint(r.right - 22, r.top + 48));

    // Thin white highlight above line
    ctx->setFrameColor(CColor(255, 255, 255, 20));
    ctx->setLineWidth(0.5);
    ctx->drawLine(CPoint(r.left + 22, r.top + 47), CPoint(r.right - 22, r.top + 47));
}

void WCDrumView::drawPanelBackground(CDrawContext* ctx, const CRect& r) {
    // Main charcoal panel
    CGraphicsPath* bgPath = ctx->createGraphicsPath();
    if (bgPath) {
        bgPath->addRoundRect(r, 8.0);

        CGradient* grad = CGradient::create(0.0, 1.0,
            CColor(54, 54, 60, 255), CColor(40, 40, 45, 255));
        if (grad) {
            ctx->fillLinearGradient(bgPath, *grad,
                CPoint(r.left, r.top), CPoint(r.left, r.bottom));
            grad->forget();
        }

        // Outer bezel shadow
        ctx->setFrameColor(CColor(20, 20, 23, 255));
        ctx->setLineWidth(3.0);
        ctx->drawGraphicsPath(bgPath, CDrawContext::kPathStroked);

        // Inner highlight
        CRect inner = r;
        inner.inset(3, 3);
        CGraphicsPath* innerPath = ctx->createGraphicsPath();
        if (innerPath) {
            innerPath->addRoundRect(inner, 6.0);
            ctx->setFrameColor(CColor(65, 65, 70, 80));
            ctx->setLineWidth(0.5);
            ctx->drawGraphicsPath(innerPath, CDrawContext::kPathStroked);
            innerPath->forget();
        }

        bgPath->forget();
    }

    drawBrushedMetal(ctx, r);
}

void WCDrumView::drawBackgroundRect(CDrawContext* ctx, const CRect& rect) {
    CRect fullRect = getViewSize();

    // Overall dark background behind everything
    ctx->setFillColor(CColor(18, 18, 20, 255));
    ctx->drawRect(fullRect, kDrawFilled);

    drawPanelBackground(ctx, fullRect);
    drawLogo(ctx, fullRect);

    // Corner screws
    CCoord si = 14;
    CCoord sr = 4.5;
    drawScrewHead(ctx, fullRect.left + si, fullRect.top + si, sr);
    drawScrewHead(ctx, fullRect.right - si, fullRect.top + si, sr);
    drawScrewHead(ctx, fullRect.left + si, fullRect.bottom - si, sr);
    drawScrewHead(ctx, fullRect.right - si, fullRect.bottom - si, sr);

    // Vent slots in header area
    drawVentSlots(ctx, fullRect);

    CViewContainer::drawBackgroundRect(ctx, rect);
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
