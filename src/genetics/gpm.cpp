#include "alife/genetics/gpm.hpp"

namespace alife::genetics {

Phenotype decode(const float* g, float vTop) {
    Phenotype p;
    p.eta = 0.30f + 0.70f * g[kEta];
    p.senseR = 14.0f + 34.0f * g[kSense];
    p.vMax = 8.0f + (vTop - 8.0f) * g[kSpeed];
    p.aggression = g[kAggr];
    p.repThresh = 0.55f + 0.40f * g[kRepThr];
    p.mutRate = 0.004f + 0.076f * g[kMutRate];
    p.radius = 3.0f + 5.5f * g[kSize];
    const float r4 = p.radius / 4.0f;
    p.mass = r4 * r4;
    p.eMax = 40.0f * p.mass;
    p.defense = g[kDefense];
    p.explore = 0.1f + 0.9f * g[kExplore];
    p.dietPreference = g[kDietPreference];
    p.heatTolerance = g[kHeatTolerance];
    return p;
}

}  // namespace alife::genetics
