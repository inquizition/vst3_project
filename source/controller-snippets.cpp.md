# `helloworldcontroller.cpp` — what to change

Three small changes to `source/helloworldcontroller.cpp`:

1. Re-declare the parameter table to match the new IDs.
2. Reposition the existing `SpectrumView` to the new FFT rect.
3. Add a `BandOverlayView` on top of the SpectrumView so the user sees the band.

Everything else (`connect`, `setComponentState`, `terminate`, the editor subclass) stays.

---

## 1) Replace the body of `initialize()`

```cpp
tresult PLUGIN_API HelloWorldController::initialize (FUnknown* context)
{
    tresult result = EditControllerEx1::initialize (context);
    if (result != kResultOk)
        return result;

    mAnalyzer = std::make_unique<SpectrumAnalyzer>();

    // --- Bypass (host-managed) ---
    parameters.addParameter (
        STR16 ("Bypass"), nullptr, 1, 0,
        Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
        kBypassId);

    // --- Band gain : -18 .. +18 dB, default 0 (centre detent at 0.5) ---
    parameters.addParameter (
        STR16 ("Gain"), STR16 ("dB"), 0, 0.5,
        Vst::ParameterInfo::kCanAutomate, kParamGainId, 0, STR16 ("Gain"));

    // --- Q : 0.1 .. 10, default 1.0 ---
    // Normalised stored value uses a curve in the processor;
    // for now we expose it linearly and convert when used.
    parameters.addParameter (
        STR16 ("Q"), nullptr, 0, 0.2,
        Vst::ParameterInfo::kCanAutomate, kParamQId, 0, STR16 ("Q"));

    // --- Band centre frequency : 20 Hz .. 20 kHz, default 1 kHz ---
    parameters.addParameter (
        STR16 ("Freq"), STR16 ("Hz"), 0, 0.5,
        Vst::ParameterInfo::kCanAutomate, kParamFreqId, 0, STR16 ("Freq"));

    // --- Listen (solo the band) ---
    parameters.addParameter (
        STR16 ("Listen"), STR16 ("On/Off"), 1, 0,
        Vst::ParameterInfo::kCanAutomate, kParamListenId, 0, STR16 ("Listen"));

    return result;
}
```

> Note: the parameter conversion helpers (`gainNormToDb`, `qNormToQ`,
> `freqNormToHz`) belong in the processor / a small `Params.h` shared between
> both halves. Keep `parameters.addParameter` linear here — the curves are
> applied when the value is read.

---

## 2) Update `setComponentState()` to read the new parameter layout

```cpp
tresult PLUGIN_API HelloWorldController::setComponentState (IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer streamer (state, kLittleEndian);

    float gainNorm = 0.5f, qNorm = 0.2f, freqNorm = 0.5f;
    int8  listen   = 0;
    int32 bypass   = 0;

    if (!streamer.readFloat (gainNorm)) return kResultFalse;
    if (!streamer.readFloat (qNorm))    return kResultFalse;
    if (!streamer.readFloat (freqNorm)) return kResultFalse;
    if (!streamer.readInt8  (listen))   return kResultFalse;
    if (!streamer.readInt32 (bypass))   return kResultFalse;

    setParamNormalized (kParamGainId,   gainNorm);
    setParamNormalized (kParamQId,      qNorm);
    setParamNormalized (kParamFreqId,   freqNorm);
    setParamNormalized (kParamListenId, listen ? 1 : 0);
    setParamNormalized (kBypassId,      bypass ? 1 : 0);

    return kResultOk;
}
```

> Your processor's `setState` / `getState` must match this order — update
> `helloworldprocessor.cpp` to read/write the same five values in the same
> sequence.

---

## 3) Reposition `SpectrumView` and add `BandOverlayView` in `createView()`

In the `HelloWorldEditor::open()` override, change the two lines that
create the SpectrumView, and then add the overlay:

```cpp
#include "BandOverlayView.h"   // at the top of the file

// ... inside open() ...

// New FFT rect — matches the empty plate in background.png.
//   left   = 30        (20 page-padding + 10 inner padding)
//   top    = 68        (~58 header + ~10 inner padding)
//   right  = 30 + 840  = 870
//   bottom = 68 + 268  = 336
VSTGUI::CRect specRect (30, 68, 870, 336);

auto* specView = new SpectrumView (specRect,
    [this](float* outMag, int size) -> bool
    {
        if (mCtrl->mProcessor)
        {
            auto& rb = mCtrl->mProcessor->getSpectrumBuffer ();
            static std::vector<float> tmp (4096);
            size_t got = rb.read (tmp.data (), tmp.size ());
            if (got > 0)
                mCtrl->mAnalyzer->pushSamples (tmp.data (), (int)got);
        }
        return mCtrl->mAnalyzer->getLatestMagnitudes (outMag, size);
    });

getFrame ()->addView (specView);

// Band overlay on top of the spectrum.
auto* bandView = new BandOverlayView (specRect,
    [this]() -> BandState
    {
        // Pull normalised values from the controller and convert.
        const float gN = (float) mCtrl->getParamNormalized (kParamGainId);
        const float qN = (float) mCtrl->getParamNormalized (kParamQId);
        const float fN = (float) mCtrl->getParamNormalized (kParamFreqId);

        BandState s;
        s.gainDb = (gN - 0.5f) * 36.f;                       // -18 .. +18
        s.q      = 0.1f * std::pow (100.f, qN);              // 0.1 .. 10
        s.freqHz = 20.f * std::pow (1000.f, fN);             // 20 .. 20 000
        return s;
    });

getFrame ()->addView (bandView);
```

That's it for v1. The two knobs (Gain, Q) drive the overlay; the band's
visible width changes as Q changes; Listen and Bypass toggle the
processor.

---

## What's left for v2

- **Direct manipulation on the FFT** — extend `BandOverlayView` to override
  `onMouseDown/Moved/Up`, hit-test the centre dot and the L/R handles,
  and call `mCtrl->setParamNormalized (kParamFreqId, …)` etc. (use
  `mCtrl->beginEdit (tag) / performEdit (tag, val) / endEdit (tag)` so
  automation records cleanly).
- **Post-FFT plotting** — split the analyzer into pre/post buffers and
  draw the post curve in ember on top of the pre curve in dim wheat
  (see `Spectrumview-weatherroom.h`).
- **Meter readouts** (In / Out / GR / Width) — replace the static text
  baked into `background.png` with `CTextLabel` widgets bound to
  hidden parameters that the processor updates from its meter values.
- **Recolor `knob_strip.png` and `onoff_button.png`** so the knob face
  and pill toggle match the wheat-on-slate palette. Re-run
  `graphic_rotating_gen.py` (already in your repo) with the new colours.
