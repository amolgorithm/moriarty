#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "alife/core/aligned_alloc.hpp"

namespace alife {

inline float wrap(float v, float L) noexcept {
    v = std::fmod(v, L);
    return v < 0.0f ? v + L : v;
}

// Shortest signed displacement on a ring of length L.
inline float wrapDelta(float d, float L) noexcept {
    const float h = 0.5f * L;
    if (d > h) return d - L;
    if (d < -h) return d + L;
    return d;
}

// Uniform grid on a torus, CSR layout. Rebuild is a counting sort, O(N).
// Contract: positions in [0, W) x [0, H), cell size >= max query radius, so a
// neighbor query only ever touches the 3x3 block around the agent's cell.
class ToroidalGrid {
   public:
    ToroidalGrid() = default;
    ToroidalGrid(float worldW, float worldH, float cellSize);

    void rebuild(const float* px, const float* py, std::size_t n);

    template <class F>
    void forNeighbors(float x, float y, F&& f) const {
        const int cx = cellX(x), cy = cellY(y);
        for (int dy = -1; dy <= 1; ++dy) {
            const int gy = (cy + dy + ny) % ny;
            for (int dx = -1; dx <= 1; ++dx) {
                const int gx = (cx + dx + nx) % nx;
                const std::size_t c = static_cast<std::size_t>(gy) * static_cast<std::size_t>(nx) +
                                      static_cast<std::size_t>(gx);
                for (std::uint32_t k = cellStart[c]; k < cellStart[c + 1]; ++k) f(sorted[k]);
            }
        }
    }

    int cellX(float x) const noexcept { return std::min(nx - 1, static_cast<int>(x * invW_)); }
    int cellY(float y) const noexcept { return std::min(ny - 1, static_cast<int>(y * invH_)); }

    float worldW = 0, worldH = 0;
    int nx = 0, ny = 0;
    avec<std::uint32_t> cellStart;  // nx*ny + 1 prefix sums
    avec<std::uint32_t> sorted;     // agent indices grouped by cell
    std::vector<std::uint32_t> cellOf_, cursor_;

   private:
    float invW_ = 0, invH_ = 0;
};

}  // namespace alife
