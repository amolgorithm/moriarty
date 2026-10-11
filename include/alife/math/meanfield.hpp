#pragma once
#include <cstddef>
#include <vector>

namespace alife::math {

// Mean-field reference model. The agent-based sim is the ground truth; this is the null
// model it is compared against: what would a well-mixed population do with the same
// per-capita rates? Gaps between the two measure what space and individuality add.

// Generalized Lotka-Volterra: dx_i/dt = x_i (r_i + sum_j A_ij x_j). Species abundance form.
struct GeneralizedLV {
    std::size_t n = 0;
    std::vector<double> r;  // intrinsic growth (negative for consumers without prey)
    std::vector<double> A;  // row-major interaction matrix, A_ii < 0 gives self-limitation

    void rhs(const std::vector<double>& x, std::vector<double>& dx) const;
    void stepRK4(std::vector<double>& x, double dt) const;  // clamps at 0

    // Interior fixed point x* solving r + A x = 0 by Gaussian elimination. ok=false if singular.
    std::vector<double> equilibrium(bool& ok) const;
};

// Consumer-resource with Holling type II response, the mean-field cousin of the sim's
// grazing/predation loop: resource R, consumer C.
//   dR/dt = g R (1 - R/K) - a R C / (1 + a h R)
//   dC/dt = e a R C / (1 + a h R) - m C
struct ConsumerResource {
    double g = 0.35, K = 16.0, a = 0.05, h = 1.0, e = 0.4, m = 0.1;
    void stepRK4(double& R, double& C, double dt) const;
};

// Interface for replacing the spatial grid with a density field in over-threshold cells.
class DensityField {
   public:
    virtual ~DensityField() = default;
    virtual double density(std::size_t cell, std::size_t species) const = 0;
    virtual void advance(double dt) = 0;
};

}  // namespace alife::math
