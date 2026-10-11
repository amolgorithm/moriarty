# One interface target carries language level, warnings, sanitizers and fp flags.
# Every first-party target links it, third-party deps never see these flags.
add_library(alife_options INTERFACE)
target_compile_features(alife_options INTERFACE cxx_std_20)

if(MSVC)
  target_compile_options(alife_options INTERFACE /W4 /permissive- /utf-8)
else()
  target_compile_options(alife_options INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
    # keep results bit-identical across builds, no fused multiply-add surprises
    -ffp-contract=off)
  if(ALIFE_NATIVE_ARCH)
    target_compile_options(alife_options INTERFACE -march=native)
  endif()
endif()

if(ALIFE_SANITIZE AND NOT MSVC)
  target_compile_options(alife_options INTERFACE
    -fsanitize=${ALIFE_SANITIZE} -fno-omit-frame-pointer)
  target_link_options(alife_options INTERFACE -fsanitize=${ALIFE_SANITIZE})
endif()

if(ALIFE_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT alife_ipo_ok OUTPUT alife_ipo_msg)
  if(alife_ipo_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "LTO requested but unsupported: ${alife_ipo_msg}")
  endif()
endif()
