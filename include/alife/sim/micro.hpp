#pragma once
#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"

namespace alife::sim {

// Newtonian step: sensing, steering, Langevin integration, collisions.
// Reads pool.read(), writes pool.write().
void microStep(World& world, ThreadPool& pool);

}  // namespace alife::sim
