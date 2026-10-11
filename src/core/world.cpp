#include "alife/core/world.hpp"

namespace alife {

World::World(const SimConfig& c) : cfg(c), grid(c.worldW, c.worldH, c.cellSize) {
    pool.reserve(c.maxAgents);
}

}  // namespace alife
