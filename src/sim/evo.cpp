#include "alife/sim/evo.hpp"

#include <algorithm>
#include <iterator>

#include "alife/genetics/archetypes.hpp"
#include "alife/genetics/gpm.hpp"
#include "alife/genetics/operators.hpp"

namespace alife::sim {

void macroStep(World& w) {
    w.species.refresh(w.pool, w.cfg.worldW, w.cfg.worldH, w.tick, w.pendingMarkers, &w.lineage);

    for (auto it = w.predation.begin(); it != w.predation.end();) {
        it->second *= 0.92f;
        it = it->second < 0.05f ? w.predation.erase(it) : std::next(it);
    }

    // immigration: the arena is one patch of a larger metacommunity. Now and then a small
    // group of a founder lineage that has gone locally rare drifts in from outside.
    {
        CounterRng rng(w.cfg.seed, w.tick, 0x1331ull);
        if (rng.uniform() < 0.05f) {
            int pick = -1;
            std::uint32_t lowest = 1u << 30;
            for (int a = 0; a < genetics::kArchetypes; ++a) {
                const auto& sp = w.species.all()[static_cast<std::size_t>(a)];
                const std::uint32_t c = sp.alive ? sp.count : 0;
                if (c < lowest) { lowest = c; pick = a; }
            }
            if (pick >= 0 && lowest < 40) {
                const auto id = static_cast<std::uint32_t>(pick);
                if (!w.species.all()[id].alive) w.species.revive(id, w.tick);
                float base[kGenomeLen];
                genetics::archetypeGenome(pick, base);
                const float cx = rng.uniform() * w.cfg.worldW, cy = rng.uniform() * w.cfg.worldH;
                const int cnt = 14 + static_cast<int>(rng.next() % 10);
                for (int i = 0; i < cnt; ++i) {
                    float g[kGenomeLen];
                    std::copy(base, base + kGenomeLen, g);
                    genetics::mutate(g, 0.02f, rng);
                    const auto ph = genetics::decode(g, w.cfg.eco.vTop);
                    w.addAgent(g, wrap(cx + 35.0f * rng.normal(), w.cfg.worldW), wrap(cy + 35.0f * rng.normal(), w.cfg.worldH),
                               0.7f * ph.eMax, id, 0, 0);
                }
                w.stats.reseeded += static_cast<std::uint64_t>(cnt);
                w.pendingMarkers.push_back("immigration: " + w.species.all()[id].shortName());
            }
        }
    }

    // keep the demo alive: if the ecosystem collapses, seed a few new colonists
    if (w.pool.count() < 120) {
        CounterRng rng(w.cfg.seed, w.tick, 0x5EEDull);
        const int a = static_cast<int>(rng.next() % genetics::kArchetypes);
        float base[kGenomeLen];
        genetics::archetypeGenome(a, base);
        const float cx = rng.uniform() * w.cfg.worldW, cy = rng.uniform() * w.cfg.worldH;
        for (int i = 0; i < 50; ++i) {
            float g[kGenomeLen];
            std::copy(base, base + kGenomeLen, g);
            genetics::mutate(g, 0.03f, rng);
            const auto ph = genetics::decode(g, w.cfg.eco.vTop);
            const std::uint32_t sp = w.species.classify(g, w.species.all().size() > static_cast<std::size_t>(a) ? static_cast<std::uint32_t>(a) : 0u, w.tick, rng);
            w.addAgent(g, wrap(cx + 40.0f * rng.normal(), w.cfg.worldW), wrap(cy + 40.0f * rng.normal(), w.cfg.worldH),
                       0.6f * ph.eMax, sp, 0, 0);
            ++w.stats.reseeded;
        }
        w.pendingMarkers.push_back("recolonization");
    }
}

void EvoWorker::submit() {}
bool EvoWorker::poll() { return false; }

}  // namespace alife::sim
