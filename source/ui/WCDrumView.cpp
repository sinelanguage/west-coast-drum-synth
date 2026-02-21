#include "WCDrumView.h"
#include <cmath>
#include <algorithm>

namespace SineLanguage {
namespace WestCoastDrumSynth {

using namespace VSTGUI;

// ============================================================================
// WCMetalKnob - Photorealistic brushed-metal knob
// ============================================================================

WCMetalKnob::WCMetalKnob(const CRect& size, IControlListener* listener,
                           int32_t tag, float defaultValue)
    : CKnob(size, listener, tag, nullptr, nullptr)
    , defaultValue_(defaultValue) {
    setMin(0.0f);
    setMax(1.0f);
    setDefaultValue(defaultValue);
}

void WCMetalKnob::drawShadow(CDrawContext* ctx, const CRect& r) {
    CCoord cx = r.getCenter().x;
    CCoord cy = r.getCenter().y + 2;
    CCoord radius = std::min(r.getWidth(), r.getHeight()) * 0.42;

    CGraphicsPath* path = ctx->createGraphicsPath();
    if (path) {
        path->addEllipse(CRect(cx - radius - 2, cy - radius, cx + radius + 2, cy + radius + 4));
        CColor shadowColor(0, 0, 0, 60);
        ctx->setFillColor(shadowColor);
        ctx->drawGraphicsPath(path, CDrawContext::kPathFilled);
        path->forget();
    }
}

void WCMetalKnob::drawMetalBody(CDrawContext* ctx, const CRect& r) {
    CCoord cx = r.getCenter().x;
    CCoord cy = r.getCenter().y;
    CCoord radius = std::min(r.getWidth(), r.getHeight()) * 0.38;

    // Outer ring (dark bezel)
    CGraphicsPath* outerPath = ctx->createGraphicsPath();
    if (outerPath) {
        outerPath->addEllipse(CRect(cx - radius - 3, cy - radius - 3,
                                     cx + radius + 3, cy + radius + 3));

        CGradient* outerGrad = CGradient::create(0.0, 1.0,
            CColor(80, 80, 85, 255), CColor(35, 35, 40, 255));
        if (outerGrad) {
            ctx->fillLinearGradient(outerPath, *outerGrad,
                CPoint(cx, cy - radius - 3), CPoint(cx, cy + radius + 3));
            outerGrad->forget();
        }
        outerPath->forget();
    }

    // Main metal body
    CGraphicsPath* bodyPath = ctx->createGraphicsPath();
    if (bodyPath) {
        bodyPath->addEllipse(CRect(cx - radius, cy - radius, cx + radius, cy + radius));

        CGradient* bodyGrad = CGradient::create(0.0, 1.0,
            CColor(200, 200, 205, 255), CColor(130, 130, 138, 255));
        if (bodyGrad) {
            ctx->fillLinearGradient(bodyPath, *bodyGrad,
                CPoint(cx - radius, cy - radius), CPoint(cx + radius, cy + radius));
            bodyGrad->forget();
        }
        bodyPath->forget();
    }

    // Brushed metal texture lines
    ctx->setLineWidth(0.5);
    for (int i = 0; i < 24; ++i) {
        float angle = (float)i / 24.0f * 3.14159f * 2.0f;
        float innerR = radius * 0.3f;
        float outerR = radius * 0.85f;
        CCoord x1 = cx + std::cos(angle) * innerR;
        CCoord y1 = cy + std::sin(angle) * innerR;
        CCoord x2 = cx + std::cos(angle) * outerR;
        CCoord y2 = cy + std::sin(angle) * outerR;
        ctx->setFrameColor(CColor(180, 180, 185, 40));
        ctx->drawLine(CPoint(x1, y1), CPoint(x2, y2));
    }

    // Specular highlight
    CGraphicsPath* specPath = ctx->createGraphicsPath();
    if (specPath) {
        CCoord specR = radius * 0.7;
        specPath->addEllipse(CRect(cx - specR, cy - radius * 0.8,
                                    cx + specR, cy - radius * 0.1));

        CGradient* specGrad = CGradient::create(0.0, 1.0,
            CColor(255, 255, 255, 50), CColor(255, 255, 255, 0));
        if (specGrad) {
            ctx->fillLinearGradient(specPath, *specGrad,
                CPoint(cx, cy - radius), CPoint(cx, cy));
            specGrad->forget();
        }
        specPath->forget();
    }
}

void WCMetalKnob::drawIndicator(CDrawContext* ctx, const CRect& r) {
    CCoord cx = r.getCenter().x;
    CCoord cy = r.getCenter().y;
    CCoord radius = std::min(r.getWidth(), r.getHeight()) * 0.38;

    float val = getValueNormalized();
    float angle = (-0.75f + val * 1.5f) * 3.14159f;

    CCoord ix = cx + std::cos(angle) * radius * 0.35f;
    CCoord iy = cy + std::sin(angle) * radius * 0.35f;
    CCoord ox = cx + std::cos(angle) * radius * 0.85f;
    CCoord oy = cy + std::sin(angle) * radius * 0.85f;

    // Indicator line
    ctx->setLineWidth(2.5);
    ctx->setFrameColor(CColor(255, 140, 20, 255));
    ctx->drawLine(CPoint(ix, iy), CPoint(ox, oy));

    // Glowing dot at tip
    CGraphicsPath* dotPath = ctx->createGraphicsPath();
    if (dotPath) {
        dotPath->addEllipse(CRect(ox - 3, oy - 3, ox + 3, oy + 3));
        ctx->setFillColor(CColor(255, 180, 60, 255));
        ctx->drawGraphicsPath(dotPath, CDrawContext::kPathFilled);
        dotPath->forget();
    }
}

void WCMetalKnob::drawHighlight(CDrawContext* ctx, const CRect& r) {
    CCoord cx = r.getCenter().x;
    CCoord cy = r.getCenter().y;
    CCoord radius = std::min(r.getWidth(), r.getHeight()) * 0.38;

    // Value arc
    float val = getValueNormalized();
    float startAngle = -0.75f * 3.14159f;
    float endAngle = (-0.75f + val * 1.5f) * 3.14159f;

    CGraphicsPath* arcPath = ctx->createGraphicsPath();
    if (arcPath) {
        CCoord arcR = radius + 5;
        int segments = static_cast<int>(val * 30) + 2;
        for (int i = 0; i < segments; ++i) {
            float t = static_cast<float>(i) / (segments - 1);
            float a = startAngle + t * (endAngle - startAngle);
            CCoord ax = cx + std::cos(a) * arcR;
            CCoord ay = cy + std::sin(a) * arcR;
            CCoord bx = cx + std::cos(a) * (arcR + 2);
            CCoord by = cy + std::sin(a) * (arcR + 2);

            uint8_t alpha = static_cast<uint8_t>(80 + t * 175);
            ctx->setFrameColor(CColor(255, 140, 20, alpha));
            ctx->setLineWidth(2.0);
            ctx->drawLine(CPoint(ax, ay), CPoint(bx, by));
        }
        arcPath->forget();
    }
}

void WCMetalKnob::draw(CDrawContext* context) {
    CRect r = getViewSize();
    drawShadow(context, r);
    drawMetalBody(context, r);
    drawIndicator(context, r);
    drawHighlight(context, r);
    setDirty(false);
}

// ============================================================================
// WCStepButton - Illuminated sequencer step pad
// ============================================================================

WCStepButton::WCStepButton(const CRect& size, IControlListener* listener,
                             int32_t tag, bool isActive)
    : COnOffButton(size, listener, tag, nullptr)
    , stepActive_(isActive) {
}

void WCStepButton::draw(CDrawContext* context) {
    CRect r = getViewSize();
    bool active = getValue() > 0.5f;

    // Button body
    CColor bgColor;
    if (isCurrentStep_ && active) {
        bgColor = CColor(255, 160, 30, 255);
    } else if (isCurrentStep_) {
        bgColor = CColor(100, 100, 60, 255);
    } else if (active) {
        bgColor = CColor(220, 120, 20, 240);
    } else {
        bgColor = CColor(50, 50, 55, 255);
    }

    // Rounded pad shape
    CGraphicsPath* padPath = context->createGraphicsPath();
    if (padPath) {
        CCoord radius = 3.0;
        padPath->addRoundRect(r, radius);

        CGradient* padGrad = CGradient::create(0.0, 1.0,
            CColor(
                std::min(255, bgColor.red + 30),
                std::min(255, bgColor.green + 30),
                std::min(255, bgColor.blue + 30),
                bgColor.alpha),
            bgColor);
        if (padGrad) {
            context->fillLinearGradient(padPath, *padGrad,
                CPoint(r.left, r.top), CPoint(r.left, r.bottom));
            padGrad->forget();
        }

        // Subtle border
        CColor borderColor = active ?
            CColor(255, 180, 60, 200) :
            CColor(70, 70, 75, 255);
        context->setFrameColor(borderColor);
        context->setLineWidth(1.0);
        context->drawGraphicsPath(padPath, CDrawContext::kPathStroked);

        padPath->forget();
    }

    // Inner glow when active
    if (active) {
        CGraphicsPath* glowPath = context->createGraphicsPath();
        if (glowPath) {
            CRect inner = r;
            inner.inset(3, 3);
            glowPath->addRoundRect(inner, 2.0);

            CGradient* glowGrad = CGradient::create(0.0, 1.0,
                CColor(255, 220, 120, 80), CColor(255, 140, 20, 0));
            if (glowGrad) {
                context->fillLinearGradient(glowPath, *glowGrad,
                    CPoint(inner.left, inner.top), CPoint(inner.left, inner.bottom));
                glowGrad->forget();
            }
            glowPath->forget();
        }
    }

    setDirty(false);
}

// ============================================================================
// WCVoiceButton - Voice channel selector
// ============================================================================

WCVoiceButton::WCVoiceButton(const CRect& size, IControlListener* listener,
                               int32_t tag, const char* label)
    : COnOffButton(size, listener, tag, nullptr)
    , label_(label) {
}

void WCVoiceButton::draw(CDrawContext* context) {
    CRect r = getViewSize();

    CColor bgColor = selected_ ?
        CColor(220, 120, 20, 255) :
        CColor(55, 55, 60, 255);

    CGraphicsPath* btnPath = context->createGraphicsPath();
    if (btnPath) {
        btnPath->addRoundRect(r, 4.0);

        CGradient* btnGrad = CGradient::create(0.0, 1.0,
            CColor(
                std::min(255, bgColor.red + 20),
                std::min(255, bgColor.green + 20),
                std::min(255, bgColor.blue + 20),
                255),
            bgColor);
        if (btnGrad) {
            context->fillLinearGradient(btnPath, *btnGrad,
                CPoint(r.left, r.top), CPoint(r.left, r.bottom));
            btnGrad->forget();
        }

        context->setFrameColor(CColor(80, 80, 85, 255));
        context->setLineWidth(1.0);
        context->drawGraphicsPath(btnPath, CDrawContext::kPathStroked);
        btnPath->forget();
    }

    // LED indicator
    CCoord ledX = r.left + 8;
    CCoord ledY = r.getCenter().y;
    CGraphicsPath* ledPath = context->createGraphicsPath();
    if (ledPath) {
        ledPath->addEllipse(CRect(ledX - 3, ledY - 3, ledX + 3, ledY + 3));
        CColor ledColor = selected_ ?
            CColor(100, 255, 80, 255) :
            CColor(60, 80, 55, 255);
        context->setFillColor(ledColor);
        context->drawGraphicsPath(ledPath, CDrawContext::kPathFilled);
        ledPath->forget();
    }

    // Label text
    context->setFontColor(selected_ ? CColor(255, 255, 255, 255) : CColor(180, 180, 185, 255));
    context->setFont(kNormalFontSmall);
    CRect textR = r;
    textR.left += 16;
    context->drawString(label_.c_str(), textR, kCenterText);

    setDirty(false);
}

// ============================================================================
// WCDrumView - Main photorealistic panel
// ============================================================================

WCDrumView::WCDrumView(const CRect& size)
    : CViewContainer(size) {
    setBackgroundColor(CColor(0, 0, 0, 0));
}

void WCDrumView::drawScrewHead(CDrawContext* ctx, CCoord x, CCoord y, CCoord radius) {
    // Outer screw body
    CGraphicsPath* screwPath = ctx->createGraphicsPath();
    if (screwPath) {
        screwPath->addEllipse(CRect(x - radius, y - radius, x + radius, y + radius));
        CGradient* screwGrad = CGradient::create(0.0, 1.0,
            CColor(160, 160, 165, 255), CColor(90, 90, 95, 255));
        if (screwGrad) {
            ctx->fillLinearGradient(screwPath, *screwGrad,
                CPoint(x - radius, y - radius), CPoint(x + radius, y + radius));
            screwGrad->forget();
        }
        screwPath->forget();
    }

    // Phillips head cross
    ctx->setLineWidth(1.5);
    ctx->setFrameColor(CColor(60, 60, 65, 200));
    ctx->drawLine(CPoint(x - radius * 0.5, y), CPoint(x + radius * 0.5, y));
    ctx->drawLine(CPoint(x, y - radius * 0.5), CPoint(x, y + radius * 0.5));
}

void WCDrumView::drawBrushedMetal(CDrawContext* ctx, const CRect& r) {
    // Subtle horizontal brush lines
    ctx->setLineWidth(0.3);
    for (CCoord y = r.top; y < r.bottom; y += 2) {
        uint8_t variation = static_cast<uint8_t>(
            45 + (static_cast<int>(y * 7.3) % 15));
        ctx->setFrameColor(CColor(variation, variation, variation + 2, 20));
        ctx->drawLine(CPoint(r.left, y), CPoint(r.right, y));
    }
}

void WCDrumView::drawSectionPanel(CDrawContext* ctx, const CRect& r, const char* title) {
    // Recessed panel area
    CGraphicsPath* panelPath = ctx->createGraphicsPath();
    if (panelPath) {
        panelPath->addRoundRect(r, 5.0);

        CGradient* panelGrad = CGradient::create(0.0, 1.0,
            CColor(30, 30, 35, 255), CColor(22, 22, 27, 255));
        if (panelGrad) {
            ctx->fillLinearGradient(panelPath, *panelGrad,
                CPoint(r.left, r.top), CPoint(r.left, r.bottom));
            panelGrad->forget();
        }

        // Inner shadow
        ctx->setFrameColor(CColor(15, 15, 18, 200));
        ctx->setLineWidth(1.5);
        ctx->drawGraphicsPath(panelPath, CDrawContext::kPathStroked);

        panelPath->forget();
    }

    // Section title
    if (title && title[0]) {
        ctx->setFontColor(CColor(200, 160, 80, 255));
        ctx->setFont(kNormalFontSmall);
        CRect titleR(r.left + 10, r.top + 4, r.right - 10, r.top + 18);
        ctx->drawString(title, titleR, kLeftText);
    }
}

void WCDrumView::drawLogo(CDrawContext* ctx, const CRect& r) {
    // Brand text
    ctx->setFontColor(CColor(220, 180, 100, 255));
    ctx->setFont(kNormalFontBig);
    CRect logoR(r.left + 20, r.top + 8, r.left + 350, r.top + 32);
    ctx->drawString("WEST COAST DRUM SYNTH", logoR, kLeftText);

    // Sub brand
    ctx->setFontColor(CColor(140, 140, 145, 255));
    ctx->setFont(kNormalFontSmaller);
    CRect subR(r.left + 20, r.top + 30, r.left + 350, r.top + 44);
    ctx->drawString("SINE LANGUAGE  |  BUCHLA-INSPIRED PERCUSSION SYNTHESIS", subR, kLeftText);

    // Decorative line
    ctx->setFrameColor(CColor(200, 150, 60, 120));
    ctx->setLineWidth(1.0);
    ctx->drawLine(CPoint(r.left + 20, r.top + 48), CPoint(r.right - 20, r.top + 48));
}

void WCDrumView::drawPanelBackground(CDrawContext* ctx, const CRect& r) {
    // Main dark charcoal background
    CGraphicsPath* bgPath = ctx->createGraphicsPath();
    if (bgPath) {
        bgPath->addRoundRect(r, 8.0);

        CGradient* bgGrad = CGradient::create(0.0, 1.0,
            CColor(52, 52, 58, 255), CColor(38, 38, 43, 255));
        if (bgGrad) {
            ctx->fillLinearGradient(bgPath, *bgGrad,
                CPoint(r.left, r.top), CPoint(r.left, r.bottom));
            bgGrad->forget();
        }

        // Outer bezel
        ctx->setFrameColor(CColor(25, 25, 28, 255));
        ctx->setLineWidth(2.0);
        ctx->drawGraphicsPath(bgPath, CDrawContext::kPathStroked);

        bgPath->forget();
    }

    drawBrushedMetal(ctx, r);
}

void WCDrumView::drawBackgroundRect(CDrawContext* ctx, const CRect& rect) {
    CRect fullRect = getViewSize();
    drawPanelBackground(ctx, fullRect);
    drawLogo(ctx, fullRect);

    // Screws in corners
    CCoord screwInset = 12;
    CCoord screwR = 4;
    drawScrewHead(ctx, fullRect.left + screwInset, fullRect.top + screwInset, screwR);
    drawScrewHead(ctx, fullRect.right - screwInset, fullRect.top + screwInset, screwR);
    drawScrewHead(ctx, fullRect.left + screwInset, fullRect.bottom - screwInset, screwR);
    drawScrewHead(ctx, fullRect.right - screwInset, fullRect.bottom - screwInset, screwR);

    // Section panels
    CRect synthSection(fullRect.left + 15, fullRect.top + 55,
                       fullRect.right - 15, fullRect.top + 310);
    drawSectionPanel(ctx, synthSection, "COMPLEX OSCILLATOR  /  WAVEFOLDER  /  LOW PASS GATE");

    CRect seqSection(fullRect.left + 15, fullRect.top + 320,
                     fullRect.right - 15, fullRect.bottom - 15);
    drawSectionPanel(ctx, seqSection, "SEQUENCER");

    // Voice labels on left side of synth section
    const char* voiceLabels[] = { "KICK", "SNARE", "CH", "OH", "TOM", "PERC" };
    CCoord labelY = synthSection.top + 35;
    ctx->setFont(kNormalFontVerySmall);
    for (int i = 0; i < kNumDrumVoices; ++i) {
        CRect lr(synthSection.left + 8, labelY + i * 38,
                 synthSection.left + 60, labelY + i * 38 + 14);
        ctx->setFontColor(CColor(180, 150, 80, 255));
        ctx->drawString(voiceLabels[i], lr, kLeftText);
    }

    // Knob labels
    const char* knobLabels[] = {
        "PITCH", "DECAY", "FM AMT", "FM RAT",
        "FOLD", "SYMM", "LPG", "LPG DEC",
        "RESO", "NOISE", "P.ENV", "P.DEC",
        "DRIVE", "PAN", "SHAPE", "LEVEL"
    };

    CCoord knobLabelX = synthSection.left + 70;
    CCoord knobLabelY = synthSection.top + 20;
    ctx->setFont(kNormalFontVerySmall);
    ctx->setFontColor(CColor(150, 150, 155, 200));

    for (int i = 0; i < 16; ++i) {
        int col = i % 8;
        int row = i / 8;
        CRect klr(knobLabelX + col * 72, knobLabelY + row * 125,
                  knobLabelX + col * 72 + 68, knobLabelY + row * 125 + 12);
        ctx->drawString(knobLabels[i], klr, kCenterText);
    }

    // Step numbers
    CCoord stepX = seqSection.left + 25;
    CCoord stepLabelY = seqSection.top + 20;
    ctx->setFont(kNormalFontVerySmall);
    ctx->setFontColor(CColor(120, 120, 125, 180));
    for (int i = 0; i < kNumSteps; ++i) {
        char num[4];
        snprintf(num, sizeof(num), "%d", i + 1);
        CCoord x = stepX + i * 42;
        CRect nr(x, stepLabelY, x + 36, stepLabelY + 12);
        ctx->drawString(num, nr, kCenterText);
    }

    // Beat group indicators
    ctx->setLineWidth(1.0);
    for (int i = 0; i < 4; ++i) {
        CCoord groupX = stepX + i * 4 * 42;
        CCoord groupEnd = groupX + 4 * 42 - 6;
        CCoord markerY = seqSection.top + 34;

        uint8_t alpha = (i % 2 == 0) ? 60 : 35;
        ctx->setFrameColor(CColor(200, 160, 80, alpha));
        ctx->drawLine(CPoint(groupX, markerY), CPoint(groupEnd, markerY));
    }

    // Transport labels
    CRect tempoLabelR(seqSection.right - 200, seqSection.top + 20,
                      seqSection.right - 150, seqSection.top + 32);
    ctx->setFontColor(CColor(150, 150, 155, 200));
    ctx->drawString("TEMPO", tempoLabelR, kCenterText);

    CRect swingLabelR(seqSection.right - 130, seqSection.top + 20,
                      seqSection.right - 80, seqSection.top + 32);
    ctx->drawString("SWING", swingLabelR, kCenterText);

    CRect masterLabelR(seqSection.right - 60, seqSection.top + 20,
                       seqSection.right - 10, seqSection.top + 32);
    ctx->drawString("MASTER", masterLabelR, kCenterText);

    CViewContainer::drawBackgroundRect(ctx, rect);
}

} // namespace WestCoastDrumSynth
} // namespace SineLanguage
