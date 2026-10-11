#include "alife/sim/commands.hpp"

namespace alife::sim {

void applyCommands(World&, std::vector<CommandBuffer>& buffers) {
    // TODO(phase 2): resolve ids, apply in slot order, swapRemove for kills
    for (auto& b : buffers) b.clear();
}

}  // namespace alife::sim
