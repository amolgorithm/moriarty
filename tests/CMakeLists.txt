add_executable(alife_tests
  test_main.cpp
  test_rng.cpp
  test_grid.cpp
  test_agent_pool.cpp
  test_entropy.cpp
  test_ring_buffer.cpp)
target_link_libraries(alife_tests PRIVATE alife_telemetry doctest::doctest alife_options)
add_test(NAME alife_tests COMMAND alife_tests)
