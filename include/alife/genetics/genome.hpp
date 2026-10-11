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
    kDefense,      // armor / spines
    kHueOff,       // pigment variation (heritable, cosmetic)
    kPattern,      // body spotting (heritable, cosmetic)
    kCtrlBegin = 10,
    kExplore = 22,
    kSpare = 23
};

// Braitenberg-style sensorimotor controller: four cues, each steers the body with
// a gain that depends on hunger. Per cue: gain, bias, hunger slope.
enum Cue : int { kFood = 0, kPrey, kThreat, kKin, kCueCount };

constexpr std::size_t cueGain(int c) { return kCtrlBegin + 3 * static_cast<std::size_t>(c); }
constexpr std::size_t cueBias(int c) { return cueGain(c) + 1; }
constexpr std::size_t cueSlope(int c) { return cueGain(c) + 2; }

// Genes that count toward species distance.
inline constexpr std::size_t kDistGenes = 11;
inline constexpr std::size_t kDistIndex[kDistGenes] = {
    kEta, kSense, kSpeed, kAggr, kRepThr, kSize, kDefense,
    cueGain(0), cueGain(1), cueGain(2), cueGain(3)};

}  // namespace alife::genetics
