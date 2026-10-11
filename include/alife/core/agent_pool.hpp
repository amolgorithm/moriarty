#pragma once
#include <cstdint>

#include "alife/core/aligned_alloc.hpp"
#include "alife/core/config.hpp"

namespace alife {

// Hot columns, touched every micro tick. Double-buffered: read one, write the other.
struct Kinematics {
    avec<float> px, py, vx, vy;
};

// Structure-of-arrays agent storage. Every column has exactly count() entries.
// Removal is swap-remove, so indices are NOT stable. Anything that outlives a
// barrier (evo queue, lineage) must hold `id`, never an index.
struct AgentPool {
    Kinematics kin[2];

    // warm: meso tick
    avec<float> energy, heat, age;
    avec<float> eta, senseR, vMax, aggression, repThresh;  // decoded phenotype
    avec<std::uint32_t> cell;

    // cold: evo and telemetry only
    avec<float> genome;  // count * kGenomeLen, row-major
    avec<std::uint64_t> id, parent, birthTick;
    avec<std::uint32_t> species;

    std::uint64_t nextId = 1;

    std::uint32_t count() const noexcept { return static_cast<std::uint32_t>(energy.size()); }

    Kinematics& read() noexcept { return kin[r_]; }
    Kinematics& write() noexcept { return kin[r_ ^ 1]; }
    const Kinematics& read() const noexcept { return kin[r_]; }
    void swapKin() noexcept { r_ ^= 1; }

    void reserve(std::size_t n);
    std::uint32_t spawn(float x, float y);  // returns index of the new agent
    void swapRemove(std::uint32_t i);

   private:
    int r_ = 0;
};

}  // namespace alife
