#pragma once
#include <cstddef>
#include <cstdint>

namespace alife {

// Genome row: 10 body genes, 4 cues x 3 controller genes, exploration, spare.
// Layout lives in genetics/genome.hpp.
inline constexpr std::size_t kGenomeLen = 24;
inline constexpr float kMaxSense = 48.0f;  // sense radius cap, grid cell must be >= this

// Ecology and metabolism. Units: energy per sim second, lengths in world units.
struct EcoParams {
    // resource field: logistic regrowth on a patchy carrying capacity
    int fieldW = 96, fieldH = 64;
    float regrow = 0.35f;        // 1/s
    float Kmax = 16.0f;          // best-patch capacity per cell
    float recycle = 0.85f;       // share of corpse energy returned to the field
    float seasonAmp = 0.30f;
    float seasonPeriod = 240.0f; // s, phase noise makes it irregular
    // waste heat: diffuses, decays, suppresses local regrowth
    float heatFrac = 0.35f;
    float heatDiffusion = 0.5f;
    float heatDecay = 0.08f;
    float heatCoupling = 0.5f; 
    // metabolism
    float crowdStress = 0.05f;   // basal upkeep multiplier per neighbor within 24 units
    float basal = 0.35f;         // x mass^0.75 (Kleiber-style)
    float senseCost = 0.00015f;  // x senseR^2
    float moveCost = 0.0002f;    // x mass x speed^2
    float defenseCost = 0.25f;   // x def x mass^0.75
    float bite = 1.9f;           // resource units per second at full appetite
    float energyPerR = 1.0f;
    // predation
    float predEff = 0.42f;
    float digestTime = 12.0f;    // s of handling time after a kill
    float huntBelow = 0.65f;     // only hunt when energy is under this fraction of max
    float aggrCost = 0.45f;      // upkeep of weaponry, x aggression x mass^0.75
    float carcassPerMass = 24.0f;
    float lunge = 1.5f;          // energy per attack attempt
    // life history
    float lifespan = 200.0f;     // s
    float maturity = 28.0f;      // s
    float pathogen = 0.05f;      // host-specific disease: hazard/s = pathogen x (species share)^2
    float vTop = 58.0f;          // top swimming speed, normalizes speed genes
};

struct SimConfig {
    float worldW = 1152.0f;
    float worldH = 768.0f; 
    float cellSize = 48.0f;
    float dtPhys = 0.05f;
    int mesoEvery = 5;      // metabolism, sensing, reproduction (0.25 s)
    int evoEvery = 100;     // macro: speciation bookkeeping, telemetry (5 s)
    std::uint64_t seed = 0xC0FFEEULL;
    std::size_t maxAgents = 9000;
    std::size_t initialAgents = 1800;
    EcoParams eco;
};

}  // namespace alife
