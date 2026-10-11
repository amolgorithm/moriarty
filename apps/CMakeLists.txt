if(ALIFE_BUILD_VIEWER)
  add_executable(moriarty_viewer
    viewer/main.cpp)
  target_link_libraries(moriarty_viewer PRIVATE alife_render alife_options)
endif()

if(ALIFE_BUILD_HEADLESS)
  add_executable(moriarty_headless headless_runner/main.cpp)
  target_link_libraries(moriarty_headless PRIVATE alife_telemetry alife_options)
endif()
