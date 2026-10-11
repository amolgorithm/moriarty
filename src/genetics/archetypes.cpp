#include "alife/genetics/archetypes.hpp"

#include <algorithm>

#include "alife/core/config.hpp"
#include "alife/genetics/genome.hpp"

namespace alife::genetics {
namespace {

struct Cue3 { float gain, bias, slope; };  // gain, bias in [-1,1], slope in [-2,2]

struct Arch {
    const char* genus;
    const char* epithet;
    float hue;
    int count;
    float eta, sense, speed, aggr, repThr, size, def, explore;
    Cue3 food, prey, threat, kin;
};

constexpr Arch kArch[kArchetypes] = {
    {"Aurelia", "communis", 28, 260, 0.75f, 0.45f, 0.30f, 0.03f, 0.40f, 0.25f, 0.20f, 0.50f,
     {0.9f, 0.0f, 0.8f}, {0, 0, 0}, {-0.9f, 0.3f, 0}, {0.3f, 0, 0}},
    {"Tessara", "massiva", 42, 100, 0.60f, 0.35f, 0.20f, 0.05f, 0.50f, 0.75f, 0.65f, 0.30f,
     {0.8f, 0.0f, 0.6f}, {0, 0, 0}, {-0.3f, 0, 0}, {0.2f, 0, 0}},
    {"Morvyn", "celer", 172, 200, 0.55f, 0.65f, 0.80f, 0.02f, 0.40f, 0.15f, 0.10f, 0.60f,
     {0.7f, 0.0f, 0.8f}, {0, 0, 0}, {-1.0f, 0.5f, 0}, {0.1f, 0, 0}},
    {"Calyx", "venator", 352, 25, 0.50f, 0.75f, 0.70f, 0.85f, 0.50f, 0.45f, 0.15f, 0.40f,
     {0.1f, 0.0f, 0.5f}, {0.95f, 0.0f, 1.0f}, {-0.4f, 0, 0}, {0, 0, 0}},
    {"Ostrea", "mixta", 84, 40, 0.65f, 0.55f, 0.50f, 0.45f, 0.45f, 0.40f, 0.30f, 0.40f,
     {0.6f, 0.0f, 0.5f}, {0.5f, 0, 0.5f}, {-0.5f, 0, 0}, {0.1f, 0, 0}},
    {"Veridia", "gregaria", 300, 200, 0.70f, 0.60f, 0.45f, 0.05f, 0.40f, 0.20f, 0.15f, 0.50f,
     {0.7f, 0.0f, 0.7f}, {0, 0, 0}, {-0.7f, 0.2f, 0}, {0.9f, 0, 0}},
    {"Pelagon", "spinosa", 205, 100, 0.85f, 0.30f, 0.15f, 0.10f, 0.50f, 0.50f, 0.90f, 0.20f,
     {0.9f, 0.0f, 0.6f}, {0, 0, 0}, {0.0f, 0, 0}, {0.3f, 0, 0}},
};

void setCue(float* g, int cue, Cue3 c) {
    g[cueGain(cue)] = std::clamp((c.gain + 1.0f) / 2.0f, 0.0f, 1.0f);
    g[cueBias(cue)] = std::clamp((c.bias + 1.0f) / 2.0f, 0.0f, 1.0f);
    g[cueSlope(cue)] = std::clamp((c.slope + 2.0f) / 4.0f, 0.0f, 1.0f);
}

}  // namespace

void archetypeGenome(int i, float* g) {
    const Arch& a = kArch[i];
    for (std::size_t k = 0; k < kGenomeLen; ++k) g[k] = 0.5f;
    g[kEta] = a.eta; g[kSense] = a.sense; g[kSpeed] = a.speed; g[kAggr] = a.aggr;
    g[kRepThr] = a.repThr; g[kMutRate] = 0.30f; g[kSize] = a.size; g[kDefense] = a.def;
    g[kHueOff] = 0.5f; g[kPattern] = 0.5f; g[kExplore] = a.explore;
    setCue(g, kFood, a.food);
    setCue(g, kPrey, a.prey);
    setCue(g, kThreat, a.threat);
    setCue(g, kKin, a.kin);
}

const char* archetypeGenus(int i) { return kArch[i].genus; }
const char* archetypeEpithet(int i) { return kArch[i].epithet; }
float archetypeHue(int i) { return kArch[i].hue; }
int archetypeCount(int i) { return kArch[i].count; }

}  // namespace alife::genetics
