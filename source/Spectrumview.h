//------------------------------------------------------------------------
// spectrumview.h
// Custom VSTGUI CView that draws a real-time spectrum analyzer.
// Driven by a CVSTGUITimer that polls the shared SpectrumAnalyzer.
//------------------------------------------------------------------------

#pragma once

#include "vstgui/lib/cview.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/animation/timingfunctions.h"
#include "vstgui/lib/cvstguitimer.h"

#include "spectrumanalyzer.h"

#include <vector>
#include <functional>

namespace Steinberg {

//------------------------------------------------------------------------
// SpectrumView
//------------------------------------------------------------------------
class SpectrumView : public VSTGUI::CView
{
public:
    // Callback signature: the view calls this to get fresh magnitude data.
    // Implement it in your editor to pull from the shared SpectrumAnalyzer.
    using DataCallback = std::function<bool(float* outMagnitudes, int size)>;

    explicit SpectrumView(const VSTGUI::CRect& size, DataCallback cb)
        : CView(size)
        , mDataCallback(std::move(cb))
        , mMagnitudes(SpectrumAnalyzer::kNumBins, 0.f)
        , mSmoothed(SpectrumAnalyzer::kNumBins, 0.f)
    {
        // Poll at ~30 fps
        mTimer = VSTGUI::makeOwned<VSTGUI::CVSTGUITimer>(
            [this](VSTGUI::CVSTGUITimer*) { onTimer(); }, 33);
    }

    ~SpectrumView() override
    {
        if (mTimer)
            mTimer->stop();
    }

    //--- CView overrides ---------------------------------------------------

    void draw(VSTGUI::CDrawContext* ctx) override
    {
        const auto& r = getViewSize();

        // --- Background ---
        ctx->setFillColor(VSTGUI::CColor(18, 18, 24, 255));
        ctx->drawRect(r, VSTGUI::kDrawFilled);

        // --- Grid lines (horizontal dB markers) ---
        drawGrid(ctx, r);

        // --- Spectrum fill (area under curve) ---
        drawSpectrumFill(ctx, r);

        // --- Spectrum line (curve on top) ---
        drawSpectrumLine(ctx, r);

        // --- Border ---
        ctx->setFrameColor(VSTGUI::CColor(60, 60, 80, 255));
        ctx->setLineWidth(1.0);
        ctx->drawRect(r, VSTGUI::kDrawStroked);
    }

    bool getIsTransparent() const override { return false; }

private:
    //-----------------------------------------------------------------------
    void onTimer()
    {
        if (mDataCallback && mDataCallback(mMagnitudes.data(), (int)mMagnitudes.size()))
        {
            // Smooth toward the new frame (exponential moving average)
            // Attack faster than release for a nice "peak decay" feel
            for (int i = 0; i < (int)mSmoothed.size(); ++i)
            {
                float target = mMagnitudes[i];
                float alpha  = (target > mSmoothed[i]) ? 0.7f : 0.15f;
                mSmoothed[i] = mSmoothed[i] + alpha * (target - mSmoothed[i]);
            }
            invalid(); // schedule redraw
        }
    }

    //-----------------------------------------------------------------------
    void drawGrid(VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        // Draw 4 horizontal lines at -20, -40, -60, -80 dB
        const float dbMarkers[] = { -20.f, -40.f, -60.f, -80.f };
        const float kRange      = SpectrumAnalyzer::kMaxDb - SpectrumAnalyzer::kMinDb; // 80 dB

        ctx->setLineWidth(0.5);
        ctx->setFrameColor(VSTGUI::CColor(50, 50, 70, 180));

        for (float db : dbMarkers)
        {
            float normalized = (db - SpectrumAnalyzer::kMinDb) / kRange;
            float y = (float)(r.bottom - normalized * r.getHeight());

            VSTGUI::CPoint p1(r.left,  y);
            VSTGUI::CPoint p2(r.right, y);
            ctx->drawLine(p1, p2);
        }
    }

    //-----------------------------------------------------------------------
    // Builds the (x, y) screen points from smoothed magnitudes.
    // Uses a logarithmic frequency axis (matches how ears perceive pitch).
    std::vector<VSTGUI::CPoint> buildPoints(const VSTGUI::CRect& r) const
    {
        const int   numBins  = (int)mSmoothed.size();
        const float width    = (float)r.getWidth();
        const float height   = (float)r.getHeight();
        const float logMin   = std::log10(1.f);
        const float logMax   = std::log10((float)numBins);
        const float logRange = logMax - logMin;

        std::vector<VSTGUI::CPoint> pts;
        pts.reserve(numBins);

        for (int i = 1; i < numBins; ++i)
        {
            float logX = (std::log10((float)i) - logMin) / logRange;
            float x    = (float)r.left + logX * width;
            float y    = (float)r.bottom - mSmoothed[i] * height;
            pts.push_back({ x, y });
        }
        return pts;
    }

    //-----------------------------------------------------------------------
    void drawSpectrumFill(VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        auto pts = buildPoints(r);
        if (pts.empty()) return;

        auto path = VSTGUI::owned(ctx->createGraphicsPath());
        path->beginSubpath({ pts[0].x, (float)r.bottom });
        for (auto& p : pts)
            path->addLine(p);
        path->addLine({ pts.back().x, (float)r.bottom });
        path->closeSubpath();

        // Gradient-style fill: semi-transparent cyan
        ctx->setFillColor(VSTGUI::CColor(0, 180, 220, 40));
        ctx->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathFilled);
    }

    //-----------------------------------------------------------------------
    void drawSpectrumLine(VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
    {
        auto pts = buildPoints(r);
        if (pts.empty()) return;

        auto path = VSTGUI::owned(ctx->createGraphicsPath());
        path->beginSubpath(pts[0]);
        for (size_t i = 1; i < pts.size(); ++i)
            path->addLine(pts[i]);

        ctx->setFrameColor(VSTGUI::CColor(0, 210, 255, 230));
        ctx->setLineWidth(1.5);
        ctx->drawGraphicsPath(path, VSTGUI::CDrawContext::kPathStroked);
    }

    //-----------------------------------------------------------------------
    DataCallback                        mDataCallback;
    std::vector<float>                  mMagnitudes;  // latest raw FFT data
    std::vector<float>                  mSmoothed;    // smoothed for display
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> mTimer;
};

} // namespace Steinberg
