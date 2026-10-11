#pragma once
#include <cstdint>
#include <vector>

namespace alife::genetics {

struct Species {
    std::uint32_t id = 0;
    std::vector<float> exemplar;  // representative genome for compatibility distance
    std::uint32_t size = 0;
};

// Incremental clustering in genotype space, NEAT-style compatibility distance.
class SpeciesTable {
   public:
    std::uint32_t assign(const float* genomeRow);  // returns species id
    const std::vector<Species>& species() const noexcept { return species_; }

   private:
    std::vector<Species> species_;
    float threshold_ = 3.0f;
};

}  // namespace alife::genetics
