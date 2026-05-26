//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "spsc_ringbuffer.h"
#include "BiquadFilter.h"

namespace Steinberg {

//------------------------------------------------------------------------
//  HelloWorldProcessor
//------------------------------------------------------------------------
class HelloWorldProcessor : public Steinberg::Vst::AudioEffect
{
public:
    HelloWorldProcessor ();
    ~HelloWorldProcessor () SMTG_OVERRIDE;

    static Steinberg::FUnknown* createInstance (void* /*context*/)
    {
        return (Steinberg::Vst::IAudioProcessor*)new HelloWorldProcessor;
    }

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

    SpscRingBuffer<float, 32768>& getSpectrumBuffer () { return mSpectrumBuffer; }

//------------------------------------------------------------------------
protected:
    // Convert normalised (0-1) parameter values to physical units
    static double normToFreqHz (double norm) { return 20.0  * std::pow (1000.0, norm); }
    static double normToGainDb (double norm) { return (norm - 0.5) * 36.0; }
    static double normToQ      (double norm) { return 0.1   * std::pow (100.0,  norm); }

    void updateFilter ();

    // Normalised parameter state (mirrors what the controller holds)
    Vst::ParamValue mParam1    = 0.5;  // gain  (0-1, centre = 0 dB)
    Vst::ParamValue mParamQ    = 0.2;  // Q     (0-1)
    Vst::ParamValue mParamFreq = 0.5;  // freq  (0-1)
    int32           mParam2    = 0;    // listen
    bool            mBypass    = false;
    bool            mFilterDirty = true;

    // One filter per channel (stereo)
    BiquadFilter mFilterL;
    BiquadFilter mFilterR;

    SpscRingBuffer<float, 32768> mSpectrumBuffer;
};

//------------------------------------------------------------------------
} // namespace Steinberg
