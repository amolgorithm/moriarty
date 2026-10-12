#include "alife/core/world.hpp"

#include <algorithm>
#include <cmath>

#include "alife/genetics/genome.hpp"
#include "alife/genetics/gpm.hpp"
#include "alife/genetics/operators.hpp"

namespace alife {
namespace {
constexpr float kTwoPi = 6.28318530718f;
float smoothstep(float a, float b, float x) {
    const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
}  // namespace

World::World(const SimConfig& c) : cfg(c), grid(c.worldW, c.worldH, c.cellSize) {
    pool.reserve(c.maxAgents);
    const auto& e = cfg.eco;
    for (Field* f : {&R[0], &R[1], &K0[0], &K0[1], &Keff[0], &Keff[1], &Q, &Qtmp})
        f->init(e.fieldW, e.fieldH, cfg.worldW, cfg.worldH);
    buildTerrain(0);
    buildTerrain(1);
    initFlow();
    seedPopulation();
    stats.initialEnergy = energyInventory();

    CounterRng r(cfg.seed, 0, 0x7AC);
    tracerX.resize(700);
    tracerY.resize(700);
    for (std::size_t i = 0; i < tracerX.size(); ++i) {
        tracerX[i] = r.uniform() * cfg.worldW;
        tracerY[i] = r.uniform() * cfg.worldH;
    }
}

void World::buildTerrain(int ch) {
    // carrying capacity from random Fourier modes on the torus: rich patches, deserts, gradients
    CounterRng rng(cfg.seed, 0, 0x7E88 + static_cast<std::uint64_t>(ch) * 977);
    struct M { float kx, ky, amp, ph; };
    std::vector<M> modes;
    for (int i = 0; i < 18; ++i) {
        int mx = static_cast<int>(rng.next() % 7) - 3, my = static_cast<int>(rng.next() % 7) - 3;
        if (mx == 0 && my == 0) mx = 1;
        modes.push_back({kTwoPi * static_cast<float>(mx) / cfg.worldW, kTwoPi * static_cast<float>(my) / cfg.worldH,
                         1.0f / (1.0f + std::hypot(static_cast<float>(mx), static_cast<float>(my))),
                         rng.uniform() * kTwoPi});
    }
    Field& K = K0[ch];
    std::vector<float> val(K.v.size());
    float lo = 1e9f, hi = -1e9f;
    for (int y = 0; y < K.h; ++y)
        for (int x = 0; x < K.w; ++x) {
            const float wx = (static_cast<float>(x) + 0.5f) * K.cellW, wy = (static_cast<float>(y) + 0.5f) * K.cellH;
            float s = 0.0f;
            for (const auto& m : modes) s += m.amp * std::cos(m.kx * wx + m.ky * wy + m.ph);
            val[static_cast<std::size_t>(y) * static_cast<std::size_t>(K.w) + static_cast<std::size_t>(x)] = s;
            lo = std::min(lo, s); hi = std::max(hi, s);
        }
    for (std::size_t i = 0; i < val.size(); ++i) {
        const float t = (val[i] - lo) / (hi - lo);
        K.v[i] = cfg.eco.Kmax * (0.08f + 0.92f * smoothstep(0.25f, 0.8f, t));
        Keff[ch].v[i] = K.v[i];
        R[ch].v[i] = 0.7f * K.v[i];
    }
}

void World::initFlow() {
    CounterRng rng(cfg.seed, 0, 0xF10);
    const int mm[6][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}, {2, 1}, {1, 2}};
    for (const auto& m : mm) {
        FlowMode f;
        f.kx = kTwoPi * static_cast<float>(m[0]) / cfg.worldW;
        f.ky = kTwoPi * static_cast<float>(m[1]) / cfg.worldH;
        const float kn = std::hypot(f.kx, f.ky);
        f.dx = f.kx / kn; f.dy = f.ky / kn;
        f.ampSigma = 7.0f / (1.0f + 0.4f * std::hypot(static_cast<float>(m[0]), static_cast<float>(m[1])));
        f.amp = f.ampSigma * rng.normal();
        f.phase = rng.uniform() * kTwoPi;
        f.omega = (rng.uniform() - 0.5f) * 0.1f;
        flow.push_back(f);
    }
}

std::pair<float, float> World::flowAt(float x, float y) const noexcept {
    float u = 0.0f, v = 0.0f;
    for (const auto& m : flow) {
        const float c = m.amp * std::cos(m.kx * x + m.ky * y + m.phase);
        u += c * m.dy;
        v -= c * m.dx;
    }
    return {u, v};
}

std::uint32_t World::addAgent(const float* genome, float x, float y, float energy, std::uint32_t sp,
                              std::uint64_t parentId, std::uint32_t gen,
                              const genetics::Circuit& inheritedCircuit) {
    const std::uint32_t i = pool.spawn(x, y);
    std::copy(genome, genome + kGenomeLen, pool.genome.begin() + static_cast<std::ptrdiff_t>(i) * static_cast<std::ptrdiff_t>(kGenomeLen));
    if (inheritedCircuit.empty()) {
        pool.circuit[i] = genetics::makeSeedCircuit();
    } else {
        pool.circuit[i] = inheritedCircuit;
    }
    pool.circuitState[i].assign(pool.circuit[i].size(), 0.0f);
    const auto ph = genetics::decode(genome, cfg.eco.vTop);
    pool.eta[i] = ph.eta; pool.senseR[i] = ph.senseR; pool.vMax[i] = ph.vMax;
    pool.aggression[i] = ph.aggression; pool.repThresh[i] = ph.repThresh;
    pool.radius[i] = ph.radius; pool.mass[i] = ph.mass; pool.defense[i] = ph.defense;
    pool.eMax[i] = ph.eMax; pool.explore[i] = ph.explore;
    pool.energy[i] = std::min(energy, ph.eMax);
    pool.species[i] = sp;
    pool.parent[i] = parentId;
    pool.generation[i] = gen;
    pool.birthTick[i] = tick;
    return i;
}

void World::seedPopulation() {
    float base[kGenomeLen];
    std::fill(base, base + kGenomeLen, 0.5f);
    base[genetics::kAggr] = 0.18f;
    base[genetics::kMutRate] = 0.45f;
    const genetics::Circuit ancestorCircuit = genetics::makeSeedCircuit();
    const std::uint32_t ancestor = species.addFounder(base, "Vita", "communis", 30.0f, 0, 0, ancestorCircuit);

    for (std::size_t i = 0; i < cfg.initialAgents; ++i) {
        CounterRng rng(cfg.seed, 0, 0x1000 + i);
        float g[kGenomeLen];
        for (std::size_t k = 0; k < kGenomeLen; ++k)
            g[k] = std::clamp(base[k] + 0.12f * rng.normal(), 0.0f, 1.0f);
        g[genetics::kAggr] = std::clamp(base[genetics::kAggr] + 0.08f * rng.normal(), 0.0f, 1.0f);
        g[genetics::kMutRate] = std::clamp(0.45f + 0.12f * rng.normal(), 0.05f, 1.0f);

        const float x = rng.uniform() * cfg.worldW;
        const float y = rng.uniform() * cfg.worldH;
        const auto ph = genetics::decode(g, cfg.eco.vTop);
        genetics::Circuit circuit = ancestorCircuit;
        genetics::mutateCircuit(circuit, 0.08f, rng);
        const std::uint32_t initialSpecies = species.classify(g, circuit, ancestor, 0, rng);
        const std::uint32_t idx = addAgent(g, x, y, ph.eMax * (0.4f + 0.4f * rng.uniform()),
                                           initialSpecies, 0, 0, circuit);
        pool.age[idx] = rng.uniform() * 35.0f;
        pool.heading[idx] = rng.uniform() * kTwoPi;
    }
    grid.rebuild(pool.read().px.data(), pool.read().py.data(), pool.count());
}

}  // namespace alife
