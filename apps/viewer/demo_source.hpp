#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "alife/render/snapshot.hpp"
#include "alife/telemetry/metrics.hpp"

namespace moriarty {

// Synthetic stand-in for the real sim so the frontend can be built and tuned
// before the core is wired up. Species are clusters pulled toward drifting
// attractors, with Langevin noise from alife::math::EulerMaruyama. Replace with
// a World + Scheduler feed once phase 1 lands.
class DemoSource {
   public:
    DemoSource(std::size_t agents, float worldW, float worldH, std::uint64_t seed);

    void step(float dt);
    void fill(alife::render::RenderSnapshot& out) const;
    alife::telemetry::Telemetry telemetry() const;
    std::vector<std::string> takeEvents();

   private:
    struct Center {
        float bx, by, radius, omega, phase;
    };
    void speciationEvent();
    void extinctionEvent();
    void centerPos(std::size_t s, float& x, float& y) const;

    std::size_t n_;
    float w_, h_;
    std::uint64_t seed_;
    std::uint64_t tick_ = 0;
    float time_ = 0.0f;

    std::vector<float> x_, y_, vx_, vy_, energy_;
    std::vector<std::uint32_t> species_;
    std::vector<Center> centers_;
    std::vector<bool> alive_;
    std::uint32_t nextSpecies_ = 0;
    std::vector<std::string> events_;
};

}  // namespace moriarty
