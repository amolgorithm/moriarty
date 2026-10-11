#pragma once
#include "alife/genetics/circuit.hpp"
#include "alife/core/rng.hpp"

namespace alife::genetics {

// Gaussian point mutation per gene, with a rare large jump. Genes stay in [0, 1].
void mutate(float* genomeRow, float rate, CounterRng& rng);

// Uniform crossover.
void crossover(const float* a, const float* b, float* out, CounterRng& rng);

void mutateCircuit(Circuit& circuit, float rate, CounterRng& rng);
Circuit crossoverCircuit(std::span<const UnitGene> a, std::span<const UnitGene> b, CounterRng& rng);

}  // namespace alife::genetics
