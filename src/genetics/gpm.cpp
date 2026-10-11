#include "alife/genetics/gpm.hpp"

#include <algorithm>
#include <cmath>

namespace alife::genetics {

Phenotype decode(const float* g, float vTop) {
    Phenotype p;
    p.eta = 0.30f + 0.70f * g[kEta];
    p.senseR = 14.0f + 34.0f * g[kSense];
    p.vMax = 8.0f + (vTop - 8.0f) * g[kSpeed];
    p.aggression = g[kAggr];
    p.repThresh = 0.55f + 0.40f * g[kRepThr];
    p.mutRate = 0.004f + 0.046f * g[kMutRate];
    p.radius = 3.0f + 5.5f * g[kSize];
    const float r4 = p.radius / 4.0f;
    p.mass = r4 * r4;
    p.eMax = 40.0f * p.mass;
    p.defense = g[kDefense];
    p.explore = 0.1f + 0.9f * g[kExplore];
    return p;
}

Controller controller(const float* g, float energyFrac) {
    Controller c{};
    const float hunger = 1.0f - std::clamp(energyFrac, 0.0f, 1.0f);
    for (int k = 0; k < kCueCount; ++k) {
        const float gain = 2.0f * g[cueGain(k)] - 1.0f;
        const float bias = 2.0f * g[cueBias(k)] - 1.0f;
        const float slope = 4.0f * g[cueSlope(k)] - 2.0f;
        const float gate = 1.0f / (1.0f + std::exp(-3.0f * (bias + slope * (hunger - 0.5f) * 2.0f)));
        c.w[k] = gain * 2.0f * gate;
    }
    return c;
}

}  // namespace alife::genetics
