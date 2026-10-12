#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "alife/core/config.hpp"
#include "alife/core/rng.hpp"
#include "alife/genetics/circuit.hpp"
#include "alife/genetics/gpm.hpp"
#include "alife/genetics/operators.hpp"
#include "alife/genetics/speciation.hpp"

using namespace alife;
using namespace alife::genetics;

namespace {
void neutralGenome(float* genome) {
    std::fill(genome, genome + kGenomeLen, 0.5f);
}
}  // namespace

TEST_CASE("mutation keeps genes in range and actually changes the genome") {
    float g[kGenomeLen], orig[kGenomeLen];
    neutralGenome(g);
    std::copy(g, g + kGenomeLen, orig);
    CounterRng rng(1, 2, 3);
    for (int i = 0; i < 200; ++i) mutate(g, 0.05f, rng);
    bool changed = false;
    for (std::size_t i = 0; i < kGenomeLen; ++i) {
        CHECK(g[i] >= 0.0f);
        CHECK(g[i] <= 1.0f);
        changed = changed || g[i] != orig[i];
    }
    CHECK(changed);
}

TEST_CASE("crossover only mixes parental alleles") {
    float a[kGenomeLen], b[kGenomeLen], c[kGenomeLen];
    neutralGenome(a);
    neutralGenome(b);
    std::fill(b, b + kGenomeLen, 0.9f);
    CounterRng rng(5, 6, 7);
    crossover(a, b, c, rng);
    for (std::size_t i = 0; i < kGenomeLen; ++i) CHECK((c[i] == a[i] || c[i] == b[i]));
}

TEST_CASE("scalar phenotype parameters trade off through the same genotype map") {
    float slow[kGenomeLen], fast[kGenomeLen];
    neutralGenome(slow);
    neutralGenome(fast);
    slow[kSpeed] = 0.1f;
    fast[kSpeed] = 0.9f;
    fast[kAggr] = 0.8f;
    CHECK(decode(fast, 58.0f).vMax > decode(slow, 58.0f).vMax);
    CHECK(decode(fast, 58.0f).aggression > decode(slow, 58.0f).aggression);
}

TEST_CASE("diet and heat-tolerance genes decode to selectable phenotype traits") {
    float genome[kGenomeLen];
    neutralGenome(genome);
    genome[kDietPreference] = 0.9f;
    genome[kHeatTolerance] = 0.1f;
    const auto phenotype = decode(genome, 58.0f);
    CHECK(phenotype.dietPreference == doctest::Approx(0.9f));
    CHECK(phenotype.heatTolerance == doctest::Approx(0.1f));
}

TEST_CASE("immune repair and digestion genes decode to bounded selectable traits") {
    float genome[kGenomeLen];
    neutralGenome(genome);
    genome[kImmunity] = 0.9f;
    genome[kRepair] = 0.8f;
    genome[kDigestion] = 0.2f;
    const auto phenotype = decode(genome, 58.0f);
    CHECK(phenotype.immunity == doctest::Approx(0.9f));
    CHECK(phenotype.repair == doctest::Approx(0.8f));
    CHECK(phenotype.digestion == doctest::Approx(0.2f));
}

TEST_CASE("a circuit can make resource uptake depend on sensed hunger") {
    const Circuit circuit{{UnitOp::Identity, 8, 0, 2, 1.0f, 0.0f}};
    CircuitInputs hungry{}, full{};
    hungry[8] = 0.8f;
    full[8] = -0.8f;
    CHECK(evaluateCircuit(circuit, hungry).outputs[2] > evaluateCircuit(circuit, full).outputs[2]);
}

TEST_CASE("the hunt-strategy port is an evolvable circuit output") {
    const Circuit circuit{{UnitOp::Identity, 0, 0,
                           static_cast<std::uint8_t>(kHuntStrategyOutput), 1.0f, 0.0f}};
    CircuitInputs inputs{};
    inputs[0] = 0.75f;
    CHECK(evaluateCircuit(circuit, inputs).outputs[kHuntStrategyOutput] ==
          doctest::Approx(0.75f));
}

TEST_CASE("circuits can evolve comparison logic from environmental inputs") {
    const Circuit circuit{
        {UnitOp::GreaterThan, 0, 1, 0, 1.0f, 0.0f},
        {UnitOp::LessThan, 0, 1, 1, 1.0f, 0.0f}};
    CircuitInputs inputs{};
    inputs[0] = 0.75f;
    inputs[1] = -0.25f;
    const auto expression = evaluateCircuit(circuit, inputs);
    CHECK(expression.outputs[0] == doctest::Approx(1.0f));
    CHECK(expression.outputs[1] == doctest::Approx(-1.0f));
}

TEST_CASE("in-place circuit evaluation matches the allocation-based reference") {
    const Circuit circuit{
        {UnitOp::Memory, 0, 1, kNoCircuitOutput, 0.8f, 0.1f, 0.2f},
        {UnitOp::GreaterThan, static_cast<std::uint16_t>(kCircuitInputs), 2, 7, 1.0f, 0.0f}};
    CircuitInputs inputs{};
    inputs[0] = 0.5f;
    inputs[1] = -0.25f;
    inputs[2] = 0.1f;
    const std::vector<float> previous{0.3f, -0.2f};
    const std::vector<float> weights{0.7f, 1.2f};
    const auto expected = evaluateCircuit(circuit, inputs, previous, weights);
    std::vector<float> state = previous;
    std::vector<float> trace{0.4f, -0.1f};
    const auto outputs = evaluateCircuitInPlace(circuit, inputs, state, trace, weights, 0.85f);
    REQUIRE(outputs.size() == expected.outputs.size());
    for (std::size_t i = 0; i < outputs.size(); ++i)
        CHECK(outputs[i] == doctest::Approx(expected.outputs[i]));
    for (std::size_t i = 0; i < state.size(); ++i) {
        CHECK(state[i] == doctest::Approx(expected.units[i]));
        CHECK(trace[i] == doctest::Approx(0.85f * (i == 0 ? 0.4f : -0.1f) +
                                         0.15f * expected.eligibility[i]));
    }
}

