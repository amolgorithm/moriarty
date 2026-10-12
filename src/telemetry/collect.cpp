#include "alife/telemetry/collect.hpp"

#include <vector>

#include "alife/math/entropy.hpp"

namespace alife::telemetry {

Telemetry collect(const World& w) {
    Telemetry t;
    t.tick = w.tick;
    t.population = w.pool.count();

    std::vector<std::uint32_t> counts(w.species.all().size(), 0);
    for (std::uint32_t i = 0; i < w.pool.count(); ++i) ++counts[w.pool.species[i]];
    std::vector<std::uint32_t> extant;
    for (const auto count : counts) {
        if (count == 0) continue;
        extant.push_back(count);
        if (count >= genetics::kEstablished) ++t.speciesCount;
    }
    t.shannonBits = static_cast<float>(math::shannonBits(extant));

    double e = 0.0;
    double units = 0.0;
    for (std::uint32_t i = 0; i < w.pool.count(); ++i) {
        e += static_cast<double>(w.pool.energy[i] / w.pool.eMax[i]);
        units += static_cast<double>(w.pool.circuit[i].size());
    }
    t.meanEnergy = t.population ? static_cast<float>(e / static_cast<double>(t.population)) : 0.0f;
    t.meanCircuitUnits = t.population ? static_cast<float>(units / static_cast<double>(t.population)) : 0.0f;
    for (float q : w.Q.v) t.totalHeat += q;
    return t;
}

}  // namespace alife::telemetry
