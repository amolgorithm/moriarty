#pragma once
#include "alife/core/rng.hpp"

namespace alife::genetics {

// Gaussian point mutation per gene, with a rare large jump. Genes stay in [0, 1].
void mutate(float* genomeRow, float rate, CounterRng& rng);

// Uniform crossover.
void crossover(const float* a, const float* b, float* out, CounterRng& rng);

}  // namespace alife::genetics
