#pragma once
#include <cstdint>

#include "alife/core/world.hpp"
#include "alife/render/snapshot.hpp"

namespace alife::render {

// Copies what the renderer needs out of the World. Drains the event queue.
// selectedId: organism to fill SelectedVis for (0 = none).
void buildSnapshot(World& world, RenderSnapshot& out, std::uint64_t selectedId);

}  // namespace alife::render
