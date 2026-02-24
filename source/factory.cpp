#include "WestCoastController.h"
#include "WestCoastProcessor.h"
#include "version.h"
#include "westcoastdrumcids.h"

#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace Steinberg::WestCoastDrumSynth;

BEGIN_FACTORY_DEF (stringCompanyName, stringCompanyWeb, stringCompanyEmail)

DEF_CLASS2 (INLINE_UID_FROM_FUID (kProcessorUID), PClassInfo::kManyInstances, kVstAudioEffectClass, stringPluginName,
            Vst::kDistributable, "Instrument|Synth|Drum", FULL_VERSION_STR, kVstVersionString,
            WestCoastProcessor::createInstance)

DEF_CLASS2 (INLINE_UID_FROM_FUID (kControllerUID), PClassInfo::kManyInstances, kVstComponentControllerClass,
            stringPluginName "Controller", 0, "", FULL_VERSION_STR, kVstVersionString,
            WestCoastController::createInstance)

END_FACTORY
