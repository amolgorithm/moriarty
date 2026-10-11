#pragma once
#include <cmath>
#include <cstdint>
#include <span>

namespace alife::math {

// Shannon entropy in bits over a histogram of counts (genotype or species frequencies).
inline double shannonBits(std::span<const std::uint32_t> counts) noexcept {
    double total = 0.0;
    for (auto c : counts) total += static_cast<double>(c);
    if (total <= 0.0) return 0.0;
    double h = 0.0;
    for (auto c : counts) {
        if (c == 0) continue;
        const double p = static_cast<double>(c) / total;
        h -= p * std::log2(p);
    }
    return h;
}

}  // namespace alife::math
