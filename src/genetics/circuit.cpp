#include "alife/genetics/circuit.hpp"

#include <algorithm>
#include <cmath>

namespace alife::genetics {
namespace {

float readSource(std::size_t source, std::size_t unitIndex, std::span<const float> units,
                 const CircuitInputs& inputs) {
    if (source < kCircuitInputs) return inputs[source];
    const std::size_t index = source - kCircuitInputs;
    return index < unitIndex ? units[index] : 0.0f;
}

}  // namespace

CircuitEvaluation evaluateCircuit(std::span<const UnitGene> circuit, const CircuitInputs& inputs,
                                  std::span<const float> previousUnits,
                                  std::span<const float> effectiveWeights) {
    CircuitEvaluation result;
    result.units.resize(circuit.size());
    result.eligibility.resize(circuit.size());
    std::array<std::size_t, kCircuitOutputs> counts{};
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        const UnitGene& gene = circuit[i];
        const float a = readSource(gene.sourceA, i, result.units, inputs);
        const float b = readSource(gene.sourceB, i, result.units, inputs);
        float value = 0.0f;
        switch (gene.op) {
            case UnitOp::Identity: value = a; break;
            case UnitOp::Add: value = 0.5f * (a + b); break;
            case UnitOp::Multiply: value = 2.0f * a * b; break;
            case UnitOp::Subtract: value = 0.5f * (a - b); break;
            case UnitOp::Minimum: value = std::min(a, b); break;
            case UnitOp::Maximum: value = std::max(a, b); break;
            case UnitOp::Absolute: value = std::fabs(a); break;
            case UnitOp::Tanh: value = std::tanh(a + b); break;
            case UnitOp::Memory: {
                const float previous = i < previousUnits.size() ? previousUnits[i] : 0.0f;
                value = 0.8f * previous + 0.2f * std::tanh(a + b);
                break;
            }
            case UnitOp::Count: value = 0.0f; break;
        }
        result.eligibility[i] = value;
        const float weight = i < effectiveWeights.size() ? effectiveWeights[i] : gene.weight;
        value = std::clamp(value * weight + gene.bias, -1.0f, 1.0f);
        result.units[i] = value;
        if (gene.output < kCircuitOutputs) {
            result.outputs[gene.output] += value;
            ++counts[gene.output];
        }
    }
    for (std::size_t i = 0; i < kCircuitOutputs; ++i) {
        if (counts[i] != 0)
            result.outputs[i] = std::clamp(result.outputs[i] / static_cast<float>(counts[i]), -1.0f, 1.0f);
    }
    return result;
}

void applyRewardModulatedPlasticity(std::span<const UnitGene> circuit,
                                    std::span<const float> eligibility, float reward,
                                    std::span<float> effectiveWeights) {
    const std::size_t n = std::min({circuit.size(), eligibility.size(), effectiveWeights.size()});
    for (std::size_t i = 0; i < n; ++i) {
        effectiveWeights[i] = std::clamp(
            effectiveWeights[i] + circuit[i].plasticity * reward * eligibility[i], -2.0f, 2.0f);
    }
}

Circuit makeSeedCircuit() {
    Circuit circuit(4);
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        UnitGene& gene = circuit[i];
        gene.op = UnitOp::Identity;
        gene.sourceA = static_cast<std::uint16_t>(i == 2 ? 8 : (i == 3 ? 9 : i));
        gene.sourceB = 0;
        gene.output = static_cast<std::uint8_t>(i);
        gene.weight = 1.0f;
        gene.bias = 0.0f;
        gene.plasticity = 0.08f;
    }
    return circuit;
}

void repairCircuit(Circuit& circuit, CounterRng& rng) {
    if (circuit.size() > kMaxCircuitUnits) circuit.resize(kMaxCircuitUnits);
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        UnitGene& gene = circuit[i];
        if (static_cast<std::size_t>(gene.op) >= static_cast<std::size_t>(UnitOp::Count))
            gene.op = UnitOp::Identity;
        const std::size_t sourceCount = kCircuitInputs + i;
        if (gene.sourceA >= sourceCount) gene.sourceA = static_cast<std::uint16_t>(rng.next() % sourceCount);
        if (gene.sourceB >= sourceCount) gene.sourceB = static_cast<std::uint16_t>(rng.next() % sourceCount);
        if (gene.output >= kCircuitOutputs && gene.output != kNoCircuitOutput) gene.output = kNoCircuitOutput;
        gene.weight = std::clamp(gene.weight, -2.0f, 2.0f);
        gene.bias = std::clamp(gene.bias, -1.0f, 1.0f);
        gene.plasticity = std::clamp(gene.plasticity, 0.0f, 0.25f);
    }
}

}  // namespace alife::genetics
