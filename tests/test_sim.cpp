#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <unordered_set>

#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"
#include "alife/sim/scheduler.hpp"
#include "alife/telemetry/collect.hpp"

using namespace alife;

namespace {
void run(World& w, int ticks) {
    ThreadPool pool(4);
    sim::Scheduler s(w, pool);
    for (int i = 0; i < ticks; ++i) s.stepOnce();
}
}  // namespace

TEST_CASE("world seeds a single lineage and every agent column stays aligned") {
    World w;
    CHECK(w.pool.count() > 500);
    run(w, 600);
    const auto n = w.pool.count();
    CHECK(n > 0);
    CHECK(w.pool.read().px.size() == n);
    CHECK(w.pool.write().vy.size() == n);
    CHECK(w.pool.species.size() == n);
    CHECK(w.pool.bodyEnergy.size() == n);
    CHECK(w.pool.dietPreference.size() == n);
    CHECK(w.pool.heatTolerance.size() == n);
    CHECK(w.pool.immunity.size() == n);
    CHECK(w.pool.repair.size() == n);
    CHECK(w.pool.digestion.size() == n);
    CHECK(w.pool.huntBias.size() == n);
    CHECK(w.pool.socialBias.size() == n);
    CHECK(w.pool.reproductionBias.size() == n);
    CHECK(w.pool.genome.size() == static_cast<std::size_t>(n) * kGenomeLen);
    CHECK(w.pool.program.size() == n);
    CHECK(w.pool.vmState.size() == n);
    CHECK(w.pool.vmExecuted.size() == n);
    CHECK(w.pool.wx.size() == n);
    CHECK(w.pool.preyId.size() == n);
    CHECK(w.pool.plantIntake.size() == n);
    CHECK(w.pool.animalIntake.size() == n);
    CHECK(w.pool.action.size() == n);

    std::unordered_set<std::uint64_t> ids;
    for (std::uint32_t i = 0; i < n; ++i) {
        CHECK(ids.insert(w.pool.id[i]).second);
        CHECK(w.pool.read().px[i] >= 0.0f);
        CHECK(w.pool.read().px[i] < w.cfg.worldW);
        CHECK(w.pool.read().py[i] >= 0.0f);
        CHECK(w.pool.read().py[i] < w.cfg.worldH);
        CHECK(std::isfinite(w.pool.energy[i]));
        CHECK(w.pool.bodyEnergy[i] > 0.0f);
        CHECK(w.pool.energy[i] <= w.pool.eMax[i] * 1.31f);
        CHECK(w.pool.program[i].size() >= genetics::kVmOutputs);
        CHECK(w.pool.program[i].size() <= genetics::kVmMaxInstructions);
    }
}

TEST_CASE("the enlarged world preserves ecological resolution and founder density") {
    const SimConfig config;
    CHECK(config.worldW / static_cast<float>(config.eco.fieldW) ==
          doctest::Approx(1728.0f / 144.0f));
    CHECK(config.worldH / static_cast<float>(config.eco.fieldH) ==
          doctest::Approx(1152.0f / 96.0f));
    CHECK(static_cast<double>(config.initialAgents) / (config.worldW * config.worldH) ==
          doctest::Approx(1800.0 / (1728.0 * 1152.0)));
    CHECK(config.initialAgents < config.maxAgents);
}

TEST_CASE("same seed, same universe") {
    World a, b;
    run(a, 500);
    run(b, 500);
    REQUIRE(a.pool.count() == b.pool.count());
    double ea = 0, eb = 0, xa = 0, xb = 0;
    for (std::uint32_t i = 0; i < a.pool.count(); ++i) {
        ea += static_cast<double>(a.pool.energy[i]);
        eb += static_cast<double>(b.pool.energy[i]);
        xa += static_cast<double>(a.pool.read().px[i]);
        xb += static_cast<double>(b.pool.read().px[i]);
    }
    CHECK(ea == doctest::Approx(eb));
    CHECK(xa == doctest::Approx(xb));
    CHECK(a.stats.births == b.stats.births);
    CHECK(a.stats.plantEnergy == doctest::Approx(b.stats.plantEnergy));
    CHECK(a.stats.animalEnergy == doctest::Approx(b.stats.animalEnergy));
}

TEST_CASE("different seeds diverge") {
    SimConfig c1, c2;
    c2.seed = 12345;
    World a(c1), b(c2);
    run(a, 300);
    run(b, 300);
    CHECK(a.stats.births != b.stats.births);
}

TEST_CASE("ecosystem is alive after two simulated minutes: births, kills and several species") {
    World w;
    run(w, 2400);
    CHECK(w.pool.count() > 100);
    CHECK(w.stats.births > 100);
    CHECK(w.stats.kills > 0);
    CHECK(w.stats.plantEnergy > 0.0);
    CHECK(w.stats.animalEnergy > 0.0);
    const double balance = w.energyInventory() + w.stats.dissipatedEnergy -
                           w.stats.initialEnergy - w.stats.externalEnergy;
    CHECK(std::abs(balance) < w.stats.initialEnergy * 1e-4);
    std::size_t alive = 0;
    for (const auto& s : w.species.all()) alive += s.alive && s.count >= genetics::kEstablished;
    CHECK(alive >= 3);
}

TEST_CASE("telemetry counts established species from the live population") {
    World w;
    run(w, 101);
    std::vector<std::uint32_t> counts(w.species.all().size(), 0);
    for (std::uint32_t i = 0; i < w.pool.count(); ++i) ++counts[w.pool.species[i]];
    std::uint32_t expected = 0;
    for (const auto count : counts) expected += count >= genetics::kEstablished;
    CHECK(telemetry::collect(w).speciesCount == expected);
}

TEST_CASE("initial population begins as standing variation within one species") {
    World w;
    REQUIRE(w.species.all().size() == 1);
    REQUIRE(w.pool.count() > 0);
    for (std::uint32_t i = 0; i < w.pool.count(); ++i) CHECK(w.pool.species[i] == 0);
}

TEST_CASE("environment noise is not periodic: seasons drift") {
    World w;
    run(w, 1);
    double lastPhase = w.seasonPhase;
    std::vector<double> steps;
    ThreadPool pool(1);
    sim::Scheduler s(w, pool);
    for (int i = 0; i < 4000; ++i) {
        s.stepOnce();
        if (i % 5 == 4) {
            steps.push_back(w.seasonPhase - lastPhase);
            lastPhase = w.seasonPhase;
        }
    }
    const auto [lo, hi] = std::minmax_element(steps.begin(), steps.end());
    CHECK(*hi - *lo > 1e-3);  // a perfect oscillator would have identical phase increments
}