TEST_CASE("output-connected complexity excludes unused units but includes their dependencies") {
    const Circuit circuit{
        {UnitOp::Identity, 0, 0, kNoCircuitOutput, 1.0f, 0.0f},
        {UnitOp::Add, static_cast<std::uint16_t>(kCircuitInputs), 1, 0, 1.0f, 0.0f},
        {UnitOp::Identity, 2, 0, kNoCircuitOutput, 1.0f, 0.0f}};
    CHECK(countOutputConnectedUnits(circuit) == 2);
}

TEST_CASE("structural mutations do not systematically favor circuit growth") {
    constexpr int trials = 10000;
    std::size_t insertions = 0, deletions = 0;
    Circuit circuit(10, UnitGene{UnitOp::Identity, 0, 0, kNoCircuitOutput, 1.0f, 0.0f});
    for (int i = 0; i < trials; ++i) {
        Circuit child = circuit;
        CounterRng rng(17, static_cast<std::uint64_t>(i), 29);
        mutateCircuit(child, 0.0f, rng);
        if (child.size() > circuit.size()) ++insertions;
        if (child.size() < circuit.size()) ++deletions;
    }
    REQUIRE(insertions + deletions > 0);
    CHECK(std::abs(static_cast<double>(insertions) - static_cast<double>(deletions)) /
              static_cast<double>(insertions + deletions) <
          0.08);
}

TEST_CASE("the ancestor starts with a minimal sensor-to-interface program") {
    const Circuit circuit = makeSeedCircuit();
    REQUIRE(circuit.size() == 4);
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        CHECK(circuit[i].output == i);
        CHECK(circuit[i].sourceA < kCircuitInputs);
    }
}

TEST_CASE("compositional circuit units combine sensor inputs into heritable outputs") {
    Circuit circuit{
        {UnitOp::Identity, 0, 0, 0, 1.0f, 0.0f},
        {UnitOp::Add, static_cast<std::uint16_t>(kCircuitInputs), 1, 1, 1.0f, 0.0f}};
    CircuitInputs inputs{};
    inputs[0] = 0.75f;
    inputs[1] = -0.25f;
    const auto e = evaluateCircuit(circuit, inputs);
    REQUIRE(e.units.size() == 2);
    CHECK(e.outputs[0] == doctest::Approx(0.75f));
    CHECK(e.outputs[1] == doctest::Approx(0.25f));
}

TEST_CASE("memory units retain a decaying internal signal") {
    const Circuit circuit{{UnitOp::Memory, 0, 1, 0, 1.0f, 0.0f}};
    CircuitInputs inputs{};
    inputs[0] = 1.0f;
    const auto first = evaluateCircuit(circuit, inputs);
    const auto second = evaluateCircuit(circuit, inputs, first.units);
    const float input = std::tanh(1.0f);
    CHECK(first.units[0] == doctest::Approx(0.2f * input));
    CHECK(second.units[0] == doctest::Approx(0.8f * first.units[0] + 0.2f * input));
}

TEST_CASE("reward-modulated plasticity adapts active subunits and stays bounded") {
    Circuit circuit{{UnitOp::Identity, 0, 0, 0, 1.0f, 0.0f, 0.2f}};
    std::vector<float> weights{1.0f}, eligibility{0.5f};
    applyRewardModulatedPlasticity(circuit, eligibility, 0.5f, weights);
    CHECK(weights[0] == doctest::Approx(1.05f));
    applyRewardModulatedPlasticity(circuit, eligibility, -0.5f, weights);
    CHECK(weights[0] == doctest::Approx(1.0f));
}

TEST_CASE("circuit mutation changes structure without invalidating connections") {
    CounterRng rng(83, 14, 29);
    Circuit circuit = makeSeedCircuit();
    const auto initialSize = circuit.size();
    bool changedSize = false;
    for (int i = 0; i < 500; ++i) {
        mutateCircuit(circuit, 0.08f, rng);
        changedSize = changedSize || circuit.size() != initialSize;
    }
    CHECK(circuit.size() <= kMaxCircuitUnits);
    CHECK(changedSize);
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        CHECK(circuit[i].sourceA < kCircuitInputs + i);
        CHECK(circuit[i].sourceB < kCircuitInputs + i);
        CHECK(circuit[i].weight >= -2.0f);
        CHECK(circuit[i].weight <= 2.0f);
    }
}

TEST_CASE("species classification: stay, branch, or join") {
    SpeciesTable t;
    float a[kGenomeLen], b[kGenomeLen];
    neutralGenome(a);
    neutralGenome(b);
    Circuit parentCircuit(6, UnitGene{UnitOp::Identity, 0, 0, kNoCircuitOutput, 1.0f, 0.0f});
    Circuit daughterCircuit = parentCircuit;
    for (auto& unit : daughterCircuit) unit.op = UnitOp::Multiply;
    const auto sa = t.addFounder(a, "Vita", "communis", 30, 0, 0, parentCircuit);
    t.delta = 0.03f;
    CounterRng rng(9, 9, 9);

    CHECK(t.classify(a, parentCircuit, sa, 10, rng) == sa);

    const auto sb = t.classify(b, daughterCircuit, sa, 20, rng);
    CHECK(sb != sa);
    CHECK(t.at(sb).parent == sa);
    CHECK(t.at(sb).genusName == "Vita");

    CHECK(t.classify(b, daughterCircuit, sa, 30, rng) == sb);
}
