cmake_minimum_required(VERSION 3.22)

project(WestCoastDrumSynth VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_definitions(DEVELOPMENT=1)

# VSTGUI pulls Objective-C++ sources on macOS. Explicitly enable ObjC/ObjC++
# so CMake defines the required CMAKE_OBJCXX_* internals for generators.
if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

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
