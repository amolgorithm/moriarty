#pragma once
#include <cstdint>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace alife::telemetry {

// A "component" is anything whose persistence can be counted: a species, a genotype class,
// later a gene-program module. Callers hand the tracker the set present at each sample.
struct Component {
    std::uint64_t id = 0;
    float depth = 1.0f;       // structural complexity (units composed, nesting depth)
    std::uint32_t niche = 0;  // ecological role label (trophic level, resource channel, ...)
};

// Bedau-Packard evolutionary activity statistics plus MODES (Dolson et al. 2019).
//
// Activity of a component is the number of samples it has persisted without interruption.
// A system shows unbounded open-ended evolution when cumulative activity of ADAPTIVE
// components keeps growing instead of plateauing. "Adaptive" means activity above a
// neutral-shadow threshold: persistence that a model without selection would not produce.
// Pass the shadow threshold from a neutral run, or leave it 0 to count raw persistence.
struct ActivitySample {
    std::uint64_t sample = 0;
    double cumulative = 0.0;  // A_cum: summed activity of adaptive components present
    double newActivity = 0.0; // A_new: components crossing the threshold in this sample
    double meanActivity = 0.0;// A_cum / D
    std::uint32_t diversity = 0;  // D: adaptive components present
    // MODES
    double change = 0.0;      // 1 - Jaccard(prev set, cur set): turnover of the component set
    double novelty = 0.0;     // components never seen before, per sample
    double complexity = 0.0;  // mean structural depth of the present set
    double ecoPotential = 0.0;// number of distinct niches occupied
};

class OeeTracker {
   public:
    explicit OeeTracker(double shadowThreshold = 0.0) : threshold_(shadowThreshold) {}

    void setThreshold(double t) noexcept { threshold_ = t; }
    const ActivitySample& update(std::span<const Component> present);

    const std::vector<ActivitySample>& history() const noexcept { return history_; }

    // True when cumulative activity over the last `window` samples still trends upward
    // (least-squares slope > 0 and the window max is within `tol` of the series max).
    bool stillGrowing(std::size_t window = 40, double tol = 0.05) const;

   private:
    double threshold_;
    std::uint64_t n_ = 0;
    std::unordered_map<std::uint64_t, std::uint64_t> activity_;  // present components only
    std::unordered_set<std::uint64_t> everSeen_, prev_;
    std::vector<ActivitySample> history_;
};

}  // namespace alife::telemetry
