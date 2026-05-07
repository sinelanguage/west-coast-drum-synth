#pragma once

// DAW-visible name (Bitwig browser, etc.). Debug vs Release builds use different
// strings so you can tell which binary is installed. Set -DWCDS_PUBLIC_PLUGIN_NAME=ON
// for a neutral shipping name without (Debug)/(Release).
#if defined(WCDS_PUBLIC_PLUGIN_NAME)
#define stringPluginName "West Coast Drum Synth"
#elif defined(NDEBUG)
#define stringPluginName "West Coast Drum Synth (Release)"
#else
#define stringPluginName "West Coast Drum Synth (Debug)"
#endif
#define stringCompanyName "Sine Language"
#define stringCompanyWeb "https://sinelanguage.com"
#define stringCompanyEmail "info@sinelanguage.com"

#define stringOriginalFilename "WestCoastDrumSynth.vst3"
#define stringFileDescription "West Coast Synthesis Drum Machine"
#define stringLegalCopyright "(c) 2026 Sine Language"

#define VERSION_STR "1.1.0"
#define FULL_VERSION_STR VERSION_STR
