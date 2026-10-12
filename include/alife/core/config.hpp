#pragma once
#include <cstddef>
#include <cstdint>

namespace alife {

// Scalar phenotype genes; executable programs have variable length.
inline constexpr std::size_t kGenomeLen = 14;
inline constexpr float kMaxSense = 48.0f;  // sense radius cap, grid cell must be >= this

// Ecology and metabolism. Units: energy per sim second, lengths in world units.
struct EcoParams {
    // resource field: logistic regrowth on a patchy carrying capacity
    int fieldW = 216, fieldH = 144;
    float regrow = 0.33f;        // 1/s
    float Kmax = 16.0f;          // best-patch capacity per cell
    float recycle = 0.85f;       // share of corpse energy returned to the field
    float seasonAmp = 0.38f;
    float seasonPeriod = 140.0f; // s, phase noise makes it irregular
    float weatherTau = 28.0f;
    float weatherSigma = 0.17f;
    float currentTau = 32.0f;
    float currentPhaseNoise = 0.06f;
    float disturbanceInterval = 45.0f;
    float blightFactor = 0.12f;
    float bloomFactor = 2.2f;
    float disturbanceMinDuration = 50.0f;
    float disturbanceDurationRange = 75.0f;
    // waste heat: diffuses, decays, suppresses local regrowth
    float heatFrac = 0.40f;
    float heatDiffusion = 0.55f;
    float heatDecay = 0.11f;
    float heatCoupling = 0.5f; 
    // metabolism
    float crowdStress = 0.055f;  // basal upkeep multiplier per neighbor within 24 units
    float basal = 0.195f;        // x mass^0.75 (Kleiber-style)
    float senseCost = 0.00006f;  // x senseR^2
    float moveCost = 0.0002f;    // x mass x speed^2
    float defenseCost = 0.14f;   // x def x mass^0.75
    float heatStressCost = 0.65f; // x local heat load for organisms without heat tolerance
    float heatToleranceCost = 0.10f; // x invested heat tolerance x mass^0.75
    float immunityCost = 0.06f;  // x immune investment x mass^0.75
    float repairCost = 0.06f;    // x tissue maintenance investment x mass^0.75
    float digestionCost = 0.03f; // x digestive investment x mass^0.75
    float bite = 2.7f;           // resource units per second at full appetite
    float energyPerR = 1.0f;
    // predation
    float predEff = 0.55f;
    float digestTime = 12.0f;    // s of handling time after a kill
    float huntBelow = 0.65f;     // hungry predators begin hunting before reserves are critical
    float aggrCost = 0.28f;      // upkeep of weaponry, x aggression x mass^0.75
    float instructionCost = 0.00008f; // upkeep per executed instruction, x mass^0.75
    float instructionBuildCost = 0.006f; // embodied energy per inherited instruction
    float carcassPerMass = 8.0f; // finite chemical-energy equivalent stored in body tissue
    float lunge = 1.5f;          // energy per attack attempt
    // life history
    float lifespan = 90.0f;      // s; deliberately accelerated experimental timescale
    float maturity = 6.0f;       // s
    float pathogen = 0.006f;     // host-specific disease: hazard/s = pathogen x (species share)^2
    float vTop = 58.0f;          // top swimming speed, normalizes speed genes
};

struct SimConfig {
    float worldW = 2592.0f;
    float worldH = 1728.0f;
    float cellSize = 48.0f;
    float dtPhys = 0.05f;
    int mesoEvery = 5;      // metabolism, sensing, reproduction (0.25 s)
    int evoEvery = 100;     // macro: speciation bookkeeping, telemetry (5 s)
    std::uint64_t seed = 0xC0FFEEULL;
    std::size_t maxAgents = 27000;
    std::size_t initialAgents = 4050;
    EcoParams eco;
};

}  // namespace alife
