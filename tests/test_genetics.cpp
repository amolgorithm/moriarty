#include <doctest/doctest.h>

#include <algorithm>
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

TEST_CASE("a circuit can make resource uptake depend on sensed hunger") {
    const Circuit circuit{{UnitOp::Identity, 8, 0, 2, 1.0f, 0.0f}};
    CircuitInputs hungry{}, full{};
    hungry[8] = 0.8f;
    full[8] = -0.8f;
    CHECK(evaluateCircuit(circuit, hungry).outputs[2] > evaluateCircuit(circuit, full).outputs[2]);
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

TEST_CASE("circuit mutation changes structure without invalidating connections") {
    CounterRng rng(83, 14, 29);
    Circuit circuit = makeSeedCircuit();
    const auto initialSize = circuit.size();
    for (int i = 0; i < 500; ++i) mutateCircuit(circuit, 0.08f, rng);
    CHECK(circuit.size() <= kMaxCircuitUnits);
    CHECK(circuit.size() != initialSize);
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
