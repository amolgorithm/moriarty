#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace alife::math {

// Information-dynamics measures on discretized time series (Lizier; Schreiber 2000).
// Plug-in (histogram) estimators with history length k. They are biased upward for short
// series: compare against a shuffled surrogate, see surrogateGap. Run on an analysis thread,
// never on the sim path.

// Active information storage A_X = I(x_{t+1} ; x_t^{(k)}): how much of a process's next
// state is predictable from its own past. Bits.
double activeInformationStorage(std::span<const std::uint8_t> x, std::size_t alphabet, std::size_t k = 1);

// Transfer entropy T_{Y->X} = I(x_{t+1} ; y_t^{(l)} | x_t^{(k)}): extra predictability of X
// from the past of Y beyond X's own past. Directional, so T_{Y->X} != T_{X->Y}. Bits.
double transferEntropy(std::span<const std::uint8_t> source, std::span<const std::uint8_t> target,
                       std::size_t alphabet, std::size_t k = 1, std::size_t l = 1);

// TE minus the mean TE over `shuffles` circular shifts of the source (a cheap surrogate that
// destroys cross-correlation but keeps the marginal). Near 0 means no detectable coupling.
double surrogateCorrectedTE(std::span<const std::uint8_t> source, std::span<const std::uint8_t> target,
                            std::size_t alphabet, std::size_t shuffles, std::uint64_t seed);

// Quantize a real series into `bins` equal-width symbols over its own range.
void quantize(std::span<const float> in, std::span<std::uint8_t> out, std::size_t bins);

}  // namespace alife::math
