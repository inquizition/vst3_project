//------------------------------------------------------------------------
// Spectrumview.h
// Real-time spectrum analyzer view with interactive frequency marker.
//------------------------------------------------------------------------

#pragma once

#include "vstgui/lib/cview.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cvstguitimer.h"

#include "Spectrumanalyzer.h"

#include <vector>
#include <functional>
#include <cmath>

namespace Steinberg {

//------------------------------------------------------------------------
class SpectrumView : public VSTGUI::CView
{
public:
    using DataCallback = std::function<bool(float* outMagnitudes, int size)>;

    // Callbacks for when the user clicks/drags the frequency marker.
    // Wire these to beginEdit / performEdit / endEdit on the controller.
    struct FreqCallbacks {
        std::function<void()>      onBegin;   // mouse down
        std::function<void(float)> onChange;  // drag / click  (freqNorm 0-1)
        std::function<void()>      onEnd;     // mouse up
    };

    explicit SpectrumView (const VSTGUI::CRect& size, DataCallback cb)
        : CView (size)
        , mDataCallback (std::move (cb))
        , mMagnitudes (SpectrumAnalyzer::kNumBins, 0.f)
        , mSmoothed   (SpectrumAnalyzer::kNumBins, 0.f)
    {
        setMouseEnabled (true);
        mTimer = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>(
            [this](VSTGUI::CVSTGUITimer*) { onTimer(); }, 33);
    }

    ~SpectrumView() override
    {
        if (mTimer) mTimer->stop();
    }

    void setFreqCallbacks (FreqCallbacks cbs) { mFreqCbs = std::move (cbs); }

    // Call this after creating the view to show the current parameter value.
    void setFreqNorm (float f)
    {
        mFreqNorm = std::max (0.f, std::min (1.f, f));
        invalid ();
    }

    //--- CView overrides ---------------------------------------------------

    void draw (VSTGUI::CDrawContext* ctx) override
    {
        const auto& r = getViewSize();
        ctx->setFillColor (VSTGUI::CColor (18, 18, 24, 255));
        ctx->drawRect (r, VSTGUI::kDrawFilled);
        drawGrid (ctx, r);
        drawSpectrumFill (ctx, r);
        drawSpectrumLine (ctx, r);
        if (mFreqNorm >= 0.f)
            drawFreqMarker (ctx, r);
        ctx->setFrameColor (VSTGUI::CColor (60, 60, 80, 255));
        ctx->setLineWidth (1.0);
        ctx->drawRect (r, VSTGUI::kDrawStroked);
    }

    VSTGUI::CMouseEventResult onMouseDown (VSTGUI::CPoint& where,
                                           const VSTGUI::CButtonState& buttons) override
    {
        if (!buttons.isLeftButton()) return VSTGUI::kMouseEventNotHandled;
        mDragging = true;
        if (mFreqCbs.onBegin) mFreqCbs.onBegin();
        applyMousePos (where);
        return VSTGUI::kMouseEventHandled;
    }

    VSTGUI::CMouseEventResult onMouseMoved (VSTGUI::CPoint& where,
                                            const VSTGUI::CButtonState& buttons) override
    {
        if (!mDragging) return VSTGUI::kMouseEventNotHandled;
        applyMousePos (where);
        return VSTGUI::kMouseEventHandled;
    }

    VSTGUI::CMouseEventResult onMouseUp (VSTGUI::CPoint& where,
                                         const VSTGUI::CButtonState& buttons) override
    {
        if (!mDragging) return VSTGUI::kMouseEventNotHandled;
        mDragging = false;
        if (mFreqCbs.onEnd) mFreqCbs.onEnd();
        return VSTGUI::kMouseEventHandled;
    }

private:
    //-----------------------------------------------------------------------
    void applyMousePos (const VSTGUI::CPoint& where)
    {
        const auto& r = getViewSize();
        float logX = (float)(where.x - r.left) / (float)r.getWidth();
        mFreqNorm  = std::max (0.f, std::min (1.f, logX));
        if (mFreqCbs.onChange) mFreqCbs.onChange (mFreqNorm);
        invalid();
    }

    //-----------------------------------------------------------------------
    void onTimer()
    {
        if (mDataCallback && mDataCallback (mMagnitudes.data(), (int)mMagnitudes.size()))
        {
            for (int i = 0; i < (int)mSmoothed.size(); ++i)
            {
                float target = mMagnitudes[i];
                float alpha  = (target > mSmoothed[i]) ? 0.7f : 0.15f;
                mSmoothed[i] = mSmoothed[i] + alpha * (target - mSmoothed[i]);
            }
            invalid();
        }
    }

