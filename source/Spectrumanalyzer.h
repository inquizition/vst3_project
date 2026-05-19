//------------------------------------------------------------------------
// spectrumanalyzer.h
// Standalone FFT analyzer class.
// Accumulates samples, applies a Hann window, runs FFT, outputs magnitudes.
// Not tied to VST3 — easy to unit test independently.
//------------------------------------------------------------------------

#pragma once
#include <vector>
#include <cmath>
#include <cstring>

// Forward declare KissFFT types so we don't need the header here
struct kiss_fft_state;
typedef struct kiss_fft_state* kiss_fft_cfg;
struct kiss_fft_cpx { float r; float i; };

class SpectrumAnalyzer
{
public:
    static constexpr int kFftSize    = 2048;  // must be power of 2
    static constexpr int kNumBins    = kFftSize / 2;
    static constexpr float kMinDb    = -80.f;
    static constexpr float kMaxDb    =   0.f;

    SpectrumAnalyzer();
    ~SpectrumAnalyzer();

    // Feed raw samples (from ring buffer reads). Thread-safe for single caller.
    void pushSamples(const float* samples, int count);

    // Returns true if a new FFT frame is ready.
    // Call from UI thread — copies normalized magnitudes (0..1) into outMagnitudes.
    bool getLatestMagnitudes(float* outMagnitudes, int size);

private:
    void runFft();
    void buildHannWindow();
    float toNormalizedDb(float linearMagnitude) const;

    kiss_fft_cfg          mFftCfg     { nullptr };
    std::vector<float>    mWindow;          // Hann window coefficients
    std::vector<float>    mAccumBuffer;     // ring of incoming samples
    int                   mAccumPos   { 0 };
    int                   mHopCounter { 0 };

    // Output — written by runFft(), read by getLatestMagnitudes()
    // Protected by a simple dirty flag (fine for display; not for DSP)
    std::vector<float>    mMagnitudes;      // kNumBins normalized values
    std::atomic<bool>     mNewFrame { false };

    // Hop size: how many new samples between FFT frames (50% overlap)
    static constexpr int kHopSize = kFftSize / 2;
};
