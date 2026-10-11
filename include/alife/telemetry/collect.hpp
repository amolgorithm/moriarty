#pragma once
#include "alife/core/world.hpp"
#include "alife/telemetry/metrics.hpp"

namespace alife::telemetry {

// Population, established species count, Shannon entropy over species abundance.
Telemetry collect(const World& world);

}  // namespace alife::telemetry
