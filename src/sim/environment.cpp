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

void stepEnvironment(World& w, float dt) {
    const auto& eco = w.cfg.eco;
    CounterRng rng(w.cfg.seed, w.tick, 0xE17ull);
    w.time += static_cast<double>(dt);

    // seasons: nominal period plus a random walk in phase, so cycles are never periodic
    w.seasonPhase += (kTwoPi / eco.seasonPeriod) * dt + 0.05f * std::sqrt(dt) * rng.normal();
    w.season = std::sin(w.seasonPhase);

    // resource weather: slow OU drift of carrying capacity
    constexpr float tauZ = 45.0f, sigZ = 0.12f;
    w.weather += -w.weather / tauZ * dt + sigZ * std::sqrt(2.0f * dt / tauZ) * rng.normal();

    // currents: OU amplitudes, drifting phases
    for (auto& m : w.flow) {
        constexpr float tau = 50.0f;
        m.amp += -m.amp / tau * dt + m.ampSigma * std::sqrt(2.0f * dt / tau) * rng.normal();
        m.phase += m.omega * dt + 0.03f * std::sqrt(dt) * rng.normal();
    }

    // shot noise: occasional blights (0.15x) and blooms (2.2x), roughly one per 70 s
    for (auto& p : w.patches) p.age += dt;
    w.patches.erase(std::remove_if(w.patches.begin(), w.patches.end(),
                                   [](const Patch& p) { return p.age > p.duration; }),
                    w.patches.end());
    if (w.patches.size() < 4 && rng.uniform() < dt / 70.0f) {
        Patch p;
        p.x = rng.uniform() * w.cfg.worldW;
        p.y = rng.uniform() * w.cfg.worldH;
        p.radius = 100.0f + rng.uniform() * 130.0f;
        p.factor = rng.uniform() < 0.55f ? 0.15f : 2.2f;
        p.age = 0.0f;
        p.duration = 60.0f + rng.uniform() * 90.0f;
        w.patches.push_back(p);
    }

    // effective capacity and regrowth, per food channel
    const float global = std::clamp(1.0f + eco.seasonAmp * w.season + w.weather, 0.25f, 2.0f);
    const float W = w.cfg.worldW, H = w.cfg.worldH;
    for (int y = 0; y < w.Q.h; ++y) {
        for (int x = 0; x < w.Q.w; ++x) {
            const std::size_t i = static_cast<std::size_t>(y) * static_cast<std::size_t>(w.Q.w) + static_cast<std::size_t>(x);
            const float cx = (static_cast<float>(x) + 0.5f) * w.Q.cellW, cy = (static_cast<float>(y) + 0.5f) * w.Q.cellH;
            float factor = global;
            for (const auto& p : w.patches) {
                const float dx = wrapDelta(cx - p.x, W), dy = wrapDelta(cy - p.y, H);
                const float s = smoothstep01(1.0f - std::hypot(dx, dy) / p.radius);
                const float env = std::min({1.0f, p.age / 8.0f, (p.duration - p.age) / 25.0f});
                factor *= 1.0f + (p.factor - 1.0f) * s * std::max(0.0f, env);
            }
            const float heat = 1.0f + eco.heatCoupling * w.Q.v[i];
            for (int ch = 0; ch < 2; ++ch) {
                const float ke = std::max(0.2f, w.K0[ch].v[i] * factor);
                w.Keff[ch].v[i] = ke;
                const float r = w.R[ch].v[i];
                const float growth = eco.regrow * std::max(r, 0.04f * ke) * (1.0f - r / ke) / heat;
                w.R[ch].v[i] = std::max(0.0f, r + dt * growth);
            }
        }
    }

    // heat: diffusion + decay
    const int fw = w.Q.w, fh = w.Q.h;
    for (int y = 0; y < fh; ++y)
        for (int x = 0; x < fw; ++x) {
            const float q = w.Q.at(x, y);
            const float lap = w.Q.at(x + 1, y) + w.Q.at(x - 1, y) + w.Q.at(x, y + 1) + w.Q.at(x, y - 1) - 4.0f * q;
            w.Qtmp.v[static_cast<std::size_t>(y) * static_cast<std::size_t>(fw) + static_cast<std::size_t>(x)] =
                std::max(0.0f, q + dt * (eco.heatDiffusion * lap - eco.heatDecay * q));
        }
    std::swap(w.Q.v, w.Qtmp.v);
}

}  // namespace alife::sim
