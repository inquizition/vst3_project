//------------------------------------------------------------------------
// Spectrumview.h
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

class SpectrumView : public VSTGUI::CView
{
public:
    using DataCallback = std::function<bool(float* outMagnitudes, int size)>;

    struct FreqCallbacks {
        std::function<void()>      onBegin;
        std::function<void(float)> onChange;
        std::function<void()>      onEnd;
    };

    // postCb  = post-filter (wet) signal
    // preCb   = pre-filter  (dry) signal  — optional, pass nullptr to omit
    explicit SpectrumView (const VSTGUI::CRect& size,
                           DataCallback postCb,
                           DataCallback preCb = nullptr)
        : CView (size)
        , mPostCallback (std::move (postCb))
        , mPreCallback  (std::move (preCb))
        , mMagnitudesPost (SpectrumAnalyzer::kNumBins, 0.f)
        , mSmoothedPost   (SpectrumAnalyzer::kNumBins, 0.f)
        , mMagnitudesPre  (SpectrumAnalyzer::kNumBins, 0.f)
        , mSmoothedPre    (SpectrumAnalyzer::kNumBins, 0.f)
    {
        setMouseEnabled (true);
        mTimer = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>(
            [this](VSTGUI::CVSTGUITimer*) { onTimer(); }, 33);
    }

    ~SpectrumView() override { if (mTimer) mTimer->stop(); }
    
    void setSampleRate (double sr) { mSampleRate = sr; }
    void setFreqCallbacks (FreqCallbacks cbs) { mFreqCbs = std::move (cbs); }
    void setFreqNorm (float f) { mFreqNorm = std::max (0.f, std::min (1.f, f)); invalid(); }

    //--- draw ----------------------------------------------------------
    void draw (VSTGUI::CDrawContext* ctx) override
    {
        const auto& r = getViewSize();
        ctx->setFillColor (VSTGUI::CColor (18, 18, 24, 255));
        ctx->drawRect (r, VSTGUI::kDrawFilled);
        drawGrid (ctx, r);

        // Draw pre-filter (dry) curve first so post-filter sits on top
        if (mPreCallback)
        {
            drawSpectrumFill (ctx, r, mSmoothedPre,
                              VSTGUI::CColor (239, 203, 123, 77));   // wheat fill, very faint
            drawSpectrumLine (ctx, r, mSmoothedPre,
                              VSTGUI::CColor (239, 238, 114, 43));   // wheat line, dim
        }

        // Post-filter (wet) curve on top
        drawSpectrumFill (ctx, r, mSmoothedPost,
                          VSTGUI::CColor (0, 46, 178, 139));         // cyan fill
        drawSpectrumLine (ctx, r, mSmoothedPost,
                          VSTGUI::CColor (0, 22, 202, 148));        // cyan line, bright

        if (mFreqNorm >= 0.f)
            drawFreqMarker (ctx, r);

        ctx->setFrameColor (VSTGUI::CColor (60, 60, 80, 255));
        ctx->setLineWidth (1.0);
        ctx->drawRect (r, VSTGUI::kDrawStroked);
    }

    //--- mouse ---------------------------------------------------------
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
    void applyMousePos (const VSTGUI::CPoint& where)
    {
        const auto& r = getViewSize();
        float logX = (float)(where.x - r.left) / (float)r.getWidth();
        mFreqNorm  = std::max (0.f, std::min (1.f, logX));
        if (mFreqCbs.onChange) mFreqCbs.onChange (mFreqNorm);
        invalid();
    }

    void onTimer()
    {
        bool dirty = false;

        if (mPostCallback && mPostCallback (mMagnitudesPost.data(), (int)mMagnitudesPost.size()))
        {
            smooth (mMagnitudesPost, mSmoothedPost);
            dirty = true;
        }
        if (mPreCallback && mPreCallback (mMagnitudesPre.data(), (int)mMagnitudesPre.size()))
        {
            smooth (mMagnitudesPre, mSmoothedPre);
            dirty = true;
        }
        if (dirty) invalid();
    }

    static void smooth (const std::vector<float>& src, std::vector<float>& dst)
    {
        for (int i = 0; i < (int)dst.size(); ++i)
        {
            float alpha = (src[i] > dst[i]) ? 0.7f : 0.15f;
            dst[i] = dst[i] + alpha * (src[i] - dst[i]);
        }
    }

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

