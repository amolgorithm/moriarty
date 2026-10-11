#include "alife/math/te.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#include "alife/core/rng.hpp"

namespace alife::math {
namespace {

std::uint64_t pack(std::span<const std::uint8_t> s, std::size_t end, std::size_t len, std::size_t A) {
    std::uint64_t key = 0;
    for (std::size_t j = 0; j < len; ++j) key = key * A + s[end - j];
    return key;
}

using Count = std::unordered_map<std::uint64_t, std::uint32_t>;

double entropyOf(const Count& c, double n) {
    double h = 0.0;
    for (const auto& [k, v] : c) {
        const double p = static_cast<double>(v) / n;
        h -= p * std::log2(p);
    }
    return h;
}

}  // namespace

double activeInformationStorage(std::span<const std::uint8_t> x, std::size_t A, std::size_t k) {
    if (x.size() <= k + 1) return 0.0;
    Count past, next, joint;
    double n = 0.0;
    for (std::size_t t = k - 1; t + 1 < x.size(); ++t) {
        const std::uint64_t p = pack(x, t, k, A), q = x[t + 1];
        ++past[p]; ++next[q]; ++joint[p * A + q];
        n += 1.0;
    }
    return entropyOf(past, n) + entropyOf(next, n) - entropyOf(joint, n);
}

double transferEntropy(std::span<const std::uint8_t> y, std::span<const std::uint8_t> x, std::size_t A,
                       std::size_t k, std::size_t l) {
    const std::size_t len = std::min(x.size(), y.size()), m = std::max(k, l);
    if (len <= m + 1) return 0.0;
    // T = H(x+, xk) + H(xk, yl) - H(xk) - H(x+, xk, yl)
    Count xk, xnxk, xkyl, all;
    double n = 0.0;
    for (std::size_t t = m - 1; t + 1 < len; ++t) {
        const std::uint64_t a = pack(x, t, k, A), b = pack(y, t, l, A), c = x[t + 1];
        const std::uint64_t ab = a * static_cast<std::uint64_t>(std::pow(A, l)) + b;
        ++xk[a]; ++xnxk[a * A + c]; ++xkyl[ab]; ++all[ab * A + c];
        n += 1.0;
    }
    return entropyOf(xnxk, n) + entropyOf(xkyl, n) - entropyOf(xk, n) - entropyOf(all, n);
}

double surrogateCorrectedTE(std::span<const std::uint8_t> y, std::span<const std::uint8_t> x, std::size_t A,
                            std::size_t shuffles, std::uint64_t seed) {
    const double raw = transferEntropy(y, x, A);
    if (shuffles == 0 || y.size() < 8) return raw;
    CounterRng rng(seed, 0, 0x7E);
    std::vector<std::uint8_t> sh(y.size());
    double mean = 0.0;
    for (std::size_t s = 0; s < shuffles; ++s) {
        const std::size_t shift = 4 + rng.next() % (y.size() - 8);
        for (std::size_t i = 0; i < y.size(); ++i) sh[i] = y[(i + shift) % y.size()];
        mean += transferEntropy(sh, x, A);
    }
    return raw - mean / static_cast<double>(shuffles);
}

void quantize(std::span<const float> in, std::span<std::uint8_t> out, std::size_t bins) {
    if (in.empty() || bins < 2) return;
    const auto [lo, hi] = std::minmax_element(in.begin(), in.end());
    const float range = std::max(1e-12f, *hi - *lo);
    for (std::size_t i = 0; i < in.size() && i < out.size(); ++i)
        out[i] = static_cast<std::uint8_t>(std::min<std::size_t>(bins - 1, static_cast<std::size_t>((in[i] - *lo) / range * static_cast<float>(bins))));
}

}  // namespace alife::math
