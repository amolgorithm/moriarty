#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "alife/telemetry/ring_buffer.hpp"

namespace alife::telemetry {

struct Telemetry {
    std::uint64_t tick = 0;
    std::uint32_t population = 0;
    std::uint32_t speciesCount = 0;
    float shannonBits = 0.0f;
    float meanEnergy = 0.0f;
    float totalHeat = 0.0f;
};

// Labelled event on the time axis (speciation burst, mass extinction, ...).
struct PhaseMarker {
    std::uint64_t sample = 0;  // index in the history, see RingBuffer::totalPushed
    std::string label;
};

class TelemetryHistory {
   public:
    explicit TelemetryHistory(std::size_t capacity = 600)
        : population(capacity), entropy(capacity), species(capacity), energy(capacity) {}

    void push(const Telemetry& t);
    void mark(std::string label);  // marker at the next sample

    const Telemetry& latest() const noexcept { return latest_; }

    RingBuffer<float> population, entropy, species, energy;
    std::vector<PhaseMarker> markers;

   private:
    Telemetry latest_;
};

}  // namespace alife::telemetry
