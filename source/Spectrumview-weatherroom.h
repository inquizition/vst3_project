//------------------------------------------------------------------------
// Spectrumview-weatherroom.h
//
// Same SpectrumView, recolored for the Weatherroom "Cabin at dusk"
// palette so it sits on top of the new background.png cleanly.
//
// To use: replace the body of draw() / drawGrid / drawSpectrumFill /
// drawSpectrumLine in your source/Spectrumview.h with the colour values
// shown here. Everything else (timer, smoothing, points) is unchanged.
//------------------------------------------------------------------------

#pragma once

// Palette (matches handoff/background.png):
//   ink     = (4, 21, 31)      — background
//   slate   = (24, 58, 55)     — surfaces
//   wheat   = (239, 214, 172)  — text / primary line
//   ember   = (196, 73, 0)     — accent (POST spectrum)
//   line    = (239, 214, 172, 31)  — hairline borders (12% wheat)
//
// All other colours in this view are derived from these.

//----------- draw() : background + border -------------------------------
// (Replace the existing draw() body.)

void draw (VSTGUI::CDrawContext* ctx) override
{
    const auto& r = getViewSize ();

    // NB: the background.png already paints the FFT plate, so the
    // SpectrumView only needs to draw the spectrum itself + grid.
    // We skip the opaque background rect to let the plate show through.

    drawGrid          (ctx, r);
    drawSpectrumFill  (ctx, r);   // PRE — dim wheat
    drawSpectrumLine  (ctx, r);   // POST — bright ember
}

//----------- drawGrid : hairline wheat ---------------------------------

void drawGrid (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
{
    const float dbMarkers[] = { -20.f, -40.f, -60.f, -80.f };
    const float kRange      = SpectrumAnalyzer::kMaxDb - SpectrumAnalyzer::kMinDb;

    ctx->setLineWidth (0.5);
    ctx->setFrameColor (VSTGUI::CColor (239, 214, 172, 31)); // wheat @ 12%

    for (float db : dbMarkers)
    {
        const float normalized = (db - SpectrumAnalyzer::kMinDb) / kRange;
        const float y = (float) (r.bottom - normalized * r.getHeight ());
        ctx->drawLine ({ (float) r.left, y }, { (float) r.right, y });
    }
}

//----------- drawSpectrumFill : dim wheat for PRE-process --------------

void drawSpectrumFill (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
{
    auto pts = buildPoints (r);
    if (pts.empty ()) return;

    auto path = VSTGUI::owned (ctx->createGraphicsPath ());
    path->beginSubpath ({ pts[0].x, (float) r.bottom });
    for (auto& p : pts) path->addLine (p);
    path->addLine ({ pts.back ().x, (float) r.bottom });
    path->closeSubpath ();

    ctx->setFillColor (VSTGUI::CColor (239, 214, 172, 18)); // wheat @ 7%
    ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathFilled);
}

//----------- drawSpectrumLine : ember for POST-process -----------------

void drawSpectrumLine (VSTGUI::CDrawContext* ctx, const VSTGUI::CRect& r) const
{
    auto pts = buildPoints (r);
    if (pts.empty ()) return;

    auto path = VSTGUI::owned (ctx->createGraphicsPath ());
    path->beginSubpath (pts[0]);
    for (size_t i = 1; i < pts.size (); ++i) path->addLine (pts[i]);

    // Use ember for the *post* spectrum (after EQ).
    // If you're not yet plotting post separately, use wheat 65% for
    // a single-line look that still reads as "Weatherroom".
    ctx->setFrameColor (VSTGUI::CColor (196, 73, 0, 230)); // ember
    ctx->setLineWidth (1.6);
    ctx->drawGraphicsPath (path, VSTGUI::CDrawContext::kPathStroked);
}
