#pragma once
#include <cstddef>

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
    kExplore = 8
};

// Species distance includes expressed program architecture as well as scalar traits.
inline constexpr std::size_t kDistTraitGenes = 8;
inline constexpr std::size_t kDistGenes = kDistTraitGenes + 10;
inline constexpr std::size_t kDistIndex[kDistTraitGenes] = {
    kEta, kSense, kSpeed, kAggr, kRepThr, kSize, kDefense, kExplore};

}  // namespace alife::genetics
