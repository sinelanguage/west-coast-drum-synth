cmake_minimum_required(VERSION 3.28.3)
project(CheckOBJCXX OBJCXX)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/result.cmake"
  "set(CMAKE_OBJCXX_COMPILER \"${CMAKE_OBJCXX_COMPILER}\")\n"
  
  )