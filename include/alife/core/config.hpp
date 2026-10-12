#pragma once
#include <cstddef>
#include <cstdint>

namespace alife {

// Scalar phenotype genes; executable behavioral circuits have variable length.
inline constexpr std::size_t kGenomeLen = 11;
inline constexpr float kMaxSense = 48.0f;  // sense radius cap, grid cell must be >= this

// Ecology and metabolism. Units: energy per sim second, lengths in world units.
struct EcoParams {
    // resource field: logistic regrowth on a patchy carrying capacity
    int fieldW = 144, fieldH = 96;
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
    float basal = 0.18f;         // x mass^0.75 (Kleiber-style)
    float senseCost = 0.00006f;  // x senseR^2
    float moveCost = 0.0002f;    // x mass x speed^2
    float defenseCost = 0.14f;   // x def x mass^0.75
    float heatStressCost = 0.65f; // x local heat load for organisms without heat tolerance
    float heatToleranceCost = 0.10f; // x invested heat tolerance x mass^0.75
    float bite = 2.7f;           // resource units per second at full appetite
    float energyPerR = 1.0f;
    // predation
    float predEff = 0.55f;
    float digestTime = 12.0f;    // s of handling time after a kill
    float huntBelow = 0.65f;     // hungry predators begin hunting before reserves are critical
    float aggrCost = 0.28f;      // upkeep of weaponry, x aggression x mass^0.75
    float circuitCost = 0.0015f; // upkeep per expressed computational unit, x mass^0.75
    float unitBuildCost = 0.08f; // embodied energy per inherited computational unit
    float carcassPerMass = 8.0f; // finite chemical-energy equivalent stored in body tissue
    float lunge = 1.5f;          // energy per attack attempt
    // life history
    float lifespan = 140.0f;     // s
    float maturity = 14.0f;      // s
    float pathogen = 0.002f;     // host-specific disease: hazard/s = pathogen x (species share)^2
    float vTop = 58.0f;          // top swimming speed, normalizes speed genes
};

struct SimConfig {
    float worldW = 1728.0f;
    float worldH = 1152.0f;
    float cellSize = 48.0f;
    float dtPhys = 0.05f;
    int mesoEvery = 5;      // metabolism, sensing, reproduction (0.25 s)
    int evoEvery = 100;     // macro: speciation bookkeeping, telemetry (5 s)
    std::uint64_t seed = 0xC0FFEEULL;
    std::size_t maxAgents = 12000;
    std::size_t initialAgents = 1800;
    EcoParams eco;
};

}  // namespace alife
