//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "helloworldprocessor.h"
#include "helloworldcids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;

namespace Steinberg {

HelloWorldProcessor::HelloWorldProcessor ()
{
    setControllerClass (kHelloWorldControllerUID);
}

HelloWorldProcessor::~HelloWorldProcessor () {}

tresult PLUGIN_API HelloWorldProcessor::initialize (FUnknown* context)
{
    tresult result = AudioEffect::initialize (context);
    if (result != kResultOk) return result;
    addAudioInput  (STR16 ("Stereo In"),  Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
    addEventInput  (STR16 ("Event In"), 1);
    return kResultOk;
}

tresult PLUGIN_API HelloWorldProcessor::terminate ()
{
    return AudioEffect::terminate ();
}

tresult PLUGIN_API HelloWorldProcessor::setActive (TBool state)
{
    if (state) { mFilterL.reset (); mFilterR.reset (); mFilterDirty = true; }
    return AudioEffect::setActive (state);
}

tresult PLUGIN_API HelloWorldProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
    mSampleRate = newSetup.sampleRate;
    mFilterL.setSampleRate (newSetup.sampleRate);
    mFilterR.setSampleRate (newSetup.sampleRate);
    mFilterDirty = true;
    return AudioEffect::setupProcessing (newSetup);
}

void HelloWorldProcessor::updateFilter ()
{
    const double f = normToFreqHz (mParamFreq);
    const double g = normToGainDb (mParam1);
    const double q = normToQ      (mParamQ);
    mFilterL.setParams (f, g, q);
    mFilterR.setParams (f, g, q);
    mFilterDirty = false;
}

tresult PLUGIN_API HelloWorldProcessor::process (Vst::ProcessData& data)
{
    if (data.inputParameterChanges)
    {
        int32 numParams = data.inputParameterChanges->getParameterCount ();
        for (int32 i = 0; i < numParams; i++)
        {
            Vst::IParamValueQueue* q = data.inputParameterChanges->getParameterData (i);
            if (!q) continue;
            Vst::ParamValue value; int32 offset;
            const int32 nPoints = q->getPointCount ();
            switch (q->getParameterId ())
            {
                case HelloWorldParams::kParamGainId:
                    if (q->getPoint (nPoints-1, offset, value) == kResultTrue)
                    { mParam1 = value; mFilterDirty = true; } break;
                case HelloWorldParams::kParamQId:
                    if (q->getPoint (nPoints-1, offset, value) == kResultTrue)
                    { mParamQ = value; mFilterDirty = true; } break;
                case HelloWorldParams::kParamFreqId:
                    if (q->getPoint (nPoints-1, offset, value) == kResultTrue)
                    { mParamFreq = value; mFilterDirty = true; } break;
                case HelloWorldParams::kParamListenId:
                    if (q->getPoint (nPoints-1, offset, value) == kResultTrue)
                        mParam2 = value > 0.5 ? 1 : 0; break;
                case HelloWorldParams::kBypassId:
                    if (q->getPoint (nPoints-1, offset, value) == kResultTrue)
                        mBypass = (value > 0.5f); break;
            }
        }
    }

    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples == 0)
        return kResultOk;

    const int32  numChannels  = data.inputs[0].numChannels;
    const uint32 sampleFrames = data.numSamples;
    float* inL = data.inputs[0].channelBuffers32[0];

    // Capture dry input for the pre-filter spectrum display
    if (numChannels > 0)
        mSpectrumBufferPre.write (inL, sampleFrames);

    if (mBypass)
    {
        for (int32 ch = 0; ch < numChannels; ch++)
            memcpy (data.outputs[0].channelBuffers32[ch],
                    data.inputs[0].channelBuffers32[ch],
                    sampleFrames * sizeof (float));
        if (numChannels > 0)
            mSpectrumBuffer.write (inL, sampleFrames);
        return kResultOk;
    }

    if (mFilterDirty) updateFilter ();

    float* outL = data.outputs[0].channelBuffers32[0];
    for (uint32 s = 0; s < sampleFrames; s++)
        outL[s] = mFilterL.process (inL[s]);

    if (numChannels > 1)
    {
        float* inR  = data.inputs[0].channelBuffers32[1];
        float* outR = data.outputs[0].channelBuffers32[1];
        for (uint32 s = 0; s < sampleFrames; s++)
            outR[s] = mFilterR.process (inR[s]);
    }

    if (numChannels > 0)
        mSpectrumBuffer.write (outL, sampleFrames);

    return kResultOk;
}

tresult PLUGIN_API HelloWorldProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
    return (symbolicSampleSize == Vst::kSample32) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API HelloWorldProcessor::setState (IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer streamer (state, kLittleEndian);
    float savedGain = 0.5f, savedQ = 0.2f, savedFreq = 0.5f;
    int32 savedListen = 0, savedBypass = 0;
    if (streamer.readFloat (savedGain)   == false) return kResultFalse;
    if (streamer.readFloat (savedQ)      == false) return kResultFalse;
    if (streamer.readFloat (savedFreq)   == false) return kResultFalse;
    if (streamer.readInt32 (savedListen) == false) return kResultFalse;
    if (streamer.readInt32 (savedBypass) == false) return kResultFalse;
    mParam1 = savedGain; mParamQ = savedQ; mParamFreq = savedFreq;
    mParam2 = savedListen > 0 ? 1 : 0;
    mBypass = savedBypass > 0;
    mFilterDirty = true;
    return kResultOk;
}

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

} // namespace Steinberg
