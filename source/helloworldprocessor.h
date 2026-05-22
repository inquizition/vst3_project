//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "spsc_ringbuffer.h"

namespace Steinberg {

//------------------------------------------------------------------------
//  HelloWorldProcessor
//------------------------------------------------------------------------
class HelloWorldProcessor : public Steinberg::Vst::AudioEffect
{
public:
	HelloWorldProcessor ();
	~HelloWorldProcessor () SMTG_OVERRIDE;

    // Create function
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

	SpscRingBuffer<float, 32768>& getSpectrumBuffer() { return mSpectrumBuffer; }

//------------------------------------------------------------------------
protected:
	Vst::ParamValue mParam1    = 0.5;  // gain (normalised 0-1)
	Vst::ParamValue mParamQ    = 0.2;  // Q   (normalised 0-1)
	Vst::ParamValue mParamFreq = 0.5;  // freq (normalised 0-1)
	int32 mParam2 = 0;                 // listen
	bool mBypass = false;
	SpscRingBuffer<float, 32768> mSpectrumBuffer;
};

//------------------------------------------------------------------------
} // namespace Steinberg