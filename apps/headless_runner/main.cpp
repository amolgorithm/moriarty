#include <cstdio>

#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"
#include "alife/sim/scheduler.hpp"

int main() {
    alife::World world;
    alife::ThreadPool pool;
    alife::sim::Scheduler sched(world, pool);

    // TODO(phase 1): seed agents, run N ticks, dump telemetry to csv
    for (int i = 0; i < 1000; ++i) sched.stepOnce();
    std::printf("tick %llu, agents %u, threads %zu\n", static_cast<unsigned long long>(world.tick),
                world.pool.count(), pool.threadCount());
    return 0;
}
