#include "alife/sim/scheduler.hpp"

#include "alife/sim/evo.hpp"
#include "alife/sim/meso.hpp"
#include "alife/sim/micro.hpp"

namespace alife::sim {

void Scheduler::advance(double frameDt) {
    constexpr int kMaxCatchUp = 64;
    acc_ += frameDt;
    for (int n = 0; acc_ >= world_.cfg.dtPhys && n < kMaxCatchUp; ++n) {
        stepOnce();
        acc_ -= world_.cfg.dtPhys;
    }
}

void Scheduler::stepOnce() {
    auto& pool = world_.pool;
    world_.grid.rebuild(pool.read().px.data(), pool.read().py.data(), pool.count());
    microStep(world_, pool_);
    pool.swapKin();
    ++world_.tick;

    if (world_.tick % static_cast<std::uint64_t>(world_.cfg.mesoEvery) == 0) mesoStep(world_, pool_);
    if (world_.tick % static_cast<std::uint64_t>(world_.cfg.evoEvery) == 0) macroStep(world_);

    if (evo_.poll()) {
        // TODO(phase 4): apply finished offspring batch here, at the barrier
    }
    applyCommands(world_, cmds_);
}

}  // namespace alife::sim
