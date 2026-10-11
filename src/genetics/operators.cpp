#include "alife/genetics/operators.hpp"

#include <algorithm>

#include "alife/core/config.hpp"

namespace alife::genetics {

void mutate(float* g, float rate, CounterRng& rng) {
    for (std::size_t i = 0; i < kGenomeLen; ++i) {
        float sigma = rate;
        if (rng.uniform() < 0.02f) sigma *= 6.0f;  // occasional macro-mutation
        g[i] = std::clamp(g[i] + sigma * rng.normal(), 0.0f, 1.0f);
    }
}

void crossover(const float* a, const float* b, float* out, CounterRng& rng) {
    for (std::size_t i = 0; i < kGenomeLen; ++i) out[i] = rng.uniform() < 0.5f ? a[i] : b[i];
}

}  // namespace alife::genetics
