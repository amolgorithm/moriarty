# moriarty

Open-ended evolution artificial life engine in C++20. Organisms live on a toroidal grid under an
energy budget, evolve through endogenous selection (no fitness function), and are tracked with
information-theoretic and open-endedness metrics.

Status: the raylib viewer runs on a synthetic data source. The sim core is scaffolded and being
built phase by phase, see the roadmap below.

## Build

Needs CMake 3.24+ and a C++20 compiler. raylib, doctest and Google Benchmark are fetched
automatically unless a system install is found.

```
cmake --preset dev
cmake --build --preset dev
./build/dev/apps/moriarty_viewer
ctest --preset dev
```

Presets: `dev` (Debug), `release` (Release + LTO + benchmarks), `asan` (ASan + UBSan), `tsan`
(TSan, viewer off). Useful options: `-DALIFE_BUILD_VIEWER=OFF` for a headless-only build,
`-DALIFE_NATIVE_ARCH=ON`.

On Linux, raylib needs the X11 and GL dev packages (`libx11-dev libxrandr-dev libxinerama-dev
libxcursor-dev libxi-dev libgl-dev`).

## Viewer controls

| key | action |
|---|---|
| space | pause |
| 1 / 2 | color by species / energy |
| t | toggle trails |
| h | toggle HUD |
| f or r | fit world to window |
| up / down | sim speed |
| drag, wheel | pan, zoom toward cursor |

Drop a TTF at `assets/fonts/hud.ttf` to replace the default HUD font.

## Layout

```
include/alife/   public headers, one folder per module
src/             implementations, mirrors include/
apps/viewer      raylib frontend + demo data source
apps/headless_runner
shaders/         GLSL for the phase 6 instanced renderer
tests/ bench/    doctest, Google Benchmark
cmake/           compiler options, dependencies
```

Targets: `alife_core` (no graphics) -> `alife_telemetry` -> `alife_render`.

## Roadmap

| phase | scope |
|---|---|
| 0 | scaffold, CMake, tests, deterministic RNG, viewer on demo data (done) |
| 1 | physics: SoA pool, toroidal grid, Euler-Maruyama integrator, single thread |
| 2 | thermodynamics: resource and heat fields, upkeep, death |
| 3 | genetics: genome, genotype-phenotype map, controller, mutation |
| 4 | concurrency: thread pool, phase barriers, async evo worker |
| 5 | speciation, lineage log, entropy and OEE metrics |
| 6 | renderer polish: instanced agents, GPU trails, field overlays |
| 7 | transfer entropy, replicator fitting, mean-field, 3D |
