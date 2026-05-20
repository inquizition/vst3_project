//----------------------------------------------------------//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "helloworldcontroller.h"
#include "helloworldcids.h"
#include "helloworldprocessor.h"   // full definition needed for FUnknownPtr cast
#include "Spectrumview.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"

using namespace Steinberg;

namespace Steinberg {

//------------------------------------------------------------------------
// connect() — called by the host to link controller <-> processor.
// We use FUnknownPtr to safely cast the peer to HelloWorldProcessor.
//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::connect (Vst::IConnectionPoint* other)
{
    tresult result = EditControllerEx1::connect (other);

    if (other && result == kResultOk)
    {
        // FUnknownPtr on IComponent is unambiguous — then dynamic_cast to the concrete type
        Steinberg::FUnknownPtr<Steinberg::Vst::IComponent> component (other);
        if (component)
            mProcessor = dynamic_cast<HelloWorldProcessor*> (component.getInterface ());
    }

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::initialize (FUnknown* context)
{
    tresult result = EditControllerEx1::initialize (context);
    if (result != kResultOk)
        return result;

    mAnalyzer = std::make_unique<SpectrumAnalyzer>();

    parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
                             Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
                             HelloWorldParams::kBypassId);

    parameters.addParameter (STR16 ("Parameter 1"), STR16 ("dB"), 0, .5,
                             Vst::ParameterInfo::kCanAutomate, HelloWorldParams::kParamVolId, 0,
                             STR16 ("Param1"));

    parameters.addParameter (STR16 ("Parameter 2"), STR16 ("On/Off"), 1, 1.,
                             Vst::ParameterInfo::kCanAutomate, HelloWorldParams::kParamOnId, 0,
                             STR16 ("Param2"));

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::terminate ()
{
    return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::setComponentState (IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer streamer (state, kLittleEndian);

    float savedParam1 = 0.f;
    if (streamer.readFloat (savedParam1) == false)
        return kResultFalse;
    setParamNormalized (HelloWorldParams::kParamVolId, savedParam1);

    int8 savedParam2 = 0;
    if (streamer.readInt8 (savedParam2) == false)
        return kResultFalse;
    setParamNormalized (HelloWorldParams::kParamOnId, savedParam2);

    int32 bypassState;
    if (streamer.readInt32 (bypassState) == false)
        return kResultFalse;
    setParamNormalized (kBypassId, bypassState ? 1 : 0);

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::setState (IBStream* state)
{
    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::getState (IBStream* state)
{
    return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API HelloWorldController::createView (FIDString name)
{
    if (!FIDStringsEqual (name, Vst::ViewType::kEditor))
        return nullptr;

    // Local subclass of VST3Editor so we can override open()
    // and inject the SpectrumView after the VSTGUI frame exists.
    struct HelloWorldEditor : public VSTGUI::VST3Editor
    {
        HelloWorldController* mCtrl;

        HelloWorldEditor (HelloWorldController* ctrl)
            : VSTGUI::VST3Editor (ctrl, "view", "helloworldeditor.uidesc")
            , mCtrl (ctrl)
        {}

        bool open (void* parent, const VSTGUI::PlatformType& type) override
        {
            if (!VSTGUI::VST3Editor::open (parent, type))
                return false;

            VSTGUI::CRect specRect (10, 130, 490, 260);

            auto* specView = new SpectrumView (specRect,
                [this](float* outMag, int size) -> bool
                {
                    // Drain ring buffer from processor into the analyzer
                    if (mCtrl->mProcessor)
                    {
                        auto& rb = mCtrl->mProcessor->getSpectrumBuffer ();
                        static std::vector<float> tmp (4096);
                        size_t got = rb.read (tmp.data (), tmp.size ());
                        if (got > 0)
                            mCtrl->mAnalyzer->pushSamples (tmp.data (), (int)got);
                    }
                    return mCtrl->mAnalyzer->getLatestMagnitudes (outMag, size);
                }
            );

            getFrame ()->addView (specView);
            return true;
        }
    };

    return new HelloWorldEditor (this);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
    return EditControllerEx1::setParamNormalized (tag, value);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
    return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace Steinberg
