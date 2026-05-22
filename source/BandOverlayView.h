//------------------------------------------------------------------------
// BandOverlayView.h
// Sits on top of SpectrumView and draws the currently-selected EQ band:
//   - vertical dashed band region
//   - bell-curve guide line (filter response in dB across the spectrum)
//   - center marker dot (at gain position)
//   - left/right edge handles
//
// Reads three normalized parameters via a small callback so this view
// has no direct dependency on your controller. Wire it up in
// HelloWorldController::createView (see handoff/controller-snippets.cpp.md).
//
// You can extend this later to handle mouse drag (move center, resize Q,
// pull gain). For v1 it's display-only and the knobs drive everything.
//------------------------------------------------------------------------

#pragma once

#include "vstgui/lib/cview.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cvstguitimer.h"
#include <functional>
#include <cmath>

namespace Steinberg {

//------------------------------------------------------------------------
// Snapshot of band parameters in real (not normalized) units.
struct BandState
{
    float freqHz;   //   20 .. 20000
    float q;        //  0.1 .. 10
    float gainDb;   //  -18 .. +18
};

//------------------------------------------------------------------------
class BandOverlayView : public VSTGUI::CView
{
public:
    using StateCallback = std::function<BandState()>;

    BandOverlayView (const VSTGUI::CRect& size, StateCallback cb)
        : CView (size)
        , mState (std::move (cb))
    {
        // Re-render at 30 fps so knob changes feel live.
        mTimer = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer> (
            [this] (VSTGUI::CVSTGUITimer*) { invalid (); }, 33);
    }

    ~BandOverlayView () override { if (mTimer) mTimer->stop (); }

    void draw (VSTGUI::CDrawContext* ctx) override
    {
        if (!mState) return;
        const auto r  = getViewSize ();
        const auto bs = mState ();

        // Map freq → log-x in [0..1] using a 20Hz..20kHz reference range.
        const float xMin = std::log10 (20.f);
        const float xMax = std::log10 (20000.f);
        auto freqToX = [&] (float hz) -> float
        {
            const float lx = std::log10 (std::max (1.f, hz));
            return (lx - xMin) / (xMax - xMin);
        };

        // Band edges: ±(0.5 / Q) octaves around the center frequency.
        const float octHalf  = 0.5f / std::max (0.1f, bs.q);
        const float fCenter  = bs.freqHz;
        const float fLeft    = fCenter * std::pow (2.f, -octHalf);
        const float fRight   = fCenter * std::pow (2.f,  octHalf);

        const float xC = freqToX (fCenter);
        const float xL = freqToX (fLeft);
        const float xR = freqToX (fRight);

        const float xPxC = (float) r.left + xC * (float) r.getWidth ();
        const float xPxL = (float) r.left + xL * (float) r.getWidth ();
        const float xPxR = (float) r.left + xR * (float) r.getWidth ();

        // ±18 dB maps to top/bottom; 0 dB is the midline.
        const float yMid = (float) r.top + (float) r.getHeight () * 0.5f;
        const float dBPerPx = 36.f / (float) r.getHeight ();
        const float yGain   = yMid - bs.gainDb / dBPerPx;

        // ---- band region (dashed rectangle) ----
        ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 90));   // ember 35%
        ctx->setFillColor  (VSTGUI::CColor (196, 73, 0, 10));   // ember 4%
        ctx->setLineWidth (1.0);
        ctx->setLineStyle (VSTGUI::CLineStyle (VSTGUI::CLineStyle::kLineCapButt,
                                               VSTGUI::CLineStyle::kLineJoinMiter,
                                               0.0, { 3.0, 3.0 }));
        VSTGUI::CRect bandRect (xPxL, r.top, xPxR, r.bottom);
        ctx->drawRect (bandRect, VSTGUI::kDrawFilledAndStroked);
        ctx->setLineStyle (VSTGUI::kLineSolid);

        // ---- bell-curve guide ----
        // ember 1.5px, sampled across the width.
        auto path = VSTGUI::owned (ctx->createGraphicsPath ());
        const int  N    = 180;
        const float halfWidthLog = octHalf * 0.30103f; // 0.5/Q octaves in log10 units
        for (int i = 0; i <= N; ++i)
        {
            const float t  = (float) i / (float) N;
            const float xPx = (float) r.left + t * (float) r.getWidth ();
            // Convert x back to log-freq, then evaluate bell.
            const float lx = xMin + t * (xMax - xMin);
            const float dl = (lx - std::log10 (fCenter)) / std::max (0.001f, halfWidthLog);
            const float dB = bs.gainDb * std::exp (- (dl * dl) * 1.6f);
            const float yPx = yMid - dB / dBPerPx;
            if (i == 0) path->beginSubpath ({ xPx, yPx });
            else         path->addLine     ({ xPx, yPx });
        }
        ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 220));
        ctx->setLineWidth (1.6);
        ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathStroked);

        // ---- center vertical line ----
        ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 140));
        ctx->setLineWidth (1.0);
        ctx->drawLine ({ xPxC, (float) r.top }, { xPxC, (float) r.bottom });

        // ---- edge handles (small rectangles on the midline) ----
        auto drawHandle = [&] (float x)
        {
            VSTGUI::CRect h (x - 3, yMid - 8, x + 3, yMid + 8);
            ctx->setFillColor (VSTGUI::CColor (4, 21, 31, 255));   // ink
            ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 255)); // ember
            ctx->setLineWidth (1.0);
            ctx->drawRect (h, VSTGUI::kDrawFilledAndStroked);
        };
        drawHandle (xPxL);
        drawHandle (xPxR);

        // ---- gain dot ----
        VSTGUI::CRect dot (xPxC - 6, yGain - 6, xPxC + 6, yGain + 6);
        ctx->setFillColor (VSTGUI::CColor (4, 21, 31, 255));
        ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 255));
        ctx->setLineWidth (1.5);
        ctx->drawEllipse (dot, VSTGUI::kDrawFilledAndStroked);

        VSTGUI::CRect inner (xPxC - 2, yGain - 2, xPxC + 2, yGain + 2);
        ctx->setFillColor (VSTGUI::CColor (196, 73, 0, 255));
        ctx->drawEllipse (inner, VSTGUI::kDrawFilled);
    }

private:
    StateCallback                                mState;
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer>  mTimer;
};

} // namespace Steinberg
