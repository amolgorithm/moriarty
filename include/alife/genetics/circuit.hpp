#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "alife/core/rng.hpp"

namespace alife::genetics {

inline constexpr std::size_t kCircuitInputs = 19;
inline constexpr std::size_t kCircuitOutputs = 8;
inline constexpr std::size_t kHuntStrategyOutput = 6;
inline constexpr std::size_t kReproductionStrategyOutput = 7;
inline constexpr std::size_t kMaxCircuitUnits = 256;
inline constexpr std::uint8_t kNoCircuitOutput = 0xFF;

enum class UnitOp : std::uint8_t {
    Identity,
    Add,
    Multiply,
    Subtract,
    Minimum,
    Maximum,
    Absolute,
    Tanh,
    Memory,
    GreaterThan,
    LessThan,
    Count
};

inline constexpr std::size_t kCircuitOperationCount = static_cast<std::size_t>(UnitOp::Count);

// A low-level heritable computational unit. Sources address sensory inputs first,
// then earlier units, making every expressed circuit a bounded, deterministic DAG.
struct UnitGene {
    UnitOp op = UnitOp::Identity;
    std::uint16_t sourceA = 0;
    std::uint16_t sourceB = 0;
    std::uint8_t output = kNoCircuitOutput;
    float weight = 1.0f;
    float bias = 0.0f;
    float plasticity = 0.0f;
};

using Circuit = std::vector<UnitGene>;
using CircuitInputs = std::array<float, kCircuitInputs>;

struct CircuitEvaluation {
    std::vector<float> units;
    std::vector<float> eligibility;
    std::array<float, kCircuitOutputs> outputs{};
};

CircuitEvaluation evaluateCircuit(std::span<const UnitGene> circuit, const CircuitInputs& inputs,
                                  std::span<const float> previousUnits = {},
                                  std::span<const float> effectiveWeights = {});
std::array<float, kCircuitOutputs> evaluateCircuitInPlace(
    std::span<const UnitGene> circuit, const CircuitInputs& inputs, std::span<float> unitState,
    std::span<float> eligibilityTrace, std::span<const float> effectiveWeights = {},
    float traceRetention = 0.0f);
std::size_t countOutputConnectedUnits(std::span<const UnitGene> circuit) noexcept;
void applyRewardModulatedPlasticity(std::span<const UnitGene> circuit,
                                    std::span<const float> eligibility, float reward,
                                    std::span<float> effectiveWeights);
Circuit makeSeedCircuit();
void repairCircuit(Circuit& circuit, CounterRng& rng);

}  // namespace alife::genetics
