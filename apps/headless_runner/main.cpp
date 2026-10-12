#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <vector>

#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"
#include "alife/sim/scheduler.hpp"
#include "alife/telemetry/collect.hpp"

// Usage: moriarty_headless [seconds_of_sim_time] [report_every_s]
int main(int argc, char** argv) {
    const double total = argc > 1 ? std::atof(argv[1]) : 600.0;
    const double every = argc > 2 ? std::atof(argv[2]) : 30.0;

    alife::World world;
    alife::ThreadPool pool;
    alife::sim::Scheduler sched(world, pool);
    const float dt = world.cfg.dtPhys;
    const auto ticks = static_cast<std::uint64_t>(total / static_cast<double>(dt));
    const auto step = static_cast<std::uint64_t>(every / static_cast<double>(dt));

    std::printf("%7s %6s %5s %5s %6s %6s %6s %7s %7s %7s %6s %6s %8s %8s %6s %6s %6s %10s\n",
                "t[s]", "pop", "taxa", "spp8+", "graze", "omni", "pred", "births", "kills", "starve", "H",
                "units", "food E", "prey E", "forage", "hunt", "evade", "balance");
    for (std::uint64_t t = 1; t <= ticks; ++t) {
        sched.stepOnce();
        if (t % step == 0) {
            using alife::genetics::Role;
            int role[3] = {0, 0, 0};
            std::uint32_t action[5] = {};
            std::vector<std::uint32_t> liveCounts(world.species.all().size(), 0);
            for (std::uint32_t i = 0; i < world.pool.count(); ++i) ++liveCounts[world.pool.species[i]];
            std::uint32_t extantTaxa = 0;
            for (std::size_t i = 0; i < liveCounts.size(); ++i) {
                const auto count = liveCounts[i];
                if (count == 0) continue;
                ++extantTaxa;
                role[static_cast<int>(world.species.all()[i].role)] += static_cast<int>(count);
            }
            for (std::uint32_t i = 0; i < world.pool.count(); ++i)
                ++action[std::min<std::size_t>(world.pool.action[i], std::size(action) - 1)];
            const auto tel = alife::telemetry::collect(world);
            const double balance = world.energyInventory() + world.stats.dissipatedEnergy -
                                   world.stats.initialEnergy - world.stats.externalEnergy;
            std::printf("%7.0f %6u %5u %5u %6d %6d %6d %7llu %7llu %7llu %6.2f %6.1f %8.0f %8.0f %6u %6u %6u %10.3g\n",
                        static_cast<double>(t) * static_cast<double>(dt),
                        tel.population, extantTaxa, tel.speciesCount, role[0], role[1], role[2],
                        static_cast<unsigned long long>(world.stats.births), static_cast<unsigned long long>(world.stats.kills),
                        static_cast<unsigned long long>(world.stats.starved), static_cast<double>(tel.shannonBits),
                        static_cast<double>(tel.meanCircuitUnits), world.stats.plantEnergy,
                        world.stats.animalEnergy, action[static_cast<std::size_t>(alife::AgentAction::Forage)],
                        action[static_cast<std::size_t>(alife::AgentAction::Hunt)],
                        action[static_cast<std::size_t>(alife::AgentAction::Evade)], balance);
        }
    }
    std::vector<std::uint32_t> liveCounts(world.species.all().size(), 0);
    for (std::uint32_t i = 0; i < world.pool.count(); ++i) ++liveCounts[world.pool.species[i]];
    std::vector<std::uint32_t> extant;
    for (std::size_t i = 0; i < liveCounts.size(); ++i)
        if (liveCounts[i] > 0) extant.push_back(static_cast<std::uint32_t>(i));
    std::sort(extant.begin(), extant.end(),
              [&liveCounts](std::uint32_t a, std::uint32_t b) { return liveCounts[a] > liveCounts[b]; });
    const std::size_t shown = std::min<std::size_t>(25, extant.size());
    std::printf("\nextant taxa at end: %zu total; showing up to 25\n", extant.size());
    for (std::size_t i = 0; i < shown; ++i) {
        const auto& sp = world.species.all()[extant[i]];
        std::printf("  %-22s n=%-5u units=%.1f aggr=%.2f size=%.1f speed=%.0f def=%.2f %s\n",
                    sp.fullName().c_str(), liveCounts[extant[i]], static_cast<double>(sp.meanUnits),
                    static_cast<double>(sp.meanAggr), static_cast<double>(sp.meanSize), static_cast<double>(sp.meanSpeed),
                    static_cast<double>(sp.meanDef), sp.parent == alife::genetics::SpeciesTable::kNone ? "founder" : "derived");
    }
    double learnedDelta = 0.0;
    std::size_t learnedUnits = 0, memoryUnits = 0;
    for (std::uint32_t i = 0; i < world.pool.count(); ++i) {
        const auto& circuit = world.pool.circuit[i];
        for (std::size_t u = 0; u < circuit.size(); ++u) {
            learnedDelta += std::abs(world.pool.circuitWeights[i][u] - circuit[u].weight);
            ++learnedUnits;
            memoryUnits += circuit[u].op == alife::genetics::UnitOp::Memory;
        }
    }
    std::printf("subunit adaptation: mean |learned-inherited weight| %.4f; memory units %zu\n",
                learnedUnits ? learnedDelta / static_cast<double>(learnedUnits) : 0.0, memoryUnits);
    const double balance = world.energyInventory() + world.stats.dissipatedEnergy -
                           world.stats.initialEnergy - world.stats.externalEnergy;
    std::printf("energy ledger: start %.3f + external %.3f = inventory %.3f + dissipated %.3f (residual %.3g)\n",
                world.stats.initialEnergy, world.stats.externalEnergy, world.energyInventory(),
                world.stats.dissipatedEnergy, balance);
    return 0;
}
