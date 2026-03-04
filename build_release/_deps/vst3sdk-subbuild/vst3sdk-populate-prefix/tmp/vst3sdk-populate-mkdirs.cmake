# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/workspace/build_release/_deps/vst3sdk-src"
  "/workspace/build_release/_deps/vst3sdk-build"
  "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix"
  "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/tmp"
  "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp"
  "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src"
  "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspace/build_release/_deps/vst3sdk-subbuild/vst3sdk-populate-prefix/src/vst3sdk-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
