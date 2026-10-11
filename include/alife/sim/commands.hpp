#pragma once
#include <cstdint>
#include <vector>

#include "alife/core/world.hpp"

namespace alife::sim {

// Workers never spawn, kill or transfer energy directly. They append commands
// to a thread-local buffer, merged and applied at the barrier in a fixed order.
enum class CmdKind : std::uint8_t { Spawn, Kill, Transfer };

struct Command {
    CmdKind kind = CmdKind::Kill;
    std::uint64_t a = 0;  // agent id (never an index)
    std::uint64_t b = 0;
    float amount = 0.0f;
};

using CommandBuffer = std::vector<Command>;

void applyCommands(World& world, std::vector<CommandBuffer>& buffers);

}  // namespace alife::sim
