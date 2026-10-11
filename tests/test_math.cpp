#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "alife/core/rng.hpp"
#include "alife/math/entropy.hpp"
#include "alife/math/meanfield.hpp"
#include "alife/math/replicator.hpp"
#include "alife/math/te.hpp"
#include "alife/telemetry/oee.hpp"

using namespace alife;

TEST_CASE("renyi, hill and simpson agree on a uniform community") {
    const std::vector<std::uint32_t> u{5, 5, 5, 5};
    CHECK(math::renyiBits(u, 2.0) == doctest::Approx(2.0));
    CHECK(math::hillNumber(u, 0.0) == doctest::Approx(4.0));
    CHECK(math::hillNumber(u, 1.0) == doctest::Approx(4.0));
    CHECK(math::simpsonDiversity(u) == doctest::Approx(0.75));
    const std::vector<std::uint32_t> skew{97, 1, 1, 1};
    CHECK(math::hillNumber(skew, 2.0) < math::hillNumber(skew, 0.0));
}

TEST_CASE("divergences") {
    const std::vector<double> p{0.5, 0.5}, q{0.9, 0.1};
    CHECK(math::klBits(p, p) == doctest::Approx(0.0));
    CHECK(math::klBits(p, q) > 0.0);
    CHECK(math::jensenShannonBits(p, q) == doctest::Approx(math::jensenShannonBits(q, p)));
    const std::vector<double> a{1, 0}, b{0, 1};
    CHECK(math::jensenShannonBits(a, b) == doctest::Approx(1.0));
}

TEST_CASE("mutual information") {
    // perfectly correlated binary variables carry 1 bit, independent ones carry 0
    CHECK(math::mutualInformationBits(std::vector<std::uint32_t>{10, 0, 0, 10}, 2, 2) == doctest::Approx(1.0));
    CHECK(math::mutualInformationBits(std::vector<std::uint32_t>{5, 5, 5, 5}, 2, 2) == doctest::Approx(0.0));
}

TEST_CASE("replicator dynamics selects the fitter type and conserves the simplex") {
    std::vector<double> x{0.5, 0.5};
    const std::vector<double> f{1.0, 0.5};
    for (int i = 0; i < 400; ++i) math::replicatorStep(x, f, 1.0, 0.05);
    CHECK(x[0] > 0.99);
    CHECK(x[0] + x[1] == doctest::Approx(1.0));
}

TEST_CASE("hawk-dove replicator converges to the mixed ESS") {
    // V=2, C=4: ESS fraction of hawks = V/C = 0.5
    const std::vector<double> A{-1.0, 2.0, 0.0, 1.0};
    std::vector<double> x{0.9, 0.1}, f(2);
    for (int i = 0; i < 4000; ++i) {
        math::gamePayoff(A, x, f);
        math::replicatorStep(x, f, 1.0, 0.01);
    }
    CHECK(x[0] == doctest::Approx(0.5).epsilon(0.01));
}

TEST_CASE("price equation: pure selection, no transmission bias") {
    const std::vector<double> q{0.5, 0.5}, w{2.0, 1.0}, z{1.0, 0.0};
    const auto t = math::priceEquation(q, w, z, z);
    CHECK(t.transmission == doctest::Approx(0.0));
    // zbar' = (0.5*2*1)/(1.5) = 2/3, zbar = 0.5, so delta = 1/6
    CHECK(t.selection == doctest::Approx(1.0 / 6.0));
}

TEST_CASE("fitness recovered from a replicator trajectory") {
    const std::vector<double> f{0.3, 0.1, 0.0};
    std::vector<double> x{0.4, 0.3, 0.3}, series;
    const double dt = 1.0;
    for (int k = 0; k < 30; ++k) {
        series.insert(series.end(), x.begin(), x.end());
        for (int j = 0; j < 100; ++j) math::replicatorStep(x, f, 1.0, dt / 100);
    }
    const auto fit = math::fitRelativeFitness(series, 30, 3, dt, 2);
    CHECK(fit[0] == doctest::Approx(0.3).epsilon(0.02));
    CHECK(fit[1] == doctest::Approx(0.1).epsilon(0.02));
    CHECK(math::replicatorPredictionGap(series, 30, 3, dt, fit) < 0.02);
}

TEST_CASE("transfer entropy is directional") {
    CounterRng rng(1, 0, 0);
    const std::size_t n = 20000;
    std::vector<std::uint8_t> y(n), x(n);
    for (auto& v : y) v = static_cast<std::uint8_t>(rng.next() & 1);
    x[0] = 0;
    for (std::size_t t = 0; t + 1 < n; ++t) x[t + 1] = y[t];  // x copies y with lag 1
    const double fwd = math::transferEntropy(y, x, 2), back = math::transferEntropy(x, y, 2);
    CHECK(fwd > 0.95);
    CHECK(back < 0.02);
    CHECK(math::surrogateCorrectedTE(y, x, 2, 8, 7) > 0.9);
}

TEST_CASE("active information storage: periodic is predictable, noise is not") {
    std::vector<std::uint8_t> per(4000), noise(4000);
    CounterRng rng(3, 0, 0);
    for (std::size_t i = 0; i < per.size(); ++i) { per[i] = i % 2; noise[i] = rng.next() & 1; }
    CHECK(math::activeInformationStorage(per, 2) > 0.95);
    CHECK(math::activeInformationStorage(noise, 2) < 0.02);
}

TEST_CASE("generalized Lotka-Volterra reaches its analytic equilibrium") {
    math::GeneralizedLV g;
    g.n = 2;
    g.r = {1.0, 0.6};
    g.A = {-1.0, -0.3, -0.2, -1.0};
    bool ok = false;
    const auto eq = g.equilibrium(ok);
    REQUIRE(ok);
    std::vector<double> x{0.1, 0.1};
    for (int i = 0; i < 20000; ++i) g.stepRK4(x, 0.01);
    CHECK(x[0] == doctest::Approx(eq[0]).epsilon(1e-3));
    CHECK(x[1] == doctest::Approx(eq[1]).epsilon(1e-3));
}

TEST_CASE("activity: persistence accumulates and a gap resets it") {
    telemetry::OeeTracker t;
    const telemetry::Component a{1, 1, 0}, b{2, 2, 1};
    for (int i = 0; i < 10; ++i) t.update(std::vector{a});
    CHECK(t.history().back().cumulative == doctest::Approx(10.0));
    t.update(std::vector{b});  // a vanishes
    const auto& s = t.update(std::vector{a, b});
    CHECK(s.cumulative == doctest::Approx(1.0 + 2.0));  // a restarted at 1, b is at 2
    CHECK(s.novelty == doctest::Approx(0.0));
    CHECK(s.ecoPotential == doctest::Approx(2.0));
    CHECK(s.complexity == doctest::Approx(1.5));
}

TEST_CASE("neutral shadow threshold ignores short-lived churn") {
    telemetry::OeeTracker t(5.0);
    for (int i = 0; i < 20; ++i) {
        const telemetry::Component churn{static_cast<std::uint64_t>(100 + i), 1, 0};
        t.update(std::vector{churn});  // new id every sample: never adaptive
    }
    CHECK(t.history().back().cumulative == doctest::Approx(0.0));
    for (int i = 0; i < 20; ++i) t.update(std::vector{telemetry::Component{1, 1, 0}});
    CHECK(t.history().back().diversity == 1);
    CHECK(t.stillGrowing(10));
}
