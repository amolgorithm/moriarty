#pragma once
#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"

namespace alife::sim {

// Metabolism: uptake, upkeep, heat export, death and replication intents.
void mesoStep(World& world, ThreadPool& pool);

}  // namespace alife::sim
