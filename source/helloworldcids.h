//------------------------------------------------------------------------
// helloworldcids.h  — Frequency Tamer parameter IDs
// Drop-in replacement for source/helloworldcids.h
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Steinberg {

//------------------------------------------------------------------------
enum HelloWorldParams : Vst::ParamID
{
    kBypassId       = 100,

    kParamGainId    = 102,  // dB, -18 .. +18, default 0
    kParamQId       = 103,  // Q,   0.1 .. 10,  default 1.0
    kParamFreqId    = 104,  // Hz,  20  .. 20000, log, default 1000
    kParamListenId  = 1000  // bool: solo the band
};

//------------------------------------------------------------------------
static const Steinberg::FUID kHelloWorldProcessorUID  (0x32C50013, 0xFF5F5CB4, 0x871C312D, 0xB4F42368);
static const Steinberg::FUID kHelloWorldControllerUID (0xAE34DD83, 0x308259DF, 0xA0D88E2F, 0xB1C1CB8B);

#define HelloWorldVST3Category "Fx"

//------------------------------------------------------------------------
} // namespace Steinberg