#include <doctest/doctest.h>

#include <algorithm>
#include <vector>

#include "alife/core/config.hpp"
#include "alife/core/rng.hpp"
#include "alife/genetics/archetypes.hpp"
#include "alife/genetics/gpm.hpp"
#include "alife/genetics/operators.hpp"
#include "alife/genetics/speciation.hpp"

using namespace alife;
using namespace alife::genetics;

TEST_CASE("mutation keeps genes in range and actually changes the genome") {
    float g[kGenomeLen], orig[kGenomeLen];
    archetypeGenome(0, g);
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
    archetypeGenome(0, a);
    archetypeGenome(3, b);
    CounterRng rng(5, 6, 7);
    crossover(a, b, c, rng);
    for (std::size_t i = 0; i < kGenomeLen; ++i) CHECK((c[i] == a[i] || c[i] == b[i]));
}

TEST_CASE("founders decode into the niches they were designed for") {
    float graz[kGenomeLen], hunt[kGenomeLen];
    archetypeGenome(0, graz);
    archetypeGenome(3, hunt);
    const Phenotype pg = decode(graz, 58.0f), ph = decode(hunt, 58.0f);
    CHECK(pg.aggression < 0.1f);
    CHECK(ph.aggression > 0.7f);
    CHECK(ph.vMax > pg.vMax);
    // hunters chase prey, grazers flee threats
    CHECK(controller(hunt, 0.5f).w[kPrey] > 0.5f);
    CHECK(controller(graz, 0.5f).w[kThreat] < -0.3f);
}

TEST_CASE("hunger raises the weight of a hunger-sensitive cue") {
    float g[kGenomeLen];
    archetypeGenome(0, g);  // food slope is positive
    CHECK(controller(g, 0.1f).w[kFood] > controller(g, 0.9f).w[kFood]);
}

TEST_CASE("species classification: stay, branch, or join") {
    SpeciesTable t;
    float a[kGenomeLen], b[kGenomeLen];
    archetypeGenome(0, a);
    archetypeGenome(3, b);
    const auto sa = t.addFounder(a, "Aurelia", "communis", 28, 0, 0);
    CounterRng rng(9, 9, 9);

    CHECK(t.classify(a, sa, 10, rng) == sa);            // identical genome stays in the parent species

    const auto sb = t.classify(b, sa, 20, rng);          // very different genome founds a new species
    CHECK(sb != sa);
    CHECK(t.at(sb).parent == sa);
    CHECK(t.at(sb).genusName == "Aurelia");              // same clade, new epithet

    CHECK(t.classify(b, sa, 30, rng) == sb);             // later arrivals join it
}
