cmake_minimum_required(VERSION 3.24)
project(moriarty VERSION 0.1.0
        DESCRIPTION "Open-ended evolution artificial life engine"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(ALIFE_BUILD_VIEWER   "Build the raylib viewer"        ON)
option(ALIFE_BUILD_HEADLESS "Build the headless runner"      ON)
option(ALIFE_BUILD_TESTS    "Build unit tests"               ON)
option(ALIFE_BUILD_BENCH    "Build benchmarks"               OFF)
option(ALIFE_ENABLE_LTO     "Enable link-time optimization"  OFF)
option(ALIFE_NATIVE_ARCH    "Compile with -march=native"     OFF)
set(ALIFE_SANITIZE "" CACHE STRING "Sanitizers, e.g. address,undefined or thread")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectOptions)
include(Dependencies)

add_subdirectory(src)

if(ALIFE_BUILD_VIEWER OR ALIFE_BUILD_HEADLESS)
  add_subdirectory(apps)
endif()

if(ALIFE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ALIFE_BUILD_BENCH)
  add_subdirectory(bench)
endif()
