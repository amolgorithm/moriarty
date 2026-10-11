#include <doctest/doctest.h>

#include "alife/core/rng.hpp"

using alife::CounterRng;

TEST_CASE("same key gives the same stream") {
    CounterRng a(1, 2, 3), b(1, 2, 3);
    for (int i = 0; i < 100; ++i) CHECK(a.next() == b.next());
}

TEST_CASE("different slots diverge") {
    CounterRng a(1, 2, 3), b(1, 2, 4);
    CHECK(a.next() != b.next());
}

TEST_CASE("uniform stays in [0, 1) with a sane mean") {
    CounterRng r(7, 0, 0);
    double sum = 0;
    for (int i = 0; i < 20000; ++i) {
        const float u = r.uniform();
        REQUIRE(u >= 0.0f);
        REQUIRE(u < 1.0f);
        sum += u;
    }
    CHECK(sum / 20000.0 == doctest::Approx(0.5).epsilon(0.02));
}

TEST_CASE("normal has roughly zero mean and unit variance") {
    CounterRng r(9, 0, 0);
    double s = 0, s2 = 0;
    const int n = 40000;
    for (int i = 0; i < n; ++i) {
        const double z = r.normal();
        s += z;
        s2 += z * z;
    }
    CHECK(s / n == doctest::Approx(0.0).epsilon(0.05));
    CHECK(s2 / n == doctest::Approx(1.0).epsilon(0.05));
}
