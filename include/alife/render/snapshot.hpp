#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace alife::render {

// Everything the renderer needs from the sim for one frame. Flat arrays so it
// can be filled straight from the SoA pool and passed through a TripleBuffer.
struct RenderSnapshot {
    std::uint64_t tick = 0;
    float worldW = 0.0f, worldH = 0.0f;
    std::vector<float> x, y;
    std::vector<float> energy;  // normalized to [0, 1]
    std::vector<std::uint32_t> species;

    std::size_t size() const noexcept { return x.size(); }
    void clear() {
        x.clear(); y.clear(); energy.clear(); species.clear();
    }
};

}  // namespace alife::render
