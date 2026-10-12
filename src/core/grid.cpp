#include "alife/core/grid.hpp"

namespace alife {

ToroidalGrid::ToroidalGrid(float w, float h, float cell) : worldW(w), worldH(h) {
    // at least 3 cells per axis so the 3x3 block never visits a cell twice
    nx = std::max(3, static_cast<int>(w / cell));
    ny = std::max(3, static_cast<int>(h / cell));
    invW_ = static_cast<float>(nx) / w;
    invH_ = static_cast<float>(ny) / h;
    cellStart.assign(static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) + 1, 0);
}

void ToroidalGrid::rebuild(const float* px, const float* py, std::size_t n) {
    const std::size_t nCells = static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny);
    std::fill(cellStart.begin(), cellStart.end(), 0u);
    sorted.resize(n);

    cellOf_.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t c = static_cast<std::size_t>(cellY(py[i])) * static_cast<std::size_t>(nx) +
                              static_cast<std::size_t>(cellX(px[i]));
        cellOf_[i] = static_cast<std::uint32_t>(c);
        ++cellStart[c + 1];
    }
    for (std::size_t c = 0; c < nCells; ++c) cellStart[c + 1] += cellStart[c];

    cursor_.resize(nCells);
    std::copy_n(cellStart.begin(), nCells, cursor_.begin());
    for (std::size_t i = 0; i < n; ++i)
        sorted[cursor_[cellOf_[i]]++] = static_cast<std::uint32_t>(i);
}

}  // namespace alife
