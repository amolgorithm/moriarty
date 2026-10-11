#include <doctest/doctest.h>

#include <vector>

#include "alife/math/entropy.hpp"

using alife::math::shannonBits;

TEST_CASE("shannon entropy") {
    CHECK(shannonBits(std::vector<std::uint32_t>{10}) == doctest::Approx(0.0));
    CHECK(shannonBits(std::vector<std::uint32_t>{5, 5}) == doctest::Approx(1.0));
    CHECK(shannonBits(std::vector<std::uint32_t>{3, 3, 3, 3}) == doctest::Approx(2.0));
    CHECK(shannonBits(std::vector<std::uint32_t>{}) == doctest::Approx(0.0));
    CHECK(shannonBits(std::vector<std::uint32_t>{0, 7, 0}) == doctest::Approx(0.0));
}