    //-----------------------------------------------------------------------
    void drawGrid (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        const float dbMarkers[] = { -20.f, -40.f, -60.f, -80.f };
        const float kRange = SpectrumAnalyzer::kMaxDb - SpectrumAnalyzer::kMinDb;
        ctx->setLineWidth (0.5);
        ctx->setFrameColor (VSTGUI::CColor (50, 50, 70, 180));
        for (float db : dbMarkers)
        {
            float norm = (db - SpectrumAnalyzer::kMinDb) / kRange;
            float y    = (float)(r.bottom - norm * r.getHeight());
            ctx->drawLine (VSTGUI::CPoint (r.left, y), VSTGUI::CPoint (r.right, y));
        }
    }

    //-----------------------------------------------------------------------
    std::vector<VSTGUI::CPoint> buildPoints (const VSTGUI::CRect& r) const
    {
        const int   numBins  = (int)mSmoothed.size();
        const float width    = (float)r.getWidth();
        const float height   = (float)r.getHeight();
        const float logMin   = std::log10 (1.f);
        const float logMax   = std::log10 ((float)numBins);
        const float logRange = logMax - logMin;

        std::vector<VSTGUI::CPoint> pts;
        pts.reserve (numBins);
        for (int i = 1; i < numBins; ++i)
        {
            float logX = (std::log10 ((float)i) - logMin) / logRange;
            float x    = (float)r.left + logX * width;
            float y    = (float)r.bottom - mSmoothed[i] * height;
            pts.push_back ({ x, y });
        }
        return pts;
    }

    //-----------------------------------------------------------------------
    void drawSpectrumFill (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        auto pts = buildPoints (r);
        if (pts.empty()) return;
        auto path = VSTGUI::owned (ctx->createGraphicsPath());
        path->beginSubpath ({ pts[0].x, (float)r.bottom });
        for (auto& p : pts) path->addLine (p);
        path->addLine ({ pts.back().x, (float)r.bottom });
        path->closeSubpath();
        ctx->setFillColor (VSTGUI::CColor (0, 180, 220, 40));
        ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathFilled);
    }

    //-----------------------------------------------------------------------
    void drawSpectrumLine (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        auto pts = buildPoints (r);
        if (pts.empty()) return;
        auto path = VSTGUI::owned (ctx->createGraphicsPath());
        path->beginSubpath (pts[0]);
        for (size_t i = 1; i < pts.size(); ++i) path->addLine (pts[i]);
        ctx->setFrameColor (VSTGUI::CColor (0, 210, 255, 230));
        ctx->setLineWidth (1.5);
        ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathStroked);
    }

    //-----------------------------------------------------------------------
    void drawFreqMarker (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        const float x = (float)r.left + mFreqNorm * (float)r.getWidth();

        // Vertical line — ember colour, semi-transparent
        ctx->setLineWidth (1.0);
        ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 100));
        ctx->drawLine (VSTGUI::CPoint (x, (float)r.top),
                       VSTGUI::CPoint (x, (float)r.bottom));

        // Find the spectrum magnitude at this freq position.
        // mFreqNorm maps to the same log-bin axis as buildPoints().
        const int numBins = (int)mSmoothed.size();
        const int bin     = std::max (1, std::min (numBins - 1,
                                (int)std::pow ((float)numBins, mFreqNorm)));
        const float dotY  = std::max ((float)r.top   + 5.f,
                            std::min ((float)r.bottom - 5.f,
                                (float)r.bottom - mSmoothed[bin] * (float)r.getHeight()));

        // Outer glow ring
        const float glowR = 9.f;
        VSTGUI::CRect glowRect (x - glowR, dotY - glowR, x + glowR, dotY + glowR);
        ctx->setFillColor (VSTGUI::CColor (196, 73, 0, 40));
        ctx->drawEllipse (glowRect, VSTGUI::kDrawFilled);

        // Filled dot — ember
        const float dotR = 5.f;
        VSTGUI::CRect dotRect (x - dotR, dotY - dotR, x + dotR, dotY + dotR);
        ctx->setFillColor (VSTGUI::CColor (196, 73, 0, 230));
        ctx->drawEllipse (dotRect, VSTGUI::kDrawFilled);

        // Dot border — wheat
        ctx->setLineWidth (1.5);
        ctx->setFrameColor (VSTGUI::CColor (239, 214, 172, 200));
        ctx->drawEllipse (dotRect, VSTGUI::kDrawStroked);
    }

    //-----------------------------------------------------------------------
    DataCallback   mDataCallback;
    FreqCallbacks  mFreqCbs;
    std::vector<float> mMagnitudes;
    std::vector<float> mSmoothed;
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> mTimer;

    float mFreqNorm = 0.5f;  // normalised (0-1), matches kParamFreqId default
    bool  mDragging = false;
};

} // namespace Steinberg
