#pragma once
#include <cstddef>
#include <span>
#include <vector>

namespace alife::math {

// Replicator dynamics and the Price equation. Observational only: selection in Moriarty is
// endogenous (differential survival under energy constraints), so these are fitted to
// species-frequency series to ask "does a replicator model explain what the sim did?"

// One Euler step of dx_i/dt = x_i (f_i - <f>) on the simplex. Renormalizes against drift.
void replicatorStep(std::span<double> x, std::span<const double> payoff, double alpha, double dt);

// Matrix-game form f = A x (row-major n x n). Fills payoff, returns mean payoff <f>.
double gamePayoff(std::span<const double> A, std::span<const double> x, std::span<double> payoff);

// Price equation for one generation step. For parent-type fractions q_i with fitness w_i and
// trait z_i (child mean trait z'_i):
//   Delta zbar = Cov(w, z)/wbar        (selection)
//              + E(w Delta z)/wbar     (transmission bias)
struct PriceTerms {
    double selection = 0.0;
    double transmission = 0.0;
    double total() const noexcept { return selection + transmission; }
};
PriceTerms priceEquation(std::span<const double> q, std::span<const double> w,
                         std::span<const double> z, std::span<const double> zChild);

// Estimate relative fitness from a frequency time series. For each type, the slope of
// ln(x_i / x_ref) against time equals f_i - f_ref under the replicator equation.
// series is row-major [samples][types]; returns f_i - f_ref (0 for the reference type).
std::vector<double> fitRelativeFitness(std::span<const double> series, std::size_t samples,
                                       std::size_t types, double sampleDt, std::size_t reference);

// Predicted frequencies after integrating the replicator equation with constant fitness
// from x0 (RK4-free Euler, small dt). Returns the L1 gap to the observed final row, in [0, 2].
double replicatorPredictionGap(std::span<const double> series, std::size_t samples, std::size_t types,
                               double sampleDt, std::span<const double> fitness);

}  // namespace alife::math
