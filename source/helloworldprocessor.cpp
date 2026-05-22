//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "helloworldprocessor.h"
#include "helloworldcids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;

namespace Steinberg {
//------------------------------------------------------------------------
// HelloWorldProcessor
//------------------------------------------------------------------------
HelloWorldProcessor::HelloWorldProcessor ()
{
	setControllerClass (kHelloWorldControllerUID);
}

//------------------------------------------------------------------------
HelloWorldProcessor::~HelloWorldProcessor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::initialize (FUnknown* context)
{
	tresult result = AudioEffect::initialize (context);
	if (result != kResultOk)
		return result;

	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::terminate ()
{
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::setActive (TBool state)
{
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::process (Vst::ProcessData& data)
{
    if (data.inputParameterChanges)
    {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
        for (int32 index = 0; index < numParamsChanged; index++)
        {
            Vst::IParamValueQueue* paramQueue =
                data.inputParameterChanges->getParameterData (index);
            if (paramQueue)
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount ();
                switch (paramQueue->getParameterId ())
                {
                    case HelloWorldParams::kParamGainId:
                        if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                            kResultTrue)
                            mParam1 = value;
                        break;
                    case HelloWorldParams::kParamQId:
                        if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                            kResultTrue)
                            mParamQ = value;
                        break;
                    case HelloWorldParams::kParamFreqId:
                        if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                            kResultTrue)
                            mParamFreq = value;
                        break;
                    case HelloWorldParams::kParamListenId:
                        if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                            kResultTrue)
                            mParam2 = value > 0.5 ? 1 : 0;
                        break;
                    case HelloWorldParams::kBypassId:
                        if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) ==
                            kResultTrue)
                            mBypass = (value > 0.5f);
                        break;
                }
            }
        }
    }

    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples == 0)
        return kResultOk;

    int32  numChannels  = data.inputs[0].numChannels;
    uint32 sampleFrames = data.numSamples;

    if (mBypass)
    {
        for (int32 ch = 0; ch < numChannels; ch++)
        {
            float* in  = data.inputs[0].channelBuffers32[ch];
            float* out = data.outputs[0].channelBuffers32[ch];
            memcpy(out, in, sampleFrames * sizeof(float));
        }
        if (numChannels > 0)
            mSpectrumBuffer.write(data.inputs[0].channelBuffers32[0], sampleFrames);
        return kResultOk;
    }

    float gain = (float)mParam1;

    for (int32 ch = 0; ch < numChannels; ch++)
    {
        float* in  = data.inputs[0].channelBuffers32[ch];
        float* out = data.outputs[0].channelBuffers32[ch];

        for (uint32 s = 0; s < sampleFrames; s++)
            out[s] = in[s] * gain;
    }

    if (numChannels > 0)
        mSpectrumBuffer.write(data.outputs[0].channelBuffers32[0], sampleFrames);

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	float savedGain = 0.5f, savedQ = 0.2f, savedFreq = 0.5f;
	int32 savedListen = 0, savedBypass = 0;

	if (streamer.readFloat (savedGain)   == false) return kResultFalse;
	if (streamer.readFloat (savedQ)      == false) return kResultFalse;
	if (streamer.readFloat (savedFreq)   == false) return kResultFalse;
	if (streamer.readInt32 (savedListen) == false) return kResultFalse;
	if (streamer.readInt32 (savedBypass) == false) return kResultFalse;

	mParam1    = savedGain;
	mParamQ    = savedQ;
	mParamFreq = savedFreq;
	mParam2    = savedListen > 0 ? 1 : 0;
	mBypass    = savedBypass > 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::getState (IBStream* state)
{
	IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat ((float)mParam1);
	streamer.writeFloat ((float)mParamQ);
	streamer.writeFloat ((float)mParamFreq);
	streamer.writeInt32 (mParam2);
	streamer.writeInt32 (mBypass ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Steinberg