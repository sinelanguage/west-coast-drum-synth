#include "version.h"
#include "wcids.h"
#include "processor.h"
#include "controller.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringSubCategory "Instrument|Drum"

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(SineLanguage::WestCoastDrumSynth::kProcessorUID),
    PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Steinberg::Vst::kDistributable,
    stringSubCategory,
    FULL_VERSION_STR,
    kVstVersionString,
    SineLanguage::WestCoastDrumSynth::WCDrumProcessor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(SineLanguage::WestCoastDrumSynth::kControllerUID),
    PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    stringPluginName "Controller",
    0,
    "",
    FULL_VERSION_STR,
    kVstVersionString,
    SineLanguage::WestCoastDrumSynth::WCDrumController::createInstance)

END_FACTORY
