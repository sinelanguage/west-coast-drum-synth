cmake_minimum_required(VERSION 3.22)

project(WestCoastDrumSynth VERSION 0.1.0 LANGUAGES C CXX)

# Default to Release so the real plugin GUI shows in DAWs. Debug builds enable VSTGUI
# live-editing mode (black screen + tiny "e" button) instead of the actual interface.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# macOS build controls:
# - default to native Apple Silicon (arm64) when CMAKE_OSX_ARCHITECTURES is unset
# - allow universal binaries via WCSD_MAC_UNIVERSAL=ON
option(WCSD_MAC_UNIVERSAL "Build macOS universal binary (arm64;x86_64)" OFF)
set(WCSD_MAC_ARCHITECTURES "arm64" CACHE STRING "macOS architectures when not universal")

if(APPLE)
  if(NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
    if(WCSD_MAC_UNIVERSAL)
      set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS target architectures" FORCE)
    else()
      set(CMAKE_OSX_ARCHITECTURES "${WCSD_MAC_ARCHITECTURES}" CACHE STRING "macOS target architectures" FORCE)
    endif()
  endif()

  if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_definitions(DEVELOPMENT=1)

# VSTGUI pulls Objective-C++ sources on Apple platforms. Use language probing
# so the configure remains portable and still defines CMAKE_OBJCXX_* rules
# whenever ObjC/ObjC++ toolchains are available.
include(CheckLanguage)
check_language(OBJC)
if(CMAKE_OBJC_COMPILER)
  enable_language(OBJC)
endif()

check_language(OBJCXX)
if(CMAKE_OBJCXX_COMPILER)
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

# AppleClang 17+ warns on an unused local in VSTGUI's cgbitmap.cpp.
# The VST3 SDK promotes warnings to errors, so relax only this warning for
# the third-party VSTGUI target to keep project sources strict.
if(APPLE AND TARGET vstgui)
  target_compile_options(vstgui PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=unused-variable>
    $<$<COMPILE_LANGUAGE:OBJCXX>:-Wno-error=unused-variable>
  )
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
  source/ui/WCDrumView.cpp
  source/ui/ArturiaSlider.cpp
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
    "resource/knob.png"
)

smtg_target_configure_version_file(WestCoastDrumSynth)
