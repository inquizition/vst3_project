//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "Spectrumanalyzer.h"
#include <memory>

namespace Steinberg {

class HelloWorldProcessor;

class HelloWorldController : public Steinberg::Vst::EditControllerEx1
{
public:
    HelloWorldController () = default;
    ~HelloWorldController () SMTG_OVERRIDE = default;

    static Steinberg::FUnknown* createInstance (void* /*context*/)
    {
        return (Steinberg::Vst::IEditController*)new HelloWorldController;
    }

    Steinberg::tresult PLUGIN_API connect            (Steinberg::Vst::IConnectionPoint* other) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API initialize         (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate          () SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState  (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView      (Steinberg::FIDString name) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState           (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState           (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag,
                                                      Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::ParamValue valueNormalized,
                                                         Steinberg::Vst::String128 string) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::TChar* string,
                                                         Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

    DEFINE_INTERFACES
    END_DEFINE_INTERFACES (EditController)
    DELEGATE_REFCOUNT (EditController)

protected:
    std::unique_ptr<SpectrumAnalyzer> mAnalyzer;     // post-filter
    std::unique_ptr<SpectrumAnalyzer> mAnalyzerPre;  // pre-filter
    HelloWorldProcessor* mProcessor { nullptr };
};

} // namespace Steinberg
