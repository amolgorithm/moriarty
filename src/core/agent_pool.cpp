#include "alife/core/agent_pool.hpp"

#include <algorithm>

namespace alife {

void AgentPool::reserve(std::size_t n) {
    for (auto& k : kin) {
        k.px.reserve(n); k.py.reserve(n); k.vx.reserve(n); k.vy.reserve(n);
    }
    energy.reserve(n); heat.reserve(n); age.reserve(n);
    eta.reserve(n); senseR.reserve(n); vMax.reserve(n); aggression.reserve(n); repThresh.reserve(n);
    cell.reserve(n);
    genome.reserve(n * kGenomeLen);
    id.reserve(n); parent.reserve(n); birthTick.reserve(n); species.reserve(n);
}

std::uint32_t AgentPool::spawn(float x, float y) {
    const std::uint32_t i = count();
    for (auto& k : kin) {
        k.px.push_back(x); k.py.push_back(y); k.vx.push_back(0.0f); k.vy.push_back(0.0f);
    }
    energy.push_back(0.0f); heat.push_back(0.0f); age.push_back(0.0f);
    eta.push_back(1.0f); senseR.push_back(8.0f); vMax.push_back(1.0f);
    aggression.push_back(0.0f); repThresh.push_back(1.0f);
    cell.push_back(0);
    genome.resize(genome.size() + kGenomeLen, 0.0f);
    id.push_back(nextId++); parent.push_back(0); birthTick.push_back(0); species.push_back(0);
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
    drop(energy); drop(heat); drop(age);
    drop(eta); drop(senseR); drop(vMax); drop(aggression); drop(repThresh);
    drop(cell);
    drop(id); drop(parent); drop(birthTick); drop(species);
    if (i != last) {
        std::copy_n(genome.begin() + static_cast<std::ptrdiff_t>(last * kGenomeLen), kGenomeLen,
                    genome.begin() + static_cast<std::ptrdiff_t>(i * kGenomeLen));
    }
    genome.resize(static_cast<std::size_t>(last) * kGenomeLen);
}

}  // namespace alife
