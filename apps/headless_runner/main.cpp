#include <cstdio>
#include <cstdlib>
#include <algorithm>
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

    std::printf("%7s %6s %4s %6s %6s %6s %7s %7s %7s %6s %6s\n", "t[s]", "pop", "spp", "graze", "omni", "pred",
                "births", "kills", "starve", "H", "units");
    for (std::uint64_t t = 1; t <= ticks; ++t) {
        sched.stepOnce();
        if (t % step == 0) {
            using alife::genetics::Role;
            int role[3] = {0, 0, 0};
            for (const auto& s : world.species.all())
                if (s.alive && s.count >= alife::genetics::kEstablished) role[static_cast<int>(s.role)] += static_cast<int>(s.count);
            const auto tel = alife::telemetry::collect(world);
            std::printf("%7.0f %6u %4u %6d %6d %6d %7llu %7llu %7llu %6.2f %6.1f\n",
                        static_cast<double>(t) * static_cast<double>(dt),
                        tel.population, tel.speciesCount, role[0], role[1], role[2],
                        static_cast<unsigned long long>(world.stats.births), static_cast<unsigned long long>(world.stats.kills),
                        static_cast<unsigned long long>(world.stats.starved), static_cast<double>(tel.shannonBits),
                        static_cast<double>(tel.meanCircuitUnits));
        }
    }
    std::printf("\nspecies alive at end:\n");
    std::vector<const alife::genetics::Species*> extant;
    for (const auto& sp : world.species.all())
        if (sp.alive && sp.count >= 3) extant.push_back(&sp);
    std::sort(extant.begin(), extant.end(), [](const auto* a, const auto* b) { return a->count > b->count; });
    const std::size_t shown = std::min<std::size_t>(25, extant.size());
    for (std::size_t i = 0; i < shown; ++i) {
        const auto& sp = *extant[i];
        std::printf("  %-22s n=%-5u aggr=%.2f size=%.1f speed=%.0f def=%.2f %s\n", sp.fullName().c_str(), sp.count,
                    static_cast<double>(sp.meanAggr), static_cast<double>(sp.meanSize), static_cast<double>(sp.meanSpeed),
                    static_cast<double>(sp.meanDef), sp.parent == alife::genetics::SpeciesTable::kNone ? "founder" : "derived");
    }
    if (extant.size() > shown) std::printf("  ... and %zu more extant lineages\n", extant.size() - shown);
    return 0;
}
