# moriarty

Artificial-life and digital-evolution experiment in C++20. Organisms compete on a toroidal,
resource-limited world. Their inherited computational circuits can change in length, wiring, and
operation through mutation and recombination; duplication, deletion, recurrent memory, and
reward-modulated subunit plasticity make their structure behaviorally active, while execution and
construction consume energy.
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
moriarty_headless 240 30 12345 0.35
moriarty_viewer --warm 300
moriarty_viewer --shot a.png --zoom 4 --frames 120
```

The headless runner accepts an optional seed and resource-regrowth rate after the reporting interval,
so matched-seed ecology experiments can be repeated rather than judging evolution from one species
count.

## Reading and controlling the viewer

Zoom out for species clusters; zoom in for individual organisms. Colors encode lineage, brightness
encodes energy, and the species list and food web summarize current ecological interactions. The
species count reports extant taxa with at least 8 organisms; the list includes every extant taxon,
shows mean output-connected units over total units, and scrolls with the wheel while hovered. The
field legend describes the two abstract food-particle fields and waste heat. The finite toroidal
world is 2592 x 1728 units; its 216 x 144 ecological grid preserves the previous spatial resolution
and its larger founder population preserves approximately the previous organism density. Wheel zooms
toward the cursor outside the species list and dragging pans.

| Input | Action |
|---|---|
| left click organism | select and follow it |
| Enter | toggle following |
| U | open/close the selected organism's computational-unit microscope |
| Tab / Shift+Tab, click a species row | focus a species |
| Esc | clear selection and focus |
| drag, wheel | pan and zoom (wheel scrolls the species list or subunit microscope when hovered) |
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

For accelerated evolutionary experiments, organisms mature at 6 simulated seconds and have a nominal
lifespan of 90 seconds. The shorter turnover, high mutation supply, and rapidly changing conditions
are experimental accelerations, not biological predictions or evidence that complexity must rise.

**Heritable substrate.** Organisms inherit 14 scalar parameters and a variable-length,
topologically ordered circuit (`genetics/circuit.hpp`). Each unit combines earlier unit outputs or
local sensory inputs using generic arithmetic/nonlinear operators. Circuit outputs connect to the
body's physical interfaces: motion, uptake, resource allocation, and defense. Unit duplication,
insertion, deletion, rewiring, operator and parameter mutations, and recombination are inherited by
offspring. The self-adaptive per-gene mutation scale spans 0.012-0.18; each offspring has a 0.38-0.58
chance of one structural circuit edit. These deliberately high rates make change visible on
interactive timescales, but can also impose substantial mutational load. There are no hand-authored
ecological founder archetypes in the live simulation. The
initial population descends from one minimal four-unit ancestor program with continuous
heritable variation and begins in one species. New species can branch only as descendants accumulate
trait or program divergence; starting genetic variation is not misreported as thousands of species.

Circuits are bounded programs with feed-forward connections and recurrent memory units; each
organism currently has a maximum of 256 units. Nineteen normalized sensory channels include food,
prey value, threat urgency, energy, currents, conspecific density/alignment, local heat, food
abundance, and age. Eleven unit operations include arithmetic, memory, and evolved comparison
instructions (`greater-than`, `less-than`), allowing conditional responses to combine cues. Eight
outputs influence steering, resource uptake, food preference, defense, social cohesion, hunting, and
reproductive strategy. The reproduction output trades the parent's reproduction threshold and
offspring reserve against offspring size; it only matters insofar as the resulting lineage survives
and reproduces. The selected-organism inspector exposes these output values plus the organism's diet
affinity, heat tolerance, immune investment, repair investment, and digestive yield.
Diet-affinity genes affect which food field an organism tracks and consumes; heat-tolerance genes
trade extra upkeep in cool conditions for lower metabolic penalties in locally heated microhabitats.
Immunity lowers density-dependent disease mortality but consumes maintenance energy. Tissue repair
reduces age-related mortality at an upkeep cost. Digestive investment raises the fraction of food and
prey energy assimilated, while also costing energy to maintain. These trade-offs expose heritable
adaptations to selection instead of granting fitness directly. Waste heat is produced by organisms,
diffuses, and reduces resource renewal, coupling density, physiology, and food availability. Each
unit carries a heritable plasticity rate; an eligibility trace is adjusted by recent net energy change, changing that
organism's effective unit weights during its lifetime. The inherited program sets the learning rate,
not a scripted skill; offspring begin from inherited weights and learn anew. Execution adds metabolic
upkeep; inherited units have a construction-energy cost. Unit duplication initially preserves the
copied unit's output connection, so redundancy can precede mutation-driven divergence. The
microscope shows current learned weights, plasticity, recurrent state, and output interface. Species
rows show mean output-connected units over total units; the live chart tracks output-connected count.
These are structural measures, not proof every connected computation is beneficial. The headless
summary also reports comparison and memory units.

The fixed scalar traits and actuator/sensor interfaces remain part of this prototype. They are
deliberate limits on the simulated substrate, not an implementation of an unconstrained computer.
Likewise, arbitrary units of energy are an abstract resource budget, not chemical stoichiometry or a
validated physical thermodynamic model.

**Ecology and selection.**
- Two abstract spatial food fields regrow under variable conditions: fine suspended particles favor
  smaller bodies and coarse particles favor larger bodies, with body size, inherited diet affinity,
  and circuit preference affecting
  intake. Grazing removes finite local stock; it is not a named plant species or infinite passive
  energy. Predation bites transfer the prey's reserve and finite embodied tissue-energy; offspring
  tissue must be paid for by the parent.
- Survival behavior has an energy-dependent action bias: hungry individuals move up food gradients,
  seek vulnerable smaller prey when aggression makes hunting worthwhile, and move away from nearby
  threats. Their inherited circuit steers and modifies these responses; colored-noise exploration
  perturbs, rather than replaces, directed movement. Evolved cohesion outputs can draw conspecifics
  together or separate them; grouping increases defense against attacks but also concentrates
  competition and crowding costs. Aggressive, substantially larger individuals can also hunt smaller
  conspecifics, while extreme size differences permit desperate cannibalism at any aggression level.
- Upkeep pays for body mass, efficiency, sensing, movement, defense, predation, crowding, circuit
  size, immunity, tissue repair, and digestive machinery. Resource regrowth is slightly slower,
  crowding and disease pressure are stronger, maturity and lifespan are shorter, and localized blights/blooms turn over
  more rapidly. Failure to meet upkeep causes starvation; corpses return part of their resources.
- Predation has a handling interval, attack cost, and stochastic outcomes affected by body and
  evolved response. The interaction graph prioritizes recent hunter/prey links, including
  size-asymmetric intraspecific predation; selected organisms show cumulative food-field and prey
  energy intake since birth, while the world panel reports total energy obtained through both
  channels.
- Energy accounting includes both food fields, organism reserves, and waste heat. Resource regrowth
  is explicit external energy input; unused assimilation, maintenance, construction, unrecycled
  remains, and heat decay are recorded as dissipated energy. Predation transfers the prey's finite
  stored reserve and embodied tissue energy (not an unaccounted body-mass bonus). The
  displayed/headless balance residual is
  inventory plus dissipated energy minus initial inventory and external input; it should stay near
  zero apart from floating-point rounding. This is a conserved abstract ledger, not a detailed
  physical energy/biomass model.
- Environmental seasons, weather, disturbances, and currents now fluctuate more rapidly and strongly.
  Food regeneration and heat dissipation remain explicit ledger terms: stochastic changes do not
  inject unaccounted organism energy, and all field growth is recorded as external input while heat
  decay and metabolic losses are recorded as dissipation.
- The diversity chart is Shannon entropy of extant species abundance. It can rise rapidly as mutation
  and lineage splitting increase, then fall during resource bottlenecks and extinctions; neither it
  nor the species count is expected to increase monotonically.
- Reproduction transfers energy and pays offspring construction costs. No fitness score, immigration,
  or automatic population rescue is applied by the macro step.
- The headless runner reports mean generation depth alongside population, established taxa, total
  and output-connected circuit units, feeding source, and energy-balance residual. Use several seeds
  and compare those outcomes with resource conditions; a rise in taxa or circuit length alone is not
  evidence of adaptation.

**Evolution and speciation.** Asexual reproduction or local conspecific recombination transfers
scalar genes and circuits. Scalar mutation rates are heritable; circuit mutations change instruction
content and structure. New traits are selected only through their effects on energy balance,
survival, and reproduction; no reward is assigned to genome or circuit size. Species are clusters in
scalar-trait and circuit-architecture space.
Lineages survive, branch, or go extinct under their local ecological outcomes; the simulation does
not guarantee a species boom or monotonic complexity growth.

## Research context and limitations

The implementation draws on, but does not reproduce, these research systems and results:

- Ofria & Wilke (2004), [Avida: A Software Platform for Research in Computational Evolutionary Biology](https://doi.org/10.1162/106454604773563612), *Artificial Life* 10(2):191–229. Avida motivates executable heritable digital organisms and controlled experiments. Task-reward environments are explicit selection design, not proof of unscripted ecology.
- Lenski, Ofria, Pennock & Adami (2003), [The evolutionary origin of complex features](https://doi.org/10.1038/nature01568), *Nature* 423:139–144. Digital organisms evolved coordinated logic functions through heritable instruction changes under defined selection conditions.
- Stanley & Miikkulainen (2002), [Evolving neural networks through augmenting topologies](https://doi.org/10.1162/106365602320169811), *Evolutionary Computation* 10(2):99–127. NEAT uses historical markings and fitness-sharing speciation to protect structural innovations; Moriarty's species are currently lineage clusters, not NEAT-style protected niches.
- Clark, Hickinbotham & Stepney (2017), [Semantic closure demonstrated by the evolution of a universal constructor architecture in an artificial chemistry](https://doi.org/10.1098/rsif.2016.1033), *Journal of the Royal Society Interface* 14:20161033. Stringmol illustrates recursively encoded copying and construction; this project does not yet implement semantic closure or self-constructing organisms.
- Taylor et al. (2016), [Open-Ended Evolution: Perspectives from the OEE Workshop in York](https://doi.org/10.1162/artl_a_00210), *Artificial Life* 22(3):408–423. OEE has multiple definitions and requires evidence of sustained novelty and complexity, not just a flexible mechanism.

Structural mutation does not systematically favor insertion over deletion, and circuit size has an
execution/upkeep and offspring-construction cost; there is no direct complexity reward. Independent
organism and field updates use a persistent worker pool. This finite prototype does not prove
open-ended evolution, literal infinity, or biological realism. Output-connected circuit length is
only a structural proxy, not proof of adaptive function. Credible OEE experiments still need replicated
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
