#include <benchmark/benchmark.h>

#include <vector>

#include "alife/core/grid.hpp"
#include "alife/core/rng.hpp"

static void BM_GridRebuild(benchmark::State& state) {
    const std::size_t n = static_cast<std::size_t>(state.range(0));
    const float W = 2048.0f, H = 2048.0f;
    alife::ToroidalGrid grid(W, H, 32.0f);
    std::vector<float> x(n), y(n);
    for (std::size_t i = 0; i < n; ++i) {
        alife::CounterRng r(1, 0, i);
        x[i] = r.uniform() * W;
        y[i] = r.uniform() * H;
    }
    for (auto _ : state) {
        grid.rebuild(x.data(), y.data(), n);
        benchmark::DoNotOptimize(grid.sorted.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(n));
}
BENCHMARK(BM_GridRebuild)->RangeMultiplier(4)->Range(1 << 12, 1 << 18);