    std::vector<VSTGUI::CPoint> buildPoints (const VSTGUI::CRect& r,
                                              const std::vector<float>& smoothed) const
    {
        const int   numBins  = (int)smoothed.size();
        const float sr       = (float)mSampleRate;
        const float fftSize  = (float)(numBins * 2);  // kFftSize = 2 * kNumBins

        // Use the same log-frequency scale as the processor: 20 Hz – 20 kHz.
        // This makes freqNorm == logX, so the marker and click positions are exact.
        constexpr float kMinHz = 20.f;
        constexpr float kMaxHz = 20000.f;
        const float logMin     = std::log10 (kMinHz);
        const float logRange   = std::log10 (kMaxHz) - logMin;

        std::vector<VSTGUI::CPoint> pts;
        pts.reserve (numBins);

        for (int i = 1; i < numBins; ++i)
        {
            const float freqHz = (float)i * sr / fftSize;
            if (freqHz < kMinHz || freqHz > kMaxHz) continue;

            const float logX = (std::log10 (freqHz) - logMin) / logRange;
            const float x    = (float)r.left + logX * (float)r.getWidth();
            const float y    = (float)r.bottom - smoothed[i] * (float)r.getHeight();
            pts.push_back ({ x, y });
        }
        return pts;
    }

    void drawSpectrumFill (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r,
                           const std::vector<float>& smoothed,
                           const VSTGUI::CColor& colour) const
    {
        auto pts = buildPoints (r, smoothed);
        if (pts.empty()) return;
        auto path = VSTGUI::owned (ctx->createGraphicsPath());
        path->beginSubpath ({ pts[0].x, (float)r.bottom });
        for (auto& p : pts) path->addLine (p);
        path->addLine ({ pts.back().x, (float)r.bottom });
        path->closeSubpath();
        ctx->setFillColor (colour);
        ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathFilled);
    }

    void drawSpectrumLine (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r,
                           const std::vector<float>& smoothed,
                           const VSTGUI::CColor& colour) const
    {
        auto pts = buildPoints (r, smoothed);
        if (pts.empty()) return;
        auto path = VSTGUI::owned (ctx->createGraphicsPath());
        path->beginSubpath (pts[0]);
        for (size_t i = 1; i < pts.size(); ++i) path->addLine (pts[i]);
        ctx->setFrameColor (colour);
        ctx->setLineWidth (1.5);
        ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathStroked);
    }

    void drawFreqMarker (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        const float x = (float)r.left + mFreqNorm * (float)r.getWidth();
        ctx->setLineWidth (1.0);
        ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 100));
        ctx->drawLine (VSTGUI::CPoint (x, (float)r.top), VSTGUI::CPoint (x, (float)r.bottom));

        const int numBins = (int)mSmoothedPost.size();
        const int bin     = std::max (1, std::min (numBins - 1,
                                (int)std::pow ((float)numBins, mFreqNorm)));
        const float dotY  = std::max ((float)r.top   + 5.f,
                            std::min ((float)r.bottom - 5.f,
                                (float)r.bottom - mSmoothedPost[bin] * (float)r.getHeight()));

        const float glowR = 9.f;
        VSTGUI::CRect glowRect (x - glowR, dotY - glowR, x + glowR, dotY + glowR);
        ctx->setFillColor (VSTGUI::CColor (196, 73, 0, 40));
        ctx->drawEllipse (glowRect, VSTGUI::kDrawFilled);

        const float dotR = 5.f;
        VSTGUI::CRect dotRect (x - dotR, dotY - dotR, x + dotR, dotY + dotR);
        ctx->setFillColor (VSTGUI::CColor (196, 73, 0, 230));
        ctx->drawEllipse (dotRect, VSTGUI::kDrawFilled);
        ctx->setLineWidth (1.5);
        ctx->setFrameColor (VSTGUI::CColor (239, 214, 172, 200));
        ctx->drawEllipse (dotRect, VSTGUI::kDrawStroked);
    }

    DataCallback  mPostCallback;
    DataCallback  mPreCallback;
    FreqCallbacks mFreqCbs;

    std::vector<float> mMagnitudesPost, mSmoothedPost;  // wet (cyan)
    std::vector<float> mMagnitudesPre,  mSmoothedPre;   // dry (wheat ghost)

    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> mTimer;
    float mFreqNorm = 0.5f;
    bool  mDragging = false;
    double mSampleRate = 44100.0;
};

} // namespace Steinberg
