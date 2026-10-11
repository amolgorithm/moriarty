# moriarty

Open-ended evolution artificial life engine in C++20. Organisms live on a toroidal world under an
energy budget, evolve through endogenous selection (no fitness function), and are tracked with
information-theoretic metrics. The raylib viewer runs the real simulation.

## Build and run

Needs CMake 3.24+ and a C++20 compiler. raylib and doctest are fetched automatically unless a system
install is found.

```
cmake --preset dev
cmake --build --preset dev --config Release
```

Windows (Visual Studio generator): `build\dev\apps\Release\moriarty_viewer.exe`
Linux and macOS: `./build/dev/apps/moriarty_viewer`

Use a Release or RelWithDebInfo build to watch the sim. A plain Debug build is many times slower.
Presets: `dev` (RelWithDebInfo), `debug`, `release` (+LTO, benchmarks), `asan`, `tsan` (no viewer).
Other options: `-DALIFE_BUILD_VIEWER=OFF`, `-DALIFE_NATIVE_ARCH=ON`.

```
moriarty_headless 1200 60     # simulate 1200 s, report every 60 s, list surviving species
moriarty_viewer --warm 300    # simulate 300 s first so the charts have history
moriarty_viewer --shot a.png --zoom 4 --frames 120   # render, save a screenshot, exit
```

On Linux raylib needs `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev`.

## Reading the viewer

Zoom out to see species as colored clusters with names. Zoom in and bodies turn into organisms.

- **Color is lineage.** Each species has its own color. Daughter species get a color near their
  parent's, and a name in the same genus, so clades are easy to spot.
- **Shape is genome.** Size, spines (defense), fangs and a streamlined body (aggression and speed),
  number of flagella (speed), eye size (sense radius), cilia on grazers, pigment spots.
- **Fill brightness is energy.** Dim means hungry, bright means well fed.
- **Interactions.** Red ring and a line mark a kill, a small ring in the parent's color marks a birth.
  The panel top right is a live food web: arrows go from hunter to prey, thicker means more recent kills.
- **Underlay.** Sage glow is fine food (small grazers), amber is coarse food (large grazers), rust is
  waste heat. The drifting specks show the stochastic current.

| input | action |
|---|---|
| left click | select organism (inspector shows traits and live steering gains) |
| enter | follow the selected organism |
| tab / shift+tab, click a species row | fly to a species, dim everything else |
| esc | clear selection and focus |
| drag, wheel | pan, zoom toward cursor |
| f | fit world |
| space | pause |
| up / down | sim speed |
| 1 / 2 | color by species / by energy |
| w, g, h | toggle wakes, food and heat underlay, HUD |

Drop a TTF at `assets/fonts/hud.ttf` to replace the default HUD font.

## The model

Everything below is implemented in `alife_core`, headless, deterministic for a given seed.

**Three time scales** (`sim/scheduler.cpp`)
- micro, every 0.05 s: Langevin step per organism. Controller command, an Ornstein-Uhlenbeck wander
  force (colored noise, 2 s correlation), soft body repulsion, white noise, and passive advection by a
  stochastic divergence-free current.
- meso, every 0.25 s: sensing and steering, grazing, predation, upkeep, death, reproduction, and the
  environment update.
- macro, every 5 s: species bookkeeping, predation-matrix decay, recolonization.

**Genome** (24 genes in [0,1], `genetics/genome.hpp`): metabolic efficiency, sense radius, speed,
aggression, reproduction threshold, mutation rate (self-adaptive), body size, defense, two cosmetic
traits, and a Braitenberg-style controller. The controller has four cues (food gradient, prey,
threat, kin), each with a gain, a bias and a hunger slope, so behavior such as fleeing or flocking
is evolvable and readable.

**Energy**
- Upkeep scales with mass^0.75, plus sensing (radius squared), movement (speed squared), armor,
  weaponry and crowding stress. Higher efficiency costs more upkeep, so it is a real trade-off.
- Two food fields with different terrain. Body size decides which one an organism can eat well, which
  favors coexisting small and large grazers.
- Predation has handling time, lunge cost, and success that depends on speed, size and the prey's armor.
- Waste heat diffuses and suppresses regrowth, a simple niche-construction loop. Corpses recycle
  nutrients into the food fields.
- Host-specific disease: mortality rises with the square of a species' share, a kill-the-winner
  mechanism that keeps one species from taking over.

**Noise that is not a clean oscillator**
- Seasons: nominal period with a random walk in phase, so cycles are irregular.
- Resource weather: slow Ornstein-Uhlenbeck drift of carrying capacity.
- Blights and blooms: Poisson shot noise, regional, fading in and out.
- Currents: Fourier modes with OU amplitudes and drifting phases.
- Individual metabolic noise (log-normal) and Brownian jitter in movement.

**Evolution and speciation**
- Reproduction is asexual or, with a nearby conspecific, uniform crossover. Gaussian point mutations
  with rare large jumps, rate set by the organism's own mutation gene.
- Species are clusters in trait space (`genetics/speciation.cpp`). A newborn joins its parent's species
  when close to its exemplar, otherwise the nearest species within a threshold, otherwise it founds a
  new species. Exemplars follow the running mean of the members.
- Roles (grazer, omnivore, predator) are derived from mean aggression, not assigned.
- Immigration: now and then a small group of a locally rare founder lineage arrives from outside, so
  the arena behaves like one patch of a larger metacommunity.

## Layout

```
include/alife/   public headers, one folder per module
src/             implementations, mirrors include/
apps/viewer      raylib frontend
apps/headless_runner
shaders/         GLSL for the phase 6 instanced renderer
tests/ bench/    doctest, Google Benchmark
cmake/           compiler options, dependencies
```

Targets: `alife_core` (no graphics) -> `alife_telemetry` -> `alife_render`.

## Roadmap

| phase | scope |
|---|---|
| 0-3 | scaffold, SoA physics and grid, thermodynamics, genetics (done, single thread) |
| 4 | concurrency: thread pool, phase barriers, async macro worker |
| 5 | lineage log, OEE metrics (activity statistics, MODES) |
| 6 | renderer: instanced agents, GPU effects |
| 7 | transfer entropy, replicator fitting, mean-field, 3D |
