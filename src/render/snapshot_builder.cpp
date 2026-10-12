#include "alife/render/snapshot_builder.hpp"

#include <algorithm>
#include <cmath>

namespace alife::render {
namespace {

AgentVis makeVis(const World& w, std::uint32_t i) {
    const auto& p = w.pool;
    const auto& k = p.read();
    const float vTop = w.cfg.eco.vTop;
    AgentVis a;
    a.x = k.px[i]; a.y = k.py[i]; a.vx = k.vx[i]; a.vy = k.vy[i];
    a.heading = p.heading[i];
    a.r = p.radius[i];
    a.energy = std::clamp(p.energy[i] / p.eMax[i], 0.0f, 1.0f);
    a.aggr = p.aggression[i];
    a.def = p.defense[i];
    a.speed = (p.vMax[i] - 8.0f) / (vTop - 8.0f);
    a.sense = (p.senseR[i] - 14.0f) / 34.0f;
    a.eta = 0.35f + 0.65f * p.eta[i];
    a.repThr = (p.repThresh[i] - 0.55f) / 0.4f;
    a.dietPreference = p.dietPreference[i];
    a.heatTolerance = p.heatTolerance[i];
    a.immunity = p.immunity[i];
    a.repair = p.repair[i];
    a.digestion = p.digestion[i];
    a.age01 = std::clamp(p.age[i] / w.cfg.eco.lifespan, 0.0f, 1.0f);
    a.species = p.species[i];
    a.programLength = static_cast<std::uint16_t>(p.program[i].size());
    a.executedInstructions = p.vmExecuted[i];
    for (const auto& instruction : p.program[i]) {
        const std::size_t op = static_cast<std::size_t>(instruction.op);
        if (op < a.opCounts.size()) ++a.opCounts[op];
    }
    for (float value : p.vmState[i]) a.registerActivity += std::fabs(value);
    a.registerActivity /= static_cast<float>(genetics::kVmRegisters);
    a.id = p.id[i];
    return a;
}

}  // namespace

void buildSnapshot(World& w, RenderSnapshot& out, std::uint64_t selectedId) {
    const auto& p = w.pool;
    const std::uint32_t n = p.count();

    out.tick = w.tick;
    out.simTime = w.time;
    out.worldW = w.cfg.worldW;
    out.worldH = w.cfg.worldH;
    out.season = w.season;
    out.births = w.stats.births;
    out.kills = w.stats.kills;
    out.plantEnergy = w.stats.plantEnergy;
    out.animalEnergy = w.stats.animalEnergy;
    out.energyBalanceError =
        w.energyInventory() + w.stats.dissipatedEnergy - w.stats.initialEnergy - w.stats.externalEnergy;

    out.agents.clear();
    out.agents.reserve(n);
    for (std::uint32_t i = 0; i < n; ++i) out.agents.push_back(makeVis(w, i));

    // species, live counts straight from the pool so the list never lags
    std::vector<std::uint32_t> counts(w.species.all().size(), 0);
    for (std::uint32_t i = 0; i < n; ++i) ++counts[p.species[i]];
    out.species.clear();
    for (const auto& s : w.species.all()) {
        if (counts[s.id] == 0) continue;
        SpeciesVis v;
        v.id = s.id; v.parent = s.parent; v.count = counts[s.id];
        v.fullName = s.fullName(); v.shortName = s.shortName();
        v.color = hsv(s.hue, s.sat, s.val);
        v.role = static_cast<RoleVis>(s.role);
        v.labelX = s.labelX; v.labelY = s.labelY;
        v.meanAggr = s.meanAggr; v.meanDef = s.meanDef; v.meanSize = s.meanSize; v.meanSpeed = s.meanSpeed;
        v.meanProgramInstructions = s.meanProgramInstructions;
        v.meanOutputConnectedInstructions = s.meanOutputConnectedInstructions;
        v.established = v.count >= genetics::kEstablished;
        out.species.push_back(std::move(v));
    }
    std::sort(out.species.begin(), out.species.end(),
              [](const SpeciesVis& a, const SpeciesVis& b) { return a.count > b.count; });

    out.edges.clear();
    for (const auto& [key, rate] : w.predation) {
        const auto pred = static_cast<std::uint32_t>(key >> 32), prey = static_cast<std::uint32_t>(key & 0xFFFFFFFFu);
        if (rate < 0.5f || pred >= counts.size() || prey >= counts.size() || !counts[pred] || !counts[prey]) continue;
        out.edges.push_back({pred, prey, rate});
    }
    std::sort(out.edges.begin(), out.edges.end(),
              [](const EdgeVis& a, const EdgeVis& b) { return a.rate > b.rate; });

    out.events.clear();
    for (const auto& e : w.events) {
        EffectVis v;
        v.type = static_cast<EffectVis::Type>(e.type);
        v.x = e.x; v.y = e.y; v.x2 = e.x2; v.y2 = e.y2; v.sp = e.sp; v.sp2 = e.sp2;
        out.events.push_back(v);
    }
    w.events.clear();

    // fields normalized for display
    out.fieldW = w.Q.w;
    out.fieldH = w.Q.h;
    const std::size_t nf = w.Q.v.size();
    out.food0.resize(nf); out.food1.resize(nf); out.heat.resize(nf);
    const float kmax = w.cfg.eco.Kmax;
    for (std::size_t i = 0; i < nf; ++i) {
        out.food0[i] = std::clamp(w.R[0].v[i] / kmax, 0.0f, 1.0f);
        out.food1[i] = std::clamp(w.R[1].v[i] / kmax, 0.0f, 1.0f);
        out.heat[i] = std::clamp(w.Q.v[i] * 0.5f, 0.0f, 1.0f);
    }
    out.tracerX = w.tracerX;
    out.tracerY = w.tracerY;

    // selected organism
    out.selected = {};
    out.selectedRequested = selectedId;
    if (selectedId != 0) {
        for (std::uint32_t i = 0; i < n; ++i) {
            if (p.id[i] != selectedId) continue;
            SelectedVis& s = out.selected;
            s.valid = true;
            s.a = out.agents[i];
            s.energyAbs = p.energy[i];
            s.eMax = p.eMax[i];
            s.ageSec = p.age[i];
            s.plantIntake = p.plantIntake[i];
            s.animalIntake = p.animalIntake[i];
            s.generation = p.generation[i];
            s.parentId = p.parent[i];
            s.units.reserve(p.program[i].size());
            for (std::size_t u = 0; u < p.program[i].size(); ++u) {
                const auto& instruction = p.program[i][u];
                SelectedVis::Unit unit;
                unit.op = static_cast<std::uint8_t>(instruction.op);
                unit.dst = instruction.dst;
                unit.sourceA = instruction.srcA;
                unit.sourceB = instruction.srcB;
                unit.immediate = instruction.immediate;
                const auto valueRegister = instruction.op == genetics::VmOp::Output
                                               ? instruction.srcA
                                               : instruction.dst;
                unit.value = p.vmState[i][valueRegister % genetics::kVmRegisters];
                s.units.push_back(unit);
            }
            s.outputs[0] = p.ax[i];
            s.outputs[1] = p.ay[i];
            s.outputs[2] = (p.harvestGain[i] - 1.0f) / 0.75f;
            s.outputs[3] = p.resourceBias[i];
            s.outputs[4] = p.protection[i];
            s.outputs[5] = p.socialBias[i];
            s.outputs[6] = p.huntBias[i];
            s.outputs[7] = p.reproductionBias[i];
            for (std::uint32_t j = 0; j < n; ++j) {
                if (p.id[j] == p.preyId[i]) { s.hasPrey = true; s.preyX = out.agents[j].x; s.preyY = out.agents[j].y; }
                if (p.id[j] == p.threatId[i]) { s.hasThreat = true; s.threatX = out.agents[j].x; s.threatY = out.agents[j].y; }
            }
            switch (static_cast<AgentAction>(p.action[i])) {
                case AgentAction::Forage: s.state = "FORAGING · tracking food gradients"; break;
                case AgentAction::Hunt: s.state = "HUNTING · pursuing prey"; break;
                case AgentAction::Evade: s.state = "EVADING · avoiding threat"; break;
                case AgentAction::Rest: s.state = "RESTING · energy replete"; break;
                default: s.state = "EXPLORING · searching"; break;
            }
            break;
        }
    }
}

}  // namespace alife::render
