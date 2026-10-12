#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "alife/genetics/circuit.hpp"
#include "alife/render/palette.hpp"

namespace alife::render {

// One organism as the renderer sees it. Genes are already decoded.
struct AgentVis {
    float x = 0, y = 0, vx = 0, vy = 0, heading = 0;
    float r = 5;          // body radius, world units
    float energy = 0.5f;  // fraction of max
    float aggr = 0, def = 0, speed = 0.3f, sense = 0.3f, eta = 0.5f, repThr = 0.7f;
    float dietPreference = 0.5f, heatTolerance = 0.5f;  // all 0..1
    float age01 = 0;
    std::uint32_t species = 0;
    std::uint16_t circuitUnits = 0;
    std::array<std::uint16_t, 9> unitOps{};
    float circuitActivity = 0;
    std::uint64_t id = 0;
};

enum class RoleVis : std::uint8_t { Grazer, Omnivore, Predator };

struct SpeciesVis {
    std::uint32_t id = 0, parent = 0xFFFFFFFFu, count = 0;
    std::string fullName, shortName;
    Rgb color{200, 200, 200};
    RoleVis role = RoleVis::Grazer;
    float labelX = 0, labelY = 0;
    float meanAggr = 0, meanDef = 0, meanSize = 0, meanSpeed = 0, meanUnits = 0;
    bool established = false;
};

struct EdgeVis {
    std::uint32_t pred = 0, prey = 0;
    float rate = 0;  // decayed kills
};

struct EffectVis {
    enum Type : std::uint8_t { Kill, Birth, Death } type = Kill;
    float x = 0, y = 0, x2 = 0, y2 = 0;
    std::uint32_t sp = 0, sp2 = 0;
};

struct SelectedVis {
    bool valid = false;
    AgentVis a;
    float energyAbs = 0, eMax = 0, ageSec = 0;
    float plantIntake = 0, animalIntake = 0;
    std::uint32_t generation = 0;
    std::uint64_t parentId = 0;
    float outputs[6] = {0, 0, 0, 0, 0, 0};  // movement, uptake, resource, defense, and hunt strategy
    bool hasPrey = false, hasThreat = false;
    float preyX = 0, preyY = 0, threatX = 0, threatY = 0;
    std::string state;
    struct Unit {
        std::uint8_t op = 0, output = 0xFF;
        std::uint16_t sourceA = 0, sourceB = 0;
        float weight = 0, inheritedWeight = 0, bias = 0, plasticity = 0, value = 0;
    };
    std::vector<Unit> units;
};

// Everything the renderer needs for one frame. Built from the World by
// buildSnapshot(), the sim never touches raylib and the renderer never touches the sim.
struct RenderSnapshot {
    std::uint64_t tick = 0;
    double simTime = 0;
    float worldW = 0, worldH = 0;
    float season = 0;

    std::vector<AgentVis> agents;
    std::vector<SpeciesVis> species;  // sorted by count, descending, alive only
    std::vector<EdgeVis> edges;       // predator -> prey, between living species
    std::vector<EffectVis> events;
    SelectedVis selected;
    std::uint64_t selectedRequested = 0;  // id the builder was asked for

    int fieldW = 0, fieldH = 0;
    std::vector<float> food0, food1, heat;  // normalized 0..1
    std::vector<float> tracerX, tracerY;

    std::uint64_t births = 0, kills = 0;
    double plantEnergy = 0.0, animalEnergy = 0.0;
    double energyBalanceError = 0.0;

    const SpeciesVis* findSpecies(std::uint32_t id) const {
        for (const auto& s : species)
            if (s.id == id) return &s;
        return nullptr;
    }
};

}  // namespace alife::render
