#pragma once
#include <cstdint>
#include <vector>

namespace alife::genetics {

enum class LineageEvent : std::uint8_t { Birth, Death, SpeciesFounded, SpeciesExtinct };

struct LineageRecord {
    std::uint64_t tick = 0;
    std::uint64_t id = 0;
    std::uint64_t parent = 0;
    std::uint32_t species = 0;
    LineageEvent event = LineageEvent::Birth;
};

// Phylogenetic record. Feeds the OEE metrics in phase 5.
class LineageLog {
   public:
    void record(const LineageRecord& r);
    const std::vector<LineageRecord>& records() const noexcept { return records_; }

   private:
    std::vector<LineageRecord> records_;
};

}  // namespace alife::genetics
