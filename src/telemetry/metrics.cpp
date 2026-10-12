#include "alife/telemetry/metrics.hpp"

#include <algorithm>

namespace alife::telemetry {

void TelemetryHistory::push(const Telemetry& t) {
    latest_ = t;
    population.push(static_cast<float>(t.population));
    entropy.push(t.shannonBits);
    species.push(static_cast<float>(t.speciesCount));
    energy.push(t.meanEnergy);
    complexity.push(t.meanOutputConnectedInstructions);
    executedInstructions.push(t.meanExecutedInstructions);

    // drop markers that scrolled out of the window
    const std::uint64_t pushed = population.totalPushed();
    const std::uint64_t cap = population.capacity();
    markers.erase(std::remove_if(markers.begin(), markers.end(),
                                 [&](const PhaseMarker& m) { return m.sample + cap < pushed; }),
                  markers.end());
}

void TelemetryHistory::mark(std::string label) {
    markers.push_back({population.totalPushed(), std::move(label)});
}

}  // namespace alife::telemetry
