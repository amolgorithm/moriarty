#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

#include "alife/core/grid.hpp"
#include "alife/core/rng.hpp"

using namespace alife;

TEST_CASE("grid neighbor query matches brute force on a torus") {
    const float W = 256.0f, H = 192.0f, R = 16.0f;
    ToroidalGrid grid(W, H, R);

    const std::size_t n = 600;
    std::vector<float> x(n), y(n);
    for (std::size_t i = 0; i < n; ++i) {
        CounterRng r(42, 0, i);
        x[i] = r.uniform() * W;
        y[i] = r.uniform() * H;
    }
    grid.rebuild(x.data(), y.data(), n);

    // actual cell widths can be a little bigger than R, so query radius is the real cell size
    const float qr = std::min(W / static_cast<float>(grid.nx), H / static_cast<float>(grid.ny));

    for (std::size_t i = 0; i < n; i += 7) {
        std::vector<std::uint32_t> viaGrid, brute;
        grid.forNeighbors(x[i], y[i], [&](std::uint32_t j) {
            const float dx = wrapDelta(x[j] - x[i], W), dy = wrapDelta(y[j] - y[i], H);
            if (dx * dx + dy * dy <= qr * qr) viaGrid.push_back(j);
        });
        for (std::uint32_t j = 0; j < n; ++j) {
            const float dx = wrapDelta(x[j] - x[i], W), dy = wrapDelta(y[j] - y[i], H);
            if (dx * dx + dy * dy <= qr * qr) brute.push_back(j);
        }
        std::sort(viaGrid.begin(), viaGrid.end());
        CHECK(viaGrid == brute);
    }
}

TEST_CASE("every agent lands in exactly one cell") {
    ToroidalGrid grid(128.0f, 128.0f, 16.0f);
    std::vector<float> x{0.0f, 127.9f, 64.0f}, y{0.0f, 127.9f, 64.0f};
    grid.rebuild(x.data(), y.data(), x.size());
    CHECK(grid.cellStart.back() == 3);
    CHECK(grid.sorted.size() == 3);
}

TEST_CASE("wrap helpers") {
    CHECK(wrap(-1.0f, 10.0f) == doctest::Approx(9.0f));
    CHECK(wrap(11.0f, 10.0f) == doctest::Approx(1.0f));
    CHECK(wrapDelta(9.0f, 10.0f) == doctest::Approx(-1.0f));
    CHECK(wrapDelta(-9.0f, 10.0f) == doctest::Approx(1.0f));
}
