#include "alife/genetics/operators.hpp"

namespace alife::genetics {

void mutate(float*, float, CounterRng&) {
    // TODO(phase 3): per-gene gaussian perturbation, rate from the mutRate gene
}

void crossover(const float*, const float*, float*, CounterRng&) {
    // TODO(phase 3): uniform crossover
}

}  // namespace alife::genetics
