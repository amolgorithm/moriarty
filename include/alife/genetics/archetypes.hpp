#pragma once
#include "alife/core/rng.hpp"

namespace alife::genetics {

inline constexpr int kArchetypes = 7;

// Founder lineages seeded at tick 0. Each fills a different ecological niche.
void archetypeGenome(int index, float* out);
const char* archetypeGenus(int index);
const char* archetypeEpithet(int index);
float archetypeHue(int index);  // degrees
int archetypeCount(int index);  // initial population share, sums to ~1800

}  // namespace alife::genetics
