#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "alife/core/rng.hpp"

namespace alife::genetics {

inline constexpr std::size_t kCircuitInputs = 16;
inline constexpr std::size_t kCircuitOutputs = 6;
inline constexpr std::size_t kMaxCircuitUnits = 96;
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
    Count
};

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
void applyRewardModulatedPlasticity(std::span<const UnitGene> circuit,
                                    std::span<const float> eligibility, float reward,
                                    std::span<float> effectiveWeights);
Circuit makeSeedCircuit();
void repairCircuit(Circuit& circuit, CounterRng& rng);

}  // namespace alife::genetics
