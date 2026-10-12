#include "alife/core/agent_pool.hpp"

#include <algorithm>

namespace alife {

template <class F>
void AgentPool::forEachColumn(F&& f) {
    const float z = 0.0f;
    const std::uint32_t z32 = 0;
    const std::uint64_t z64 = 0;
    f(energy, z); f(bodyEnergy, z); f(digest, z); f(age, z);
    f(eta, 1.0f); f(senseR, 20.0f); f(vMax, 30.0f); f(aggression, z); f(repThresh, 0.7f);
    f(dietPreference, 0.5f); f(heatTolerance, 0.5f);
    f(radius, 5.0f); f(mass, 1.5f); f(defense, z); f(eMax, 60.0f); f(explore, 0.5f);
    f(wx, z); f(wy, z); f(ax, z); f(ay, z);
    f(harvestGain, 1.0f); f(resourceBias, z); f(protection, z); f(huntBias, z); f(heading, z);
    f(plantIntake, z); f(animalIntake, z);
    f(action, static_cast<std::uint8_t>(AgentAction::Explore));
    f(cell, z32); f(species, z32); f(generation, z32);
    f(id, z64); f(parent, z64); f(birthTick, z64); f(preyId, z64); f(threatId, z64);
}

void AgentPool::reserve(std::size_t n) {
    for (auto& k : kin) {
        k.px.reserve(n); k.py.reserve(n); k.vx.reserve(n); k.vy.reserve(n);
    }
    forEachColumn([n](auto& v, auto) { v.reserve(n); });
    genome.reserve(n * kGenomeLen);
    circuit.reserve(n);
    circuitState.reserve(n);
    circuitWeights.reserve(n);
    circuitEligibility.reserve(n);
}

std::uint32_t AgentPool::spawn(float x, float y) {
    const std::uint32_t i = count();
    for (auto& k : kin) {
        k.px.push_back(x); k.py.push_back(y); k.vx.push_back(0.0f); k.vy.push_back(0.0f);
    }
    forEachColumn([](auto& v, auto init) { v.push_back(init); });
    genome.resize(genome.size() + kGenomeLen, 0.0f);
    circuit.emplace_back();
    circuitState.emplace_back();
    circuitWeights.emplace_back();
    circuitEligibility.emplace_back();
    id.back() = nextId++;
    return i;
}

void AgentPool::swapRemove(std::uint32_t i) {
    const std::uint32_t last = count() - 1;
    auto drop = [&](auto& v) {
        v[i] = v[last];
        v.pop_back();
    };
    for (auto& k : kin) {
        drop(k.px); drop(k.py); drop(k.vx); drop(k.vy);
    }
    forEachColumn([&](auto& v, auto) { drop(v); });
    if (i != last) {
        circuit[i] = std::move(circuit[last]);
        circuitState[i] = std::move(circuitState[last]);
        circuitWeights[i] = std::move(circuitWeights[last]);
        circuitEligibility[i] = std::move(circuitEligibility[last]);
    }
    circuit.pop_back();
    circuitState.pop_back();
    circuitWeights.pop_back();
    circuitEligibility.pop_back();
    if (i != last) {
        std::copy_n(genome.begin() + static_cast<std::ptrdiff_t>(last * kGenomeLen), kGenomeLen,
                    genome.begin() + static_cast<std::ptrdiff_t>(i * kGenomeLen));
    }
    genome.resize(static_cast<std::size_t>(last) * kGenomeLen);
}

}  // namespace alife
