include(FetchContent)

# some pinned deps still declare an ancient cmake_minimum_required, CMake 4 rejects that
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Prefer a system install when one exists, otherwise fetch a pinned tag.

if(ALIFE_BUILD_VIEWER)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS 5.5 NAMES raylib)
  FetchContent_MakeAvailable(raylib)
endif()

if(ALIFE_BUILD_TESTS)
  set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS NAMES doctest)
  FetchContent_MakeAvailable(doctest)
endif()

if(ALIFE_BUILD_BENCH)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS NAMES benchmark)
  FetchContent_MakeAvailable(benchmark)
endif()
