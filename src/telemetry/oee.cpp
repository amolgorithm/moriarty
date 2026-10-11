#include "alife/telemetry/oee.hpp"

#include <algorithm>

namespace alife::telemetry {

const ActivitySample& OeeTracker::update(std::span<const Component> present) {
    ActivitySample s;
    s.sample = n_++;

    std::unordered_set<std::uint64_t> cur;
    cur.reserve(present.size() * 2);
    std::unordered_set<std::uint32_t> niches;
    double depthSum = 0.0;
    std::unordered_map<std::uint64_t, std::uint64_t> next;
    next.reserve(present.size() * 2);

    for (const auto& c : present) {
        cur.insert(c.id);
        niches.insert(c.niche);
        depthSum += static_cast<double>(c.depth);
        if (everSeen_.insert(c.id).second) s.novelty += 1.0;

        // persistence counter: continues if present last sample, restarts otherwise
        const auto it = activity_.find(c.id);
        const std::uint64_t a = (it == activity_.end() ? 0 : it->second) + 1;
        next[c.id] = a;

        const double ad = static_cast<double>(a);
        if (ad > threshold_) {
            s.cumulative += ad;
            ++s.diversity;
            if (ad - 1.0 <= threshold_) s.newActivity += 1.0;  // just crossed
        }
    }
    activity_ = std::move(next);

    s.meanActivity = s.diversity ? s.cumulative / static_cast<double>(s.diversity) : 0.0;
    s.complexity = present.empty() ? 0.0 : depthSum / static_cast<double>(present.size());
    s.ecoPotential = static_cast<double>(niches.size());

    std::size_t inter = 0;
    for (auto id : cur) inter += prev_.count(id);
    const std::size_t uni = cur.size() + prev_.size() - inter;
    s.change = uni ? 1.0 - static_cast<double>(inter) / static_cast<double>(uni) : 0.0;

    prev_ = std::move(cur);
    history_.push_back(s);
    return history_.back();
}

bool OeeTracker::stillGrowing(std::size_t window, double tol) const {
    const std::size_t n = history_.size();
    if (n < window || window < 4) return false;
    double st = 0, sy = 0, stt = 0, sty = 0, maxAll = 0, maxWin = 0;
    for (const auto& h : history_) maxAll = std::max(maxAll, h.cumulative);
    for (std::size_t i = n - window; i < n; ++i) {
        const double t = static_cast<double>(i), y = history_[i].cumulative;
        st += t; sy += y; stt += t * t; sty += t * y;
        maxWin = std::max(maxWin, y);
    }
    const double m = static_cast<double>(window), den = m * stt - st * st;
    const double slope = den != 0.0 ? (m * sty - st * sy) / den : 0.0;
    return slope > 0.0 && maxWin >= (1.0 - tol) * maxAll;
}

}  // namespace alife::telemetry
