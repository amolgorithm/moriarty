#pragma once
#include "alife/core/world.hpp"

namespace alife::sim {

// Macro tick (5 s): species bookkeeping, predation-matrix decay, re-seeding on collapse.
// Synchronous for now. Phase 4 moves this onto its own worker thread against a snapshot.
void macroStep(World& world);

// Placeholder for the async worker interface (phase 4).
class EvoWorker {
   public:
    void submit();
    bool poll();
};

}  // namespace alife::sim
