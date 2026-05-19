//------------------------------------------------------------------------
// spectrumanalyzer.cpp
//------------------------------------------------------------------------

#include "spectrumanalyzer.h"
#include "kiss_fft.h"   // from external/kissfft/

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpectrumAnalyzer::SpectrumAnalyzer()
{
    mFftCfg = kiss_fft_alloc(kFftSize, 0 /*forward*/, nullptr, nullptr);

    mWindow.resize(kFftSize);
    buildHannWindow();

    mAccumBuffer.assign(kFftSize, 0.f);
    mMagnitudes.assign(kNumBins, 0.f);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    if (mFftCfg)
        kiss_fft_free(mFftCfg);
}

void SpectrumAnalyzer::buildHannWindow()
{
    for (int i = 0; i < kFftSize; ++i)
        mWindow[i] = 0.5f * (1.f - std::cos(2.f * (float)M_PI * i / (kFftSize - 1)));
}

void SpectrumAnalyzer::pushSamples(const float* samples, int count)
{
    for (int i = 0; i < count; ++i)
    {
        // Write into circular accumulation buffer
        mAccumBuffer[mAccumPos] = samples[i];
        mAccumPos = (mAccumPos + 1) % kFftSize;

        // Every kHopSize new samples, run a new FFT frame (50% overlap)
        ++mHopCounter;
        if (mHopCounter >= kHopSize)
        {
            mHopCounter = 0;
            runFft();
        }
    }
}

void SpectrumAnalyzer::runFft()
{
    // Build windowed input — de-rotate the circular buffer as we go
    std::vector<kiss_fft_cpx> in(kFftSize), out(kFftSize);

    for (int i = 0; i < kFftSize; ++i)
    {
        int idx  = (mAccumPos + i) % kFftSize;
        in[i].r  = mAccumBuffer[idx] * mWindow[i];
        in[i].i  = 0.f;
    }

    kiss_fft(mFftCfg, in.data(), out.data());

    // Convert complex output to normalized dB magnitudes
    const float normFactor = 1.f / kFftSize;

    for (int bin = 0; bin < kNumBins; ++bin)
    {
        float re  = out[bin].r * normFactor;
        float im  = out[bin].i * normFactor;
        float mag = std::sqrt(re * re + im * im);
        mMagnitudes[bin] = toNormalizedDb(mag);
    }

    mNewFrame.store(true, std::memory_order_release);
}

bool SpectrumAnalyzer::getLatestMagnitudes(float* outMagnitudes, int size)
{
    if (!mNewFrame.load(std::memory_order_acquire))
        return false;

    mNewFrame.store(false, std::memory_order_release);

    const int toCopy = std::min(size, kNumBins);
    std::memcpy(outMagnitudes, mMagnitudes.data(), toCopy * sizeof(float));
    return true;
}

float SpectrumAnalyzer::toNormalizedDb(float linearMagnitude) const
{
    if (linearMagnitude <= 0.f)
        return 0.f;

    // Convert to dB
    float db = 20.f * std::log10(linearMagnitude);

    // Clamp and normalize to 0..1
    db = std::max(db, kMinDb);
    db = std::min(db, kMaxDb);
    return (db - kMinDb) / (kMaxDb - kMinDb);
}
