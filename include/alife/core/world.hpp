#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "alife/core/agent_pool.hpp"
#include "alife/core/config.hpp"
#include "alife/core/grid.hpp"
#include "alife/genetics/lineage.hpp"
#include "alife/genetics/speciation.hpp"

namespace alife {

// Scalar field on a coarse torus grid (resources, heat).
struct Field {
    int w = 0, h = 0;
    float cellW = 1, cellH = 1;
    std::vector<float> v;

    void init(int fw, int fh, float worldW, float worldH, float value = 0.0f) {
        w = fw; h = fh;
        cellW = worldW / static_cast<float>(fw);
        cellH = worldH / static_cast<float>(fh);
        v.assign(static_cast<std::size_t>(fw) * static_cast<std::size_t>(fh), value);
    }
    int ix(float x) const noexcept { return std::min(w - 1, std::max(0, static_cast<int>(x / cellW))); }
    int iy(float y) const noexcept { return std::min(h - 1, std::max(0, static_cast<int>(y / cellH))); }
    std::size_t idx(float x, float y) const noexcept {
        return static_cast<std::size_t>(iy(y)) * static_cast<std::size_t>(w) + static_cast<std::size_t>(ix(x));
    }
    float at(int x, int y) const noexcept {
        x = ((x % w) + w) % w;
        y = ((y % h) + h) % h;
        return v[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)];
    }
    float sample(float x, float y, float worldW, float worldH) const noexcept {
        return at(ix(wrap(x, worldW)), iy(wrap(y, worldH)));
    }
};

// Stochastic divergence-free current: a few Fourier modes whose amplitudes follow
// Ornstein-Uhlenbeck processes and whose phases drift.
struct FlowMode {
    float kx, ky;     // wavenumber
    float dx, dy;     // unit wave direction
    float amp;        // velocity amplitude (OU)
    float ampSigma;
    float phase, omega;
};

// Patchy disturbances: blights and blooms that fade in and out (shot noise).
struct Patch {
    float x, y, radius, factor, age, duration;
};

struct Event {
    enum Type : std::uint8_t { Kill, Birth, Death } type = Kill;
    float x = 0, y = 0, x2 = 0, y2 = 0;
    std::uint32_t sp = 0, sp2 = 0;
};

struct Counters {
    std::uint64_t births = 0, kills = 0, starved = 0, senescent = 0;
    double plantEnergy = 0.0, animalEnergy = 0.0;
    double initialEnergy = 0.0, externalEnergy = 0.0, dissipatedEnergy = 0.0;
};

struct World {
    SimConfig cfg;
    AgentPool pool;
    ToroidalGrid grid;
    std::uint64_t tick = 0;
    double time = 0.0;

    // Two food channels with different terrain. R[0]: fine suspended food, favors small
    // bodies. R[1]: coarse food, favors large bodies. Q: waste heat.
    Field R[2], K0[2], Keff[2], Q, Qtmp;
    genetics::SpeciesTable species;
    genetics::LineageLog lineage;

    std::vector<FlowMode> flow;
    std::vector<Patch> patches;
    float seasonPhase = 0.0f, season = 0.0f, weather = 0.0f;
    std::vector<float> tracerX, tracerY;  // passive particles, visualize the current

    std::vector<Event> events;                               // drained by the snapshot builder
    std::unordered_map<std::uint64_t, float> predation;      // (pred << 32 | prey) -> decayed kill count
    std::vector<std::string> pendingMarkers;                 // drained by the viewer
    Counters stats;

    struct Scratch {
        std::vector<int> preyIdx;
        std::vector<float> preyDist;
        std::vector<float> energyAtStart;
        std::vector<std::uint8_t> crowd;
        std::vector<std::uint8_t> mated;
        std::vector<std::uint8_t> dead, cause;
    } scratch;

    explicit World(const SimConfig& c = {});

    std::pair<float, float> flowAt(float x, float y) const noexcept;
    double energyInventory() const noexcept {
        double energy = 0.0;
        for (const auto& field : R)
            for (float value : field.v) energy += static_cast<double>(value) * cfg.eco.energyPerR;
        for (float value : Q.v) energy += value;
        for (std::uint32_t i = 0; i < pool.count(); ++i)
            energy += static_cast<double>(pool.energy[i]) + pool.bodyEnergy[i];
        return energy;
    }
    std::uint32_t addAgent(const float* genome, float x, float y, float energy, std::uint32_t species,
                           std::uint64_t parent, std::uint32_t generation,
                           const genetics::Program& program = {});
    void emit(const Event& e) {
        if (events.size() < 700) events.push_back(e);
    }

   private:
    void buildTerrain(int channel);
    void initFlow();
    void seedPopulation();
};

}  // namespace alife
