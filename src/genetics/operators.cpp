#include "alife/genetics/operators.hpp"

#include <algorithm>

#include "alife/core/config.hpp"

namespace alife::genetics {

void mutate(float* g, float rate, CounterRng& rng) {
    for (std::size_t i = 0; i < kGenomeLen; ++i) {
        float sigma = rate;
        if (rng.uniform() < 0.04f) sigma *= 4.0f;  // occasional macro-mutation
        g[i] = std::clamp(g[i] + sigma * rng.normal(), 0.0f, 1.0f);
    }
}

void crossover(const float* a, const float* b, float* out, CounterRng& rng) {
    for (std::size_t i = 0; i < kGenomeLen; ++i) out[i] = rng.uniform() < 0.5f ? a[i] : b[i];
}

void mutateCircuit(Circuit& circuit, float rate, CounterRng& rng) {
    if (circuit.empty()) circuit = makeSeedCircuit();
    for (auto& gene : circuit) {
        if (rng.uniform() < rate * 0.55f)
            gene.op = static_cast<UnitOp>(rng.next() % static_cast<std::uint64_t>(UnitOp::Count));
        if (rng.uniform() < rate * 0.55f)
            gene.sourceA = static_cast<std::uint16_t>(rng.next() % (kCircuitInputs + (&gene - circuit.data())));
        if (rng.uniform() < rate * 0.55f)
            gene.sourceB = static_cast<std::uint16_t>(rng.next() % (kCircuitInputs + (&gene - circuit.data())));
        if (rng.uniform() < rate * 0.25f)
            gene.output = rng.uniform() < 0.5f ? kNoCircuitOutput :
                          static_cast<std::uint8_t>(rng.next() % kCircuitOutputs);
        gene.weight = std::clamp(gene.weight + rate * rng.normal(), -2.0f, 2.0f);
        gene.bias = std::clamp(gene.bias + rate * 0.5f * rng.normal(), -1.0f, 1.0f);
        gene.plasticity = std::clamp(gene.plasticity + rate * 0.2f * rng.normal(), 0.0f, 0.25f);
    }

    const float structureRate = std::clamp(0.38f + rate, 0.38f, 0.58f);
    if (rng.uniform() < structureRate) {
        const bool canInsert = circuit.size() < kMaxCircuitUnits;
        const bool canDelete = circuit.size() > 3;
        const bool insert = canInsert && (!canDelete || rng.uniform() < 0.5f);
        if (insert) {
            UnitGene gene;
            if (!circuit.empty() && rng.uniform() < 0.7f) {
                gene = circuit[static_cast<std::size_t>(rng.next() % circuit.size())];
            } else {
                gene.op = static_cast<UnitOp>(rng.next() % static_cast<std::uint64_t>(UnitOp::Count));
                const std::size_t sourceCount = kCircuitInputs + circuit.size();
                gene.sourceA = static_cast<std::uint16_t>(rng.next() % sourceCount);
                gene.sourceB = static_cast<std::uint16_t>(rng.next() % sourceCount);
                gene.output = rng.uniform() < 0.65f ? static_cast<std::uint8_t>(rng.next() % kCircuitOutputs)
                                                   : kNoCircuitOutput;
                gene.weight = (rng.uniform() * 2.0f - 1.0f) * 1.5f;
                gene.bias = (rng.uniform() * 2.0f - 1.0f) * 0.35f;
                gene.plasticity = rng.uniform() * 0.08f;
            }
            circuit.push_back(gene);
        } else if (canDelete) {
            const std::size_t removed = static_cast<std::size_t>(rng.next() % circuit.size());
            const std::size_t removedSource = kCircuitInputs + removed;
            for (std::size_t i = removed + 1; i < circuit.size(); ++i) {
                auto remap = [&](std::uint16_t& source) {
                    if (source == removedSource)
                        source = static_cast<std::uint16_t>(rng.next() % kCircuitInputs);
                    else if (source > removedSource)
                        --source;
                };
                remap(circuit[i].sourceA);
                remap(circuit[i].sourceB);
            }
            circuit.erase(circuit.begin() + static_cast<std::ptrdiff_t>(removed));
        }
    }
    repairCircuit(circuit, rng);
}

Circuit crossoverCircuit(std::span<const UnitGene> a, std::span<const UnitGene> b, CounterRng& rng) {
    if (a.empty()) return Circuit(b.begin(), b.begin() + static_cast<std::ptrdiff_t>(std::min(b.size(), kMaxCircuitUnits)));
    if (b.empty()) return Circuit(a.begin(), a.begin() + static_cast<std::ptrdiff_t>(std::min(a.size(), kMaxCircuitUnits)));

    const std::size_t shared = std::min(a.size(), b.size());
    Circuit child;
    child.reserve(std::min(std::max(a.size(), b.size()), kMaxCircuitUnits));
    for (std::size_t i = 0; i < shared && child.size() < kMaxCircuitUnits; ++i)
        child.push_back(rng.uniform() < 0.5f ? a[i] : b[i]);
    const auto tail = rng.uniform() < 0.5f ? a : b;
    for (std::size_t i = shared; i < tail.size() && child.size() < kMaxCircuitUnits; ++i) child.push_back(tail[i]);
    repairCircuit(child, rng);
    return child;
}

}  // namespace alife::genetics
