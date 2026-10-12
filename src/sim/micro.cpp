#include "alife/sim/micro.hpp"

#include <algorithm>
#include <cmath>

#include "alife/math/integrator.hpp"

namespace alife::sim {

// Newtonian layer: controller command + OU wander + soft collisions drive a Langevin
// step, the stochastic current advects the body. Reads kin[read], writes kin[write].
void microStep(World& w, ThreadPool& tp) {
    auto& p = w.pool;
    const auto& cfg = w.cfg;
    const float dt = cfg.dtPhys, W = cfg.worldW, H = cfg.worldH;
    const auto& rk = p.read();
    auto& wk = p.write();

    constexpr float gamma = 1.2f;   // drag
    constexpr float tauW = 2.0f;    // wander correlation time
    const math::EulerMaruyama integ{gamma, 5.0f};
    const float ouK = std::sqrt(2.0f * dt / tauW);

    tp.parallelFor(p.count(), 1024, [&](std::size_t b, std::size_t e) {
        for (std::size_t i = b; i < e; ++i) {
            CounterRng rng(cfg.seed, w.tick, p.id[i]);
            float x = rk.px[i], y = rk.py[i], vx = rk.vx[i], vy = rk.vy[i];

            p.wx[i] += -p.wx[i] / tauW * dt + ouK * rng.normal();
            p.wy[i] += -p.wy[i] / tauW * dt + ouK * rng.normal();

            // soft body repulsion
            float sx = 0.0f, sy = 0.0f;
            const float ri = p.radius[i];
            w.grid.forNeighbors(x, y, [&](std::uint32_t j) {
                if (j == i) return;
                const float dx = wrapDelta(x - rk.px[j], W), dy = wrapDelta(y - rk.py[j], H);
                const float rs = ri + p.radius[j] + 1.0f;
                const float d2 = dx * dx + dy * dy;
                if (d2 < rs * rs && d2 > 1e-6f) {
                    const float d = std::sqrt(d2), f = (rs - d) / rs;
                    sx += dx / d * f;
                    sy += dy / d * f;
                }
            });

            const float intent = std::hypot(p.ax[i], p.ay[i]);
            const float ex = p.explore[i] * (0.12f + 0.25f / (1.0f + intent));
            const float g = gamma * p.vMax[i];
            const float fx = (p.ax[i] + ex * p.wx[i] + 1.5f * sx) * g;
            const float fy = (p.ay[i] + ex * p.wy[i] + 1.5f * sy) * g;
            integ.step(x, y, vx, vy, fx, fy, 1.0f, dt, rng);

            // passive advection by the current, heavier bodies resist it
            const auto [cu, cv] = w.flowAt(rk.px[i], rk.py[i]);
            const float couple = 1.0f / (0.5f + 0.5f * p.mass[i]);
            x += cu * couple * dt;
            y += cv * couple * dt;

            const float sp = std::hypot(vx, vy), cap = 1.6f * p.vMax[i];
            if (sp > cap) { vx *= cap / sp; vy *= cap / sp; }

            wk.px[i] = wrap(x, W);
            wk.py[i] = wrap(y, H);
            wk.vx[i] = vx;
            wk.vy[i] = vy;

            if (sp > 3.0f) {
                float d = std::atan2(vy, vx) - p.heading[i];
                d = std::remainder(d, 6.28318530718f);
                p.heading[i] += d * 0.2f;
            }
        }
    });

    // tracers: passive particles that make the current visible
    for (std::size_t t = 0; t < w.tracerX.size(); ++t) {
        CounterRng rng(cfg.seed, w.tick, 0x70000000ull + t);
        const auto [cu, cv] = w.flowAt(w.tracerX[t], w.tracerY[t]);
        w.tracerX[t] = wrap(w.tracerX[t] + cu * dt + 1.2f * std::sqrt(dt) * rng.normal(), W);
        w.tracerY[t] = wrap(w.tracerY[t] + cv * dt + 1.2f * std::sqrt(dt) * rng.normal(), H);
    }
}

}  // namespace alife::sim
