cmake_minimum_required(VERSION 3.14.0)

set(vst3sdk_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vst3sdk")

if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(WestCoastDrumSynth
    VERSION 1.0.0
    DESCRIPTION "West Coast Drum Synthesizer"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(WestCoastDrumSynth
    source/version.h
    source/wcids.h
    source/wcparams.h
    source/dsp/WaveFolder.h
    source/dsp/WaveFolder.cpp
    source/dsp/LowPassGate.h
    source/dsp/LowPassGate.cpp
    source/dsp/EnvelopeGenerator.h
    source/dsp/EnvelopeGenerator.cpp
    source/dsp/FMOperator.h
    source/dsp/FMOperator.cpp
    source/dsp/NoiseGenerator.h
    source/dsp/NoiseGenerator.cpp
    source/dsp/DrumVoice.h
    source/dsp/DrumVoice.cpp
    source/sequencer/StepSequencer.h
    source/sequencer/StepSequencer.cpp
    source/presets.h
    source/processor.h
    source/processor.cpp
    source/controller.h
    source/controller.cpp
    source/entry.cpp
    source/ui/WCDrumView.h
    source/ui/WCDrumView.cpp
)

smtg_target_add_plugin_resources(WestCoastDrumSynth
    RESOURCES
        resource/wcdrumsynth.uidesc
)

target_link_libraries(WestCoastDrumSynth
    PRIVATE
        sdk
        vstgui_support
)

target_include_directories(WestCoastDrumSynth
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

smtg_target_configure_version_file(WestCoastDrumSynth)

if(SMTG_MAC)
    smtg_target_set_bundle(WestCoastDrumSynth
        BUNDLE_IDENTIFIER "com.sinelanguage.westcoastdrumsynth"
        COMPANY_NAME "Sine Language"
    )
    smtg_target_set_debug_executable(WestCoastDrumSynth
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(WestCoastDrumSynth PRIVATE
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT WestCoastDrumSynth)
        smtg_target_set_debug_executable(WestCoastDrumSynth
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
