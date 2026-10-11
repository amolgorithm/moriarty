#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace alife::math {

// Information measures over frequency distributions (genotype, species, trait-bin counts).
// All logs are base 2 (bits). These are observational: they read the sim, never steer it.

inline double total(std::span<const std::uint32_t> c) noexcept {
    double t = 0.0;
    for (auto v : c) t += static_cast<double>(v);
    return t;
}

// Shannon entropy H = -sum p log2 p of a histogram of counts.
inline double shannonBits(std::span<const std::uint32_t> counts) noexcept {
    const double t = total(counts);
    if (t <= 0.0) return 0.0;
    double h = 0.0;
    for (auto c : counts) {
        if (c == 0) continue;
        const double p = static_cast<double>(c) / t;
        h -= p * std::log2(p);
    }
    return h;
}

// Renyi entropy of order alpha. alpha -> 1 recovers Shannon, alpha = 2 is collision entropy,
// alpha -> inf is min-entropy (dominated by the most common type).
inline double renyiBits(std::span<const std::uint32_t> counts, double alpha) noexcept {
    const double t = total(counts);
    if (t <= 0.0) return 0.0;
    if (std::abs(alpha - 1.0) < 1e-9) return shannonBits(counts);
    if (std::isinf(alpha)) {
        const double m = static_cast<double>(*std::max_element(counts.begin(), counts.end()));
        return -std::log2(m / t);
    }
    double s = 0.0;
    for (auto c : counts)
        if (c) s += std::pow(static_cast<double>(c) / t, alpha);
    return std::log2(s) / (1.0 - alpha);
}

// Hill number (effective number of types) of order q: exp of the Renyi entropy, in base 2 here.
// q = 0 richness, q = 1 exp(Shannon), q = 2 inverse Simpson. Comparable across communities.
inline double hillNumber(std::span<const std::uint32_t> counts, double q) noexcept {
    return std::exp2(renyiBits(counts, q));
}

// Simpson diversity 1 - sum p^2: chance two random individuals belong to different types.
inline double simpsonDiversity(std::span<const std::uint32_t> counts) noexcept {
    const double t = total(counts);
    if (t <= 0.0) return 0.0;
    double s = 0.0;
    for (auto c : counts) s += (static_cast<double>(c) / t) * (static_cast<double>(c) / t);
    return 1.0 - s;
}

// Kullback-Leibler divergence D(p || q) in bits. Terms with p = 0 vanish, q = 0 under p > 0 is inf.
inline double klBits(std::span<const double> p, std::span<const double> q) noexcept {
    double d = 0.0;
    for (std::size_t i = 0; i < p.size() && i < q.size(); ++i) {
        if (p[i] <= 0.0) continue;
        if (q[i] <= 0.0) return INFINITY;
        d += p[i] * std::log2(p[i] / q[i]);
    }
    return d;
}

// Jensen-Shannon divergence, symmetric and bounded by 1 bit. Used as a distance between
// community compositions at two times (how far has the ecosystem moved?).
inline double jensenShannonBits(std::span<const double> p, std::span<const double> q) {
    std::vector<double> m(std::min(p.size(), q.size()));
    for (std::size_t i = 0; i < m.size(); ++i) m[i] = 0.5 * (p[i] + q[i]);
    return 0.5 * klBits(p.first(m.size()), m) + 0.5 * klBits(q.first(m.size()), m);
}

// Mutual information I(X;Y) in bits from a row-major joint count table (nx rows, ny cols).
inline double mutualInformationBits(std::span<const std::uint32_t> joint, std::size_t nx, std::size_t ny) {
    const double t = total(joint);
    if (t <= 0.0 || joint.size() < nx * ny) return 0.0;
    std::vector<double> px(nx, 0.0), py(ny, 0.0);
    for (std::size_t i = 0; i < nx; ++i)
        for (std::size_t j = 0; j < ny; ++j) {
            const double v = static_cast<double>(joint[i * ny + j]) / t;
            px[i] += v;
            py[j] += v;
        }
    double mi = 0.0;
    for (std::size_t i = 0; i < nx; ++i)
        for (std::size_t j = 0; j < ny; ++j) {
            const double v = static_cast<double>(joint[i * ny + j]) / t;
            if (v > 0.0) mi += v * std::log2(v / (px[i] * py[j]));
        }
    return mi;
}

// Miller-Madow bias-corrected Shannon entropy: H + (K - 1) / (2 N ln 2), K = occupied bins.
inline double shannonBitsMillerMadow(std::span<const std::uint32_t> counts) noexcept {
    const double t = total(counts);
    if (t <= 0.0) return 0.0;
    std::size_t k = 0;
    for (auto c : counts) k += c > 0;
    return shannonBits(counts) + static_cast<double>(k - 1) / (2.0 * t * std::log(2.0));
}

}  // namespace alife::math
