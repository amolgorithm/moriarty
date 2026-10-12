#include <doctest/doctest.h>

#include "alife/core/agent_pool.hpp"

using namespace alife;

TEST_CASE("spawn and swap-remove keep every column the same length") {
    AgentPool p;
    for (int i = 0; i < 5; ++i) p.spawn(static_cast<float>(i), 0.0f);
    CHECK(p.count() == 5);
    CHECK(p.genome.size() == 5 * kGenomeLen);
    CHECK(p.circuit.size() == 5);
    CHECK(p.circuitWeights.size() == 5);
    CHECK(p.circuitEligibility.size() == 5);

    p.genome[4 * kGenomeLen] = 99.0f;  // tag the last agent
    const auto lastId = p.id[4];

    p.swapRemove(1);
    CHECK(p.count() == 4);
    CHECK(p.read().px.size() == 4);
    CHECK(p.write().py.size() == 4);
    CHECK(p.genome.size() == 4 * kGenomeLen);
    CHECK(p.species.size() == 4);
    CHECK(p.bodyEnergy.size() == 4);
    CHECK(p.circuit.size() == 4);
    CHECK(p.circuitState.size() == 4);
    CHECK(p.circuitWeights.size() == 4);
    CHECK(p.circuitEligibility.size() == 4);
    CHECK(p.plantIntake.size() == 4);
    CHECK(p.animalIntake.size() == 4);
    CHECK(p.action.size() == 4);

    // last agent moved into slot 1, genome row and id came with it
    CHECK(p.id[1] == lastId);
    CHECK(p.genome[1 * kGenomeLen] == doctest::Approx(99.0f));
}

TEST_CASE("removing the only agent empties the pool") {
    AgentPool p;
    p.spawn(0, 0);
    p.swapRemove(0);
    CHECK(p.count() == 0);
    CHECK(p.genome.empty());
    CHECK(p.circuit.empty());
    CHECK(p.circuitState.empty());
    CHECK(p.circuitWeights.empty());
    CHECK(p.circuitEligibility.empty());
    CHECK(p.plantIntake.empty());
    CHECK(p.animalIntake.empty());
    CHECK(p.action.empty());
}

TEST_CASE("kinematics buffers swap") {
    AgentPool p;
    p.spawn(1.0f, 2.0f);
    p.write().px[0] = 5.0f;
    p.swapKin();
    CHECK(p.read().px[0] == doctest::Approx(5.0f));
}
