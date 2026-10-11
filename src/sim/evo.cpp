#include "alife/sim/evo.hpp"

#include <iterator>

namespace alife::sim {

void macroStep(World& w) {
    w.species.refresh(w.pool, w.cfg.worldW, w.cfg.worldH, w.tick, w.pendingMarkers, &w.lineage);

    for (auto it = w.predation.begin(); it != w.predation.end();) {
        it->second *= 0.92f;
        it = it->second < 0.05f ? w.predation.erase(it) : std::next(it);
    }

}

void EvoWorker::submit() {}
bool EvoWorker::poll() { return false; }

}  // namespace alife::sim
