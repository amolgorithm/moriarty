#pragma once
#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"
#include "alife/sim/commands.hpp"
#include "alife/sim/evo.hpp"

namespace alife::sim {

// Fixed-timestep accumulator driving the micro / meso / macro cadence.
class Scheduler {
   public:
    Scheduler(World& world, ThreadPool& pool) : world_(world), pool_(pool) {}

    void advance(double frameDt);  // may run several micro ticks, capped to avoid a spiral
    void stepOnce();               // one micro tick plus any due meso / evo work
    double alpha() const noexcept { return acc_ / world_.cfg.dtPhys; }  // render interpolation

   private:
    World& world_;
    ThreadPool& pool_;
    EvoWorker evo_;
    std::vector<CommandBuffer> cmds_;
    double acc_ = 0.0;
};

}  // namespace alife::sim
