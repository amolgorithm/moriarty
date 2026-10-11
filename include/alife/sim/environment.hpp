#pragma once
#include "alife/core/world.hpp"

namespace alife::sim {

// Meso-rate environment update: irregular seasons, resource weather (OU), random
// blights and blooms, stochastic currents, logistic regrowth, heat diffusion.
void stepEnvironment(World& world, float dt);

}  // namespace alife::sim
