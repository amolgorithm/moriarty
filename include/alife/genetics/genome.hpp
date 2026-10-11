#pragma once
#include <cstddef>

namespace alife::genetics {

// Gene layout inside a genome row. Controller weights fill the rest.
enum Gene : std::size_t {
    kMetabolicEff = 0,
    kSenseRadius,
    kMaxSpeed,
    kAggression,
    kRepThreshold,
    kMutRate,  // self-adaptive mutation rate
    kControllerBegin
};

}  // namespace alife::genetics
