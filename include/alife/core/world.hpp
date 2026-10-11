#pragma once
#include <cstdint>

#include "alife/core/agent_pool.hpp"
#include "alife/core/config.hpp"
#include "alife/core/grid.hpp"

namespace alife {

struct World {
    SimConfig cfg;
    AgentPool pool;
    ToroidalGrid grid;
    std::uint64_t tick = 0;

    explicit World(const SimConfig& c = {});
    // TODO(phase 2): resource field R(x), heat field Q(x)
};

}  // namespace alife
