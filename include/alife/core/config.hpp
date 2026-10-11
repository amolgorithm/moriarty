#pragma once
#include <cstddef>
#include <cstdint>

namespace alife {

inline constexpr std::size_t kGenomeLen = 32;  // 6 life-history genes + 26 controller weights
inline constexpr float kMaxSense = 32.0f;      // sense radius cap, grid cell must be >= this

struct SimConfig {
    float worldW = 2048.0f;
    float worldH = 2048.0f;
    float cellSize = 32.0f;
    float dtPhys = 0.01f;
    int mesoEvery = 10;     // meso tick = 10 x micro
    int evoEvery = 1000;    // macro tick = 1000 x micro
    std::uint64_t seed = 0xC0FFEEULL;
    std::size_t maxAgents = 100'000;
};

}  // namespace alife
