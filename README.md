# moriarty

Artificial-life and digital-evolution experiment in C++20. Organisms compete on a toroidal,
resource-limited world. Their inherited computational circuits can change in length, wiring, and
operation through mutation and recombination; unit duplication can preserve and elaborate an
existing computation, while execution and construction consume energy.
This is a bounded experimental model, not a claim of literal infinite complexity or biological
thermodynamic accuracy.

## Build and run

Needs CMake 3.24+ and a C++20 compiler. raylib and doctest are fetched automatically unless a system
install is found.

```
cmake --preset dev
cmake --build --preset dev --config Release
```

Windows (Visual Studio generator): `build\dev\apps\Release\moriarty_viewer.exe`
Linux and macOS: `./build/dev/apps/moriarty_viewer`

Use a Release or RelWithDebInfo build to watch the sim. Presets: `dev`, `debug`, `release`, `asan`,
`tsan` (no viewer). Other options: `-DALIFE_BUILD_VIEWER=OFF`, `-DALIFE_NATIVE_ARCH=ON`.

```
moriarty_headless 1200 60
moriarty_viewer --warm 300
moriarty_viewer --shot a.png --zoom 4 --frames 120
```

## Reading and controlling the viewer

Zoom out for species clusters; zoom in for individual organisms. Colors encode lineage, brightness
encodes energy, and the species list and food web summarize current ecological interactions. The
world is 1728 x 1152 units; wheel zooms toward the cursor and dragging pans.

| Input | Action |
|---|---|
| left click organism | select and follow it |
| Enter | toggle following |
| U | open/close the selected organism's computational-unit microscope |
| Tab / Shift+Tab, click a species row | focus a species |
| Esc | clear selection and focus |
| drag, wheel | pan and zoom |
| F | fit world |
| Space | pause |
| Up / Down | simulation speed |
| 1 / 2 | color by species / energy |
| W, G, H | toggle wakes, fields, HUD |

## Model

All simulation state and dynamics are in `alife_core`; the viewer renders snapshots of the real
simulation. Runs are deterministic for a given seed.

**Time scales**
- Micro, 0.05 s: Langevin motion, soft-body repulsion, stochastic currents, and wander.
- Meso, 0.25 s: local sensing, steering, resource uptake, predation, metabolic costs, death, and
  reproduction; the environment also advances.
- Macro, 5 s: species bookkeeping and interaction-history decay.

**Heritable substrate.** Organisms inherit 9 scalar parameters and a variable-length,
topologically ordered circuit (`genetics/circuit.hpp`). Each unit combines earlier unit outputs or
local sensory inputs using generic arithmetic/nonlinear operators. Circuit outputs connect to the
body's physical interfaces: motion, uptake, resource allocation, and defense. Unit duplication,
insertion, deletion, rewiring, operator and parameter mutations, and recombination are inherited by
offspring.
There are no hand-authored ecological founder archetypes in the live simulation. The initial
population descends from one minimal four-unit ancestor program with continuous heritable variation;
additional internal computations must arise through mutation or recombination.

Circuits are deterministic bounded DAGs: connections address a sensory input or an earlier unit,
and each organism currently has a maximum of 96 units. Execution adds metabolic upkeep; inherited
units have an embodied construction-energy cost. The microscope lists every unit, its two sources,
operator, parameters, current value, and output interface. The live "mean computational units"
chart measures average circuit length.

The fixed scalar traits and actuator/sensor interfaces remain part of this prototype. They are
deliberate limits on the simulated substrate, not an implementation of an unconstrained computer.
Likewise, arbitrary units of energy are an abstract resource budget, not chemical stoichiometry or a
validated physical thermodynamic model.

**Ecology and selection.**
- Two spatial food fields regrow under variable conditions; organisms consume shared local stocks.
- Upkeep pays for body mass, efficiency, sensing, movement, defense, predation, crowding, and circuit
  size. Resource scarcity can cause starvation; corpses return part of their resources.
- Predation has a handling interval, attack cost, and stochastic outcomes affected by body and
  evolved response.
- Environmental seasons, weather, disturbances, and currents create changing local conditions.
- Reproduction transfers energy and pays offspring construction costs. No fitness score, immigration,
  or automatic population rescue is applied by the macro step.

**Evolution and speciation.** Asexual reproduction or local conspecific recombination transfers
scalar genes and circuits. Scalar mutation rates are heritable; circuit mutations change instruction
content and structure. Species are clusters in both scalar-trait and circuit-architecture space.
Lineages survive, branch, or go extinct under their local ecological outcomes; the simulation does
not guarantee a species boom or monotonic complexity growth.

## Research context and limitations

The implementation draws on, but does not reproduce, these research systems and results:

- Ofria & Wilke (2004), [Avida: A Software Platform for Research in Computational Evolutionary Biology](https://doi.org/10.1162/106454604773563612), *Artificial Life* 10(2):191–229. Avida motivates executable heritable digital organisms and controlled experiments. Task-reward environments are explicit selection design, not proof of unscripted ecology.
- Lenski, Ofria, Pennock & Adami (2003), [The evolutionary origin of complex features](https://doi.org/10.1038/nature01568), *Nature* 423:139–144. Digital organisms evolved coordinated logic functions through heritable instruction changes under defined selection conditions.
- Clark, Hickinbotham & Stepney (2017), [Semantic closure demonstrated by the evolution of a universal constructor architecture in an artificial chemistry](https://doi.org/10.1098/rsif.2016.1033), *Journal of the Royal Society Interface* 14:20161033. Stringmol illustrates recursively encoded copying and construction; this project does not yet implement semantic closure or self-constructing organisms.
- Taylor et al. (2016), [Open-Ended Evolution: Perspectives from the OEE Workshop in York](https://doi.org/10.1162/artl_a_00210), *Artificial Life* 22(3):408–423. OEE has multiple definitions and requires evidence of sustained novelty and complexity, not just a flexible mechanism.

This finite prototype does not prove open-ended evolution, literal infinity, or biological realism.
Circuit length alone is only one complexity proxy. Credible OEE experiments still need replicated
seeds, long runs, archived lineages, resource and neutral controls, and multiple measures of adaptive
novelty and complexity. The deeper recursive-constructor approach described by Stringmol remains
future work.

## Layout

```
include/alife/   public headers
src/             simulation, genetics, math, telemetry, renderer
apps/viewer      raylib frontend
apps/headless_runner
tests/           doctest
```

Targets: `alife_core` (no graphics) -> `alife_telemetry` -> `alife_render`.
