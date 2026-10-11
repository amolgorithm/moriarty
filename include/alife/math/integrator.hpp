#pragma once
#include <cmath>

#include "alife/core/rng.hpp"

namespace alife::math {

// Overdamped-ish Langevin step, Euler-Maruyama:
//   v += (F/m - gamma v) dt + sigma sqrt(dt) xi,   x += v dt
// Position wrapping is the caller's job.
struct EulerMaruyama {
    float gamma = 1.0f;
    float sigma = 0.0f;

    void step(float& x, float& y, float& vx, float& vy, float fx, float fy, float invMass,
              float dt, CounterRng& rng) const noexcept {
        const float s = sigma * std::sqrt(dt);
        vx += (fx * invMass - gamma * vx) * dt + s * rng.normal();
        vy += (fy * invMass - gamma * vy) * dt + s * rng.normal();
        x += vx * dt;
        y += vy * dt;
    }
};

}  // namespace alife::math
