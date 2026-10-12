#pragma once
#include <cstddef>

#include "alife/genetics/circuit.hpp"

namespace alife::genetics {

// All genes live in [0, 1] and are scaled to physical ranges by the GPM (gpm.hpp).
enum Gene : std::size_t {
    kEta = 0,      // metabolic efficiency (costs more upkeep)
    kSense,        // sense radius
    kSpeed,        // swimming speed
    kAggr,         // predation propensity vs grazing
    kRepThr,       // reproduction threshold, fraction of max energy
    kMutRate,      // self-adaptive mutation rate
    kSize,         // body radius
    kDefense,     // defense investment
    kExplore = 8,
    kDietPreference,
    kHeatTolerance,
    kImmunity,
    kRepair,
    kDigestion
};

// Species distance includes expressed program architecture as well as scalar traits.
inline constexpr std::size_t kDistTraitGenes = 13;
inline constexpr std::size_t kDistGenes = kDistTraitGenes + kCircuitOperationCount + 3;
inline constexpr std::size_t kDistIndex[kDistTraitGenes] = {
    kEta, kSense, kSpeed, kAggr, kRepThr, kSize, kDefense, kExplore, kDietPreference, kHeatTolerance,
    kImmunity, kRepair, kDigestion};

}  // namespace alife::genetics
