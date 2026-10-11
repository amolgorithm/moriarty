#include "alife/math/replicator.hpp"

#include <algorithm>
#include <cmath>

namespace alife::math {

void replicatorStep(std::span<double> x, std::span<const double> f, double alpha, double dt) {
    const std::size_t n = std::min(x.size(), f.size());
    double mean = 0.0;
    for (std::size_t i = 0; i < n; ++i) mean += x[i] * f[i];
    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = std::max(0.0, x[i] + alpha * dt * x[i] * (f[i] - mean));
        sum += x[i];
    }
    if (sum > 0.0)
        for (std::size_t i = 0; i < n; ++i) x[i] /= sum;
}

double gamePayoff(std::span<const double> A, std::span<const double> x, std::span<double> payoff) {
    const std::size_t n = x.size();
    double mean = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        double f = 0.0;
        for (std::size_t j = 0; j < n; ++j) f += A[i * n + j] * x[j];
        payoff[i] = f;
        mean += x[i] * f;
    }
    return mean;
}

PriceTerms priceEquation(std::span<const double> q, std::span<const double> w, std::span<const double> z,
                         std::span<const double> zc) {
    const std::size_t n = std::min({q.size(), w.size(), z.size(), zc.size()});
    double wbar = 0.0, zbar = 0.0, wz = 0.0, wdz = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        wbar += q[i] * w[i];
        zbar += q[i] * z[i];
        wz += q[i] * w[i] * z[i];
        wdz += q[i] * w[i] * (zc[i] - z[i]);
    }
    PriceTerms t;
    if (wbar <= 0.0) return t;
    t.selection = (wz - wbar * zbar) / wbar;  // Cov(w, z) / wbar
    t.transmission = wdz / wbar;              // E(w dz) / wbar
    return t;
}

std::vector<double> fitRelativeFitness(std::span<const double> s, std::size_t samples, std::size_t types,
                                       double dt, std::size_t ref) {
    std::vector<double> out(types, 0.0);
    if (samples < 2 || ref >= types || s.size() < samples * types) return out;
    constexpr double eps = 1e-9;
    for (std::size_t i = 0; i < types; ++i) {
        if (i == ref) continue;
        // least-squares slope of y(t) = ln(x_i / x_ref), skipping samples where either is absent
        double st = 0, sy = 0, stt = 0, sty = 0, m = 0;
        for (std::size_t k = 0; k < samples; ++k) {
            const double a = s[k * types + i], b = s[k * types + ref];
            if (a < eps || b < eps) continue;
            const double t = static_cast<double>(k) * dt, y = std::log(a / b);
            st += t; sy += y; stt += t * t; sty += t * y; m += 1.0;
        }
        const double den = m * stt - st * st;
        if (m >= 3.0 && std::abs(den) > 1e-12) out[i] = (m * sty - st * sy) / den;
    }
    return out;
}

double replicatorPredictionGap(std::span<const double> s, std::size_t samples, std::size_t types, double dt,
                               std::span<const double> fitness) {
    if (samples < 2 || s.size() < samples * types || fitness.size() < types) return 2.0;
    std::vector<double> x(s.begin(), s.begin() + static_cast<std::ptrdiff_t>(types));
    const int sub = 20;
    for (std::size_t k = 1; k < samples; ++k)
        for (int j = 0; j < sub; ++j) replicatorStep(x, fitness, 1.0, dt / sub);
    double gap = 0.0;
    for (std::size_t i = 0; i < types; ++i) gap += std::abs(x[i] - s[(samples - 1) * types + i]);
    return gap;
}

}  // namespace alife::math
