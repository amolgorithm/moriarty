# alife_core: sim state, genetics, math, concurrency. No graphics, no I/O.
add_library(alife_core STATIC
  core/agent_pool.cpp
  core/grid.cpp
  core/world.cpp
  sim/commands.cpp
  sim/environment.cpp
  sim/micro.cpp
  sim/meso.cpp
  sim/evo.cpp
  sim/scheduler.cpp
  genetics/archetypes.cpp
  genetics/gpm.cpp
  genetics/operators.cpp
  genetics/speciation.cpp
  genetics/lineage.cpp
  math/replicator.cpp
  concurrency/thread_pool.cpp)
add_library(moriarty::core ALIAS alife_core)
target_include_directories(alife_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
find_package(Threads REQUIRED)
target_link_libraries(alife_core PUBLIC alife_options Threads::Threads)

# alife_telemetry: metric history, OEE statistics
add_library(alife_telemetry STATIC
  telemetry/metrics.cpp
  telemetry/collect.cpp
  telemetry/oee.cpp)
add_library(moriarty::telemetry ALIAS alife_telemetry)
target_link_libraries(alife_telemetry PUBLIC alife_core)

# alife_render: raylib frontend
if(ALIFE_BUILD_VIEWER)
  add_library(alife_render STATIC
    render/camera.cpp
    render/hud.cpp
    render/snapshot_builder.cpp
    render/renderer.cpp)
  add_library(moriarty::render ALIAS alife_render)
  target_link_libraries(alife_render PUBLIC alife_telemetry raylib)
  target_compile_definitions(alife_render PRIVATE
    ALIFE_SHADER_DIR="${PROJECT_SOURCE_DIR}/shaders"
    ALIFE_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")
endif()
