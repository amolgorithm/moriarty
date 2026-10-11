#pragma once
#include <span>

namespace alife::math {

// Observational only. Fit species frequency series against the replicator
// equation, it does not drive the sim (selection is endogenous).
// TODO(phase 7): payoff estimation from per-species net energy flux.
void replicatorStep(std::span<double> x, std::span<const double> payoff, double alpha, double dt);

}  // namespace alife::math
