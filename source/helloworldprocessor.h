//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "spsc_ringbuffer.h"
#include "BiquadFilter.h"

namespace Steinberg {

class HelloWorldProcessor : public Steinberg::Vst::AudioEffect
{
public:
    HelloWorldProcessor ();
    ~HelloWorldProcessor () SMTG_OVERRIDE;

    static Steinberg::FUnknown* createInstance (void* /*context*/)
    {
        return (Steinberg::Vst::IAudioProcessor*)new HelloWorldProcessor;
    }

    Steinberg::tresult PLUGIN_API initialize       (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate        () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive        (Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing  (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process          (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState         (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState         (Steinberg::IBStream* state) SMTG_OVERRIDE;

    SpscRingBuffer<float, 32768>& getSpectrumBuffer    () { return mSpectrumBuffer; }
    SpscRingBuffer<float, 32768>& getSpectrumBufferPre () { return mSpectrumBufferPre; }
    double getSampleRate () const { return mSampleRate; }

//------------------------------------------------------------------------
protected:
    static double normToFreqHz (double n) { return 20.0  * std::pow (1000.0, n); }
    static double normToGainDb (double n) { return (n - 0.5) * 36.0; }
    static double normToQ      (double n) { return 0.1   * std::pow (100.0,  n); }

    void updateFilter ();

    Vst::ParamValue mParam1    = 0.5;
    Vst::ParamValue mParamQ    = 0.2;
    Vst::ParamValue mParamFreq = 0.5;
    int32           mParam2    = 0;
    bool            mBypass      = false;
    bool            mFilterDirty = true;

    BiquadFilter mFilterL;
    BiquadFilter mFilterR;

    SpscRingBuffer<float, 32768> mSpectrumBuffer;     // post-filter (wet)
    SpscRingBuffer<float, 32768> mSpectrumBufferPre;  // pre-filter  (dry)
    double mSampleRate = 44100.0;
};

} // namespace Steinberg
