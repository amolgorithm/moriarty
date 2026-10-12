#pragma once
#include <cstdint>

#include "alife/core/aligned_alloc.hpp"
#include "alife/core/config.hpp"
#include "alife/genetics/circuit.hpp"

namespace alife {

enum class AgentAction : std::uint8_t { Explore, Forage, Hunt, Evade, Rest };

// Hot columns, touched every micro tick. Double-buffered: read one, write the other.
struct Kinematics {
    avec<float> px, py, vx, vy;
};

// Structure-of-arrays agent storage. Every column has exactly count() entries.
// Removal is swap-remove, so indices are NOT stable. Anything that outlives a
// barrier (evo queue, lineage, UI selection) must hold `id`, never an index.
struct AgentPool {
    Kinematics kin[2];

    // warm: meso tick
    avec<float> energy, bodyEnergy, digest, age;  // digest: seconds until the next hunt is possible
    avec<float> eta, senseR, vMax, aggression, repThresh, dietPreference, heatTolerance;  // decoded phenotype
    avec<float> radius, mass, defense, eMax, explore;
    avec<float> wx, wy;      // OU wander state (colored noise)
    avec<float> ax, ay;      // steering command from the controller
    avec<float> harvestGain, resourceBias, protection, huntBias;
    avec<float> plantIntake, animalIntake;
    avec<std::uint8_t> action;
    avec<float> heading;     // smoothed facing, for rendering
    avec<std::uint32_t> cell, species, generation;

    // cold: evo and telemetry only
    avec<float> genome;  // count * kGenomeLen, row-major
    std::vector<genetics::Circuit> circuit;
    std::vector<std::vector<float>> circuitState;
    std::vector<std::vector<float>> circuitWeights, circuitEligibility;
    avec<std::uint64_t> id, parent, birthTick, preyId, threatId;

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
    template <class F>
    void forEachColumn(F&& f);
    int r_ = 0;
};

}  // namespace alife
