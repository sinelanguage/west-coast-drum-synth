# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-src")
  file(MAKE_DIRECTORY "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-src")
endif()
file(MAKE_DIRECTORY
  "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-build"
  "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix"
  "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/tmp"
  "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp"
  "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src"
  "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/sinelanguage/dev/2026/ai/west-coast-drum-synth/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
