#pragma once
#include "alife/core/rng.hpp"

namespace alife::genetics {

void mutate(float* genomeRow, float rate, CounterRng& rng);
void crossover(const float* a, const float* b, float* out, CounterRng& rng);

}  // namespace alife::genetics
