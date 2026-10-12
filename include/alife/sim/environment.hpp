#pragma once
#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"

namespace alife::sim {

// Meso-rate environment update: irregular seasons, resource weather (OU), random
// blights and blooms, stochastic currents, logistic regrowth, heat diffusion.
void stepEnvironment(World& world, float dt, ThreadPool& threadPool);

}  // namespace alife::sim
