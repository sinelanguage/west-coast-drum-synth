cmake_minimum_required(VERSION 3.22)

project(WestCoastDrumSynth VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_definitions(DEVELOPMENT=1)

set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "Disable plugin links in build tree" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "Enable VSTGUI editor support" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "Disable validator post-build step" FORCE)
set(VSTGUI_STANDALONE OFF CACHE BOOL "Disable VSTGUI standalone targets" FORCE)
set(VSTGUI_STANDALONE_EXAMPLES OFF CACHE BOOL "Disable VSTGUI standalone examples" FORCE)
set(VSTGUI_TOOLS OFF CACHE BOOL "Disable VSTGUI tools" FORCE)
set(VSTGUI_UISCRIPTING OFF CACHE BOOL "Disable VSTGUI scripting features" FORCE)

include(FetchContent)

if(DEFINED ENV{VST3_SDK_ROOT} AND EXISTS "$ENV{VST3_SDK_ROOT}/CMakeLists.txt")
  set(VST3_SDK_SOURCE_DIR "$ENV{VST3_SDK_ROOT}")
  add_subdirectory("$ENV{VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk/CMakeLists.txt")
  set(VST3_SDK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk" "${CMAKE_BINARY_DIR}/vst3sdk")
else()
  FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_SUBMODULES_RECURSE TRUE
  )
  FetchContent_MakeAvailable(vst3sdk)
  set(VST3_SDK_SOURCE_DIR "${vst3sdk_SOURCE_DIR}")
endif()

set(SDK_ROOT "${VST3_SDK_SOURCE_DIR}")
set(public_sdk_SOURCE_DIR "${VST3_SDK_SOURCE_DIR}/public.sdk")
set(pluginterfaces_SOURCE_DIR "${VST3_SDK_SOURCE_DIR}/pluginterfaces")

smtg_add_vst3plugin(WestCoastDrumSynth
  source/factory.cpp
  source/version.h
  source/westcoastdrumcids.h
  source/ParameterIds.h
  source/WestCoastProcessor.h
  source/WestCoastProcessor.cpp
  source/WestCoastController.h
  source/WestCoastController.cpp
  source/engine/DrumVoice.h
  source/engine/DrumVoice.cpp
  source/engine/StepSequencer.h
  source/engine/StepSequencer.cpp
  source/presets/FactoryPresets.h
  source/presets/FactoryPresets.cpp
  resource/WestCoastEditor.uidesc
)

target_include_directories(WestCoastDrumSynth
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

target_link_libraries(WestCoastDrumSynth
  PRIVATE
    sdk
    vstgui_support
)

smtg_target_add_plugin_resources(WestCoastDrumSynth
  RESOURCES
    "resource/WestCoastEditor.uidesc"
)

smtg_target_configure_version_file(WestCoastDrumSynth)
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
