#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "alife/core/agent_pool.hpp"
#include "alife/core/rng.hpp"
#include "alife/genetics/genome.hpp"
#include "alife/genetics/circuit.hpp"
#include "alife/genetics/lineage.hpp"

namespace alife::genetics {

enum class Role : std::uint8_t { Grazer, Omnivore, Predator };

inline constexpr std::uint32_t kEstablished = 8;  // members needed to count as a species

struct Species {
    std::uint32_t id = 0;
    std::uint32_t parent = 0xFFFFFFFFu;
    std::uint32_t genus = 0;
    std::string genusName, epithet;
    float hue = 0, sat = 0.6f, val = 0.85f;  // degrees, 0..1, 0..1
    std::array<float, kDistGenes> exemplar{};
    std::uint64_t founded = 0, extinct = 0;
    bool alive = true, announced = false;
    std::uint32_t count = 0, peak = 0;
    float meanAggr = 0, meanDef = 0, meanSize = 0, meanSpeed = 0, meanUnits = 0;
    float meanDiet = 0.5f, meanHeatTolerance = 0.5f;
    float labelX = 0, labelY = 0;
    Role role = Role::Grazer;

    std::string fullName() const { return genusName + " " + epithet; }
    std::string shortName() const { return std::string(1, genusName.empty() ? '?' : genusName[0]) + ". " + epithet; }
};

// Incremental clustering in trait space. A newborn joins its parent's species when
// close to the exemplar, otherwise the nearest species within delta, otherwise it
// founds a new one. Exemplars track the running mean of the members.
class SpeciesTable {
   public:
    static constexpr std::uint32_t kNone = 0xFFFFFFFFu;

    std::uint32_t addFounder(const float* genome, const char* genus, const char* epithet, float hue,
                             std::uint32_t genusIndex, std::uint64_t tick,
                             std::span<const UnitGene> circuit = {});
    std::uint32_t classify(const float* genome, std::uint32_t parentSpecies, std::uint64_t tick,
                           CounterRng& rng);
    std::uint32_t classify(const float* genome, std::span<const UnitGene> circuit, std::uint32_t parentSpecies,
                           std::uint64_t tick, CounterRng& rng);
    void revive(std::uint32_t id, std::uint64_t tick);
    void refresh(const AgentPool& pool, float worldW, float worldH, std::uint64_t tick,
                 std::vector<std::string>& markers, LineageLog* lineage);

    const std::vector<Species>& all() const noexcept { return sp_; }
    const Species& at(std::uint32_t id) const { return sp_[id]; }

    static void project(const float* genome, float* out);
    static void project(const float* genome, std::span<const UnitGene> circuit, float* out);
    static float distance(const float* projected, const std::array<float, kDistGenes>& ex);

    float delta = 0.08f;

   private:
    std::vector<Species> sp_;
};

}  // namespace alife::genetics
