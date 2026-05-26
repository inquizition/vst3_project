//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "helloworldcontroller.h"
#include "helloworldcids.h"
#include "helloworldprocessor.h"
#include "Spectrumview.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

#include <vector>

using namespace Steinberg;

namespace Steinberg {

tresult PLUGIN_API HelloWorldController::connect (Vst::IConnectionPoint* other)
{
    tresult result = EditControllerEx1::connect (other);
    if (other && result == kResultOk)
    {
        Steinberg::FUnknownPtr<Steinberg::Vst::IComponent> component (other);
        if (component)
            mProcessor = dynamic_cast<HelloWorldProcessor*> (component.getInterface ());
    }
    return result;
}

tresult PLUGIN_API HelloWorldController::initialize (FUnknown* context)
{
    tresult result = EditControllerEx1::initialize (context);
    if (result != kResultOk) return result;

    mAnalyzer    = std::make_unique<SpectrumAnalyzer>();
    mAnalyzerPre = std::make_unique<SpectrumAnalyzer>();

    parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
        Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass, kBypassId);
    parameters.addParameter (STR16 ("Gain"),   STR16 ("dB"),     0, 0.5,
        Vst::ParameterInfo::kCanAutomate, kParamGainId,   0, STR16 ("Gain"));
    parameters.addParameter (STR16 ("Q"),      nullptr,          0, 0.2,
        Vst::ParameterInfo::kCanAutomate, kParamQId,      0, STR16 ("Q"));
    parameters.addParameter (STR16 ("Freq"),   STR16 ("Hz"),     0, 0.5,
        Vst::ParameterInfo::kCanAutomate, kParamFreqId,   0, STR16 ("Freq"));
    parameters.addParameter (STR16 ("Listen"), STR16 ("On/Off"), 1, 0,
        Vst::ParameterInfo::kCanAutomate, kParamListenId, 0, STR16 ("Listen"));

    return result;
}

tresult PLUGIN_API HelloWorldController::terminate ()
{
    return EditControllerEx1::terminate ();
}

tresult PLUGIN_API HelloWorldController::setComponentState (IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer streamer (state, kLittleEndian);
    float gainNorm = 0.5f, qNorm = 0.2f, freqNorm = 0.5f;
    int32 listenState = 0, bypassState = 0;
    if (streamer.readFloat (gainNorm)    == false) return kResultFalse;
    if (streamer.readFloat (qNorm)       == false) return kResultFalse;
    if (streamer.readFloat (freqNorm)    == false) return kResultFalse;
    if (streamer.readInt32 (listenState) == false) return kResultFalse;
    if (streamer.readInt32 (bypassState) == false) return kResultFalse;
    setParamNormalized (kParamGainId,   gainNorm);
    setParamNormalized (kParamQId,      qNorm);
    setParamNormalized (kParamFreqId,   freqNorm);
    setParamNormalized (kParamListenId, listenState ? 1.0 : 0.0);
    setParamNormalized (kBypassId,      bypassState ? 1.0 : 0.0);
    return kResultOk;
}

tresult PLUGIN_API HelloWorldController::setState (IBStream* state) { return kResultTrue; }
tresult PLUGIN_API HelloWorldController::getState (IBStream* state) { return kResultTrue; }

IPlugView* PLUGIN_API HelloWorldController::createView (FIDString name)
{
    if (!FIDStringsEqual (name, Vst::ViewType::kEditor))
        return nullptr;

    struct HelloWorldEditor : public VSTGUI::VST3Editor
    {
        HelloWorldController* mCtrl;

        HelloWorldEditor (HelloWorldController* ctrl)
            : VSTGUI::VST3Editor (ctrl, "view", "helloworldeditor.uidesc"), mCtrl (ctrl) {}

        bool open (void* parent, const VSTGUI::PlatformType& type) override
        {
            if (!VSTGUI::VST3Editor::open (parent, type))
                return false;

            VSTGUI::CRect specRect (30, 68, 870, 336);

            auto* specView = new SpectrumView (specRect,
                // Post-filter (wet) callback
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
                },
                // Pre-filter (dry) callback
                [this](float* outMag, int size) -> bool
                {
                    if (mCtrl->mProcessor)
                    {
                        auto& rb = mCtrl->mProcessor->getSpectrumBufferPre ();
                        static std::vector<float> tmp (4096);
                        size_t got = rb.read (tmp.data (), tmp.size ());
                        if (got > 0)
                            mCtrl->mAnalyzerPre->pushSamples (tmp.data (), (int)got);
                    }
                    return mCtrl->mAnalyzerPre->getLatestMagnitudes (outMag, size);
                }
            );

            specView->setFreqCallbacks ({
                [this]()        { mCtrl->beginEdit (kParamFreqId); },
                [this](float f) {
                    mCtrl->setParamNormalized (kParamFreqId, f);
                    mCtrl->performEdit        (kParamFreqId, f);
                },
                [this]()        { mCtrl->endEdit (kParamFreqId); }
            });

            specView->setFreqNorm ((float)mCtrl->getParamNormalized (kParamFreqId));
            getFrame ()->addView (specView);
            return true;
        }
    };

    return new HelloWorldEditor (this);
}

tresult PLUGIN_API HelloWorldController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
    return EditControllerEx1::setParamNormalized (tag, value);
}

tresult PLUGIN_API HelloWorldController::getParamStringByValue (Vst::ParamID tag,
    Vst::ParamValue valueNormalized, Vst::String128 string)
{
    return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

tresult PLUGIN_API HelloWorldController::getParamValueByString (Vst::ParamID tag,
    Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

} // namespace Steinberg
