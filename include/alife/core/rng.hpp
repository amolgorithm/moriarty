#pragma once
#include <cmath>
#include <cstdint>

namespace alife {

constexpr std::uint64_t mix64(std::uint64_t z) noexcept {
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// Counter-based RNG keyed by (seed, tick, slot). No shared state, so results
// don't depend on thread count or scheduling order.
class CounterRng {
   public:
    constexpr CounterRng(std::uint64_t seed, std::uint64_t tick, std::uint64_t slot) noexcept
        : key_(mix64(seed ^ mix64(tick ^ mix64(slot)))) {}

    constexpr std::uint64_t next() noexcept { return mix64(key_ + (ctr_++) * 0xD1B54A32D192ED03ull); }

    // [0, 1)
    float uniform() noexcept { return static_cast<float>(next() >> 40) * (1.0f / 16777216.0f); }

    // N(0, 1) via Box-Muller
    float normal() noexcept {
        const float u1 = (static_cast<float>(next() >> 40) + 1.0f) * (1.0f / 16777216.0f);
        const float u2 = uniform();
        return std::sqrt(-2.0f * std::log(u1)) * std::cos(6.28318530718f * u2);
    }

   private:
    std::uint64_t key_;
    std::uint64_t ctr_ = 0;
};

}  // namespace alife
