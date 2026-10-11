#pragma once

namespace alife::sim {

// Macro-scale worker: crossover, mutation, speciation, lineage.
// Runs on its own thread against an immutable snapshot (phase 4).
class EvoWorker {
   public:
    void submit();       // TODO(phase 4): take an EvoSnapshot
    bool poll();         // true when a finished batch is ready to apply at the barrier
};

}  // namespace alife::sim
