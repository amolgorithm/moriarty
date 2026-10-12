#include "alife/sim/environment.hpp"

#include <algorithm>
#include <cmath>

namespace alife::sim {
namespace {
constexpr float kTwoPi = 6.28318530718f;

float smoothstep01(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
}  // namespace

void stepEnvironment(World& w, float dt, ThreadPool& threadPool) {
    const auto& eco = w.cfg.eco;
    CounterRng rng(w.cfg.seed, w.tick, 0xE17ull);
    w.time += static_cast<double>(dt);

    // seasons: nominal period plus a random walk in phase, so cycles are never periodic
    w.seasonPhase += (kTwoPi / eco.seasonPeriod) * dt + 0.05f * std::sqrt(dt) * rng.normal();
    w.season = std::sin(w.seasonPhase);

    // resource weather: slow OU drift of carrying capacity
    const float tauZ = std::max(1.0f, eco.weatherTau);
    const float sigZ = std::max(0.0f, eco.weatherSigma);
    w.weather += -w.weather / tauZ * dt + sigZ * std::sqrt(2.0f * dt / tauZ) * rng.normal();

    // currents: OU amplitudes, drifting phases
    for (auto& m : w.flow) {
        const float tau = std::max(1.0f, eco.currentTau);
        m.amp += -m.amp / tau * dt + m.ampSigma * std::sqrt(2.0f * dt / tau) * rng.normal();
        m.phase += m.omega * dt + eco.currentPhaseNoise * std::sqrt(dt) * rng.normal();
    }

    // Local disturbances turn over quickly enough to impose changing selection pressures.
    for (auto& p : w.patches) p.age += dt;
    w.patches.erase(std::remove_if(w.patches.begin(), w.patches.end(),
                                   [](const Patch& p) { return p.age > p.duration; }),
                    w.patches.end());
    if (w.patches.size() < 6 && rng.uniform() < dt / std::max(1.0f, eco.disturbanceInterval)) {
        Patch p;
        p.x = rng.uniform() * w.cfg.worldW;
        p.y = rng.uniform() * w.cfg.worldH;
        p.radius = 120.0f + rng.uniform() * 185.0f;
        p.factor = rng.uniform() < 0.58f ? eco.blightFactor : eco.bloomFactor;
        p.age = 0.0f;
        p.duration = eco.disturbanceMinDuration + rng.uniform() * eco.disturbanceDurationRange;
        w.patches.push_back(p);
    }

    // Update resource and heat fields together; each cell is independent.
    const float global = std::clamp(1.0f + eco.seasonAmp * w.season + w.weather, 0.25f, 2.0f);
    const float W = w.cfg.worldW, H = w.cfg.worldH;
    const int fw = w.Q.w, fh = w.Q.h;
    const std::size_t cellCount = w.Q.v.size();
    constexpr std::size_t chunkSize = 512;
    struct FieldTotals {
        double external = 0.0, dissipated = 0.0, heatBefore = 0.0, heatAfter = 0.0;
    };
    std::vector<FieldTotals> totals((cellCount + chunkSize - 1) / chunkSize);
    threadPool.parallelFor(cellCount, chunkSize, [&](std::size_t begin, std::size_t end) {
        FieldTotals local;
        for (std::size_t i = begin; i < end; ++i) {
            const int x = static_cast<int>(i % static_cast<std::size_t>(fw));
            const int y = static_cast<int>(i / static_cast<std::size_t>(fw));
            const float cx = (static_cast<float>(x) + 0.5f) * w.Q.cellW;
            const float cy = (static_cast<float>(y) + 0.5f) * w.Q.cellH;
            float factor = global;
            for (const auto& patch : w.patches) {
                const float dx = wrapDelta(cx - patch.x, W), dy = wrapDelta(cy - patch.y, H);
                const float strength = smoothstep01(1.0f - std::hypot(dx, dy) / patch.radius);
                const float envelope = std::min({1.0f, patch.age / 8.0f,
                                                 (patch.duration - patch.age) / 25.0f});
                factor *= 1.0f + (patch.factor - 1.0f) * strength * std::max(0.0f, envelope);
            }
            const float heat = 1.0f + eco.heatCoupling * w.Q.v[i];
            for (int ch = 0; ch < 2; ++ch) {
                const float ke = std::max(0.2f, w.K0[ch].v[i] * factor);
                w.Keff[ch].v[i] = ke;
                const float resource = w.R[ch].v[i];
                const float growth = eco.regrow * std::max(resource, 0.04f * ke) *
                                     (1.0f - resource / ke) / heat;
                const float updated = std::max(0.0f, resource + dt * growth);
                const double deltaEnergy = static_cast<double>(updated - resource) * eco.energyPerR;
                if (deltaEnergy >= 0.0)
                    local.external += deltaEnergy;
                else
                    local.dissipated -= deltaEnergy;
                w.R[ch].v[i] = updated;
            }

            const float q = w.Q.v[i];
            const std::size_t left = static_cast<std::size_t>(y) * fw + (x == 0 ? fw - 1 : x - 1);
            const std::size_t right = static_cast<std::size_t>(y) * fw + (x + 1 == fw ? 0 : x + 1);
            const std::size_t up = static_cast<std::size_t>(y == 0 ? fh - 1 : y - 1) * fw + x;
            const std::size_t down = static_cast<std::size_t>(y + 1 == fh ? 0 : y + 1) * fw + x;
            const float lap = w.Q.v[left] + w.Q.v[right] + w.Q.v[up] + w.Q.v[down] - 4.0f * q;
            const float nextHeat = std::max(0.0f, q + dt * (eco.heatDiffusion * lap - eco.heatDecay * q));
            w.Qtmp.v[i] = nextHeat;
            local.heatBefore += q;
            local.heatAfter += nextHeat;
        }
        totals[begin / chunkSize] = local;
    });

    double heatBefore = 0.0, heatAfter = 0.0;
    for (const FieldTotals& local : totals) {
        w.stats.externalEnergy += local.external;
        w.stats.dissipatedEnergy += local.dissipated;
        heatBefore += local.heatBefore;
        heatAfter += local.heatAfter;
    }
    std::swap(w.Q.v, w.Qtmp.v);
    const double heatDelta = heatAfter - heatBefore;
    if (heatDelta >= 0.0)
        w.stats.externalEnergy += heatDelta;
    else
        w.stats.dissipatedEnergy -= heatDelta;
}

}  // namespace alife::sim
