#include <doctest/doctest.h>

#include "alife/concurrency/triple_buffer.hpp"
#include "alife/telemetry/metrics.hpp"
#include "alife/telemetry/ring_buffer.hpp"

using namespace alife;

TEST_CASE("ring buffer keeps the newest samples in order") {
    telemetry::RingBuffer<int> r(4);
    for (int i = 1; i <= 6; ++i) r.push(i);
    CHECK(r.size() == 4);
    CHECK(r.at(0) == 3);
    CHECK(r.back() == 6);
    CHECK(r.totalPushed() == 6);
}

TEST_CASE("markers scroll out with the window") {
    telemetry::TelemetryHistory h(4);
    h.mark("first");
    for (int i = 0; i < 10; ++i) h.push({});
    CHECK(h.markers.empty());
}

TEST_CASE("triple buffer hands over the latest frame") {
    TripleBuffer<int> tb;
    tb.back() = 1;
    tb.publish();
    tb.back() = 2;
    tb.publish();
    CHECK(tb.front() == 2);
    CHECK(tb.front() == 2);  // nothing new, same frame
}
