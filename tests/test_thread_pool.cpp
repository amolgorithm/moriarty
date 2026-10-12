#include <doctest/doctest.h>

#include <atomic>
#include <stdexcept>
#include <vector>

#include "alife/concurrency/thread_pool.hpp"

using namespace alife;

TEST_CASE("thread pool covers every parallel range exactly once") {
    ThreadPool pool(4);
    std::vector<std::uint32_t> visits(10003, 0);
    pool.parallelFor(visits.size(), 37, [&](std::size_t begin, std::size_t end) {
        for (std::size_t i = begin; i < end; ++i) ++visits[i];
    });
    for (const auto visit : visits) CHECK(visit == 1);
}

TEST_CASE("thread pool propagates worker exceptions and remains usable") {
    ThreadPool pool(3);
    CHECK_THROWS_AS(pool.parallelFor(1000, 10, [](std::size_t begin, std::size_t) {
                        if (begin == 100) throw std::logic_error("worker failure");
                    }),
                    std::logic_error);
    std::atomic<std::size_t> completed = 0;
    pool.parallelFor(1000, 10, [&](std::size_t begin, std::size_t end) {
        completed.fetch_add(end - begin, std::memory_order_relaxed);
    });
    CHECK(completed.load(std::memory_order_relaxed) == 1000);
}

TEST_CASE("thread pool rejects an empty chunk size") {
    ThreadPool pool(2);
    CHECK_THROWS_AS(pool.parallelFor(1, 0, [](std::size_t, std::size_t) {}), std::invalid_argument);
}
