#include "alife/sim/meso.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "alife/genetics/gpm.hpp"
#include "alife/genetics/operators.hpp"
#include "alife/sim/environment.hpp"

namespace alife::sim {
namespace {

enum Cause : std::uint8_t { kAlive = 0, kKilled, kStarved, kOld };

struct Birth {
    std::array<float, kGenomeLen> genome;
    genetics::Circuit circuit;
    float x, y, energy;
    std::uint32_t parentSpecies, generation;
    std::uint64_t parentId;
    float vx, vy;
};

}  // namespace

// Metabolic layer, 0.25 s: sensing -> controller, feeding, predation, upkeep,
// death, reproduction. Fully endogenous: no fitness function anywhere.
void mesoStep(World& w, ThreadPool&) {
    auto& p = w.pool;
    const auto& cfg = w.cfg;
    const auto& eco = cfg.eco;
    const float dt = cfg.dtPhys * static_cast<float>(cfg.mesoEvery);
    const float W = cfg.worldW, H = cfg.worldH;

    stepEnvironment(w, dt);

    const std::uint32_t n = p.count();
    if (n == 0) return;

    auto& sc = w.scratch;
    sc.preyIdx.assign(n, -1);
    sc.preyDist.assign(n, 1e9f);
    sc.crowd.assign(n, 0);
    sc.dead.assign(n, 0);
    sc.cause.assign(n, kAlive);
    const auto& k = p.read();

    // ---- pass 1: sense the neighborhood, express the inherited circuit ----
    for (std::uint32_t i = 0; i < n; ++i) {
        const float x = k.px[i], y = k.py[i], sr = p.senseR[i], sr2 = sr * sr;
        int prey = -1, threat = -1;
        float dPrey = 1e9f, dThreat = 1e9f;
        float pdx = 0, pdy = 0, tdx = 0, tdy = 0, kx = 0, ky = 0;
        int kn = 0, crowd = 0;
        w.grid.forNeighbors(x, y, [&](std::uint32_t j) {
            if (j == i) return;
            const float dx = wrapDelta(k.px[j] - x, W), dy = wrapDelta(k.py[j] - y, H);
            const float d2 = dx * dx + dy * dy;
            if (d2 < 576.0f) ++crowd;
            if (d2 >= sr2) return;
            if (p.species[j] == p.species[i]) { kx += dx; ky += dy; ++kn; }
            const float d = std::sqrt(d2);
            if (p.radius[j] <= p.radius[i] * 0.9f && d < dPrey) {
                dPrey = d;
                prey = static_cast<int>(j);
                pdx = dx;
                pdy = dy;
            }
            if (p.aggression[j] > 0.3f && p.radius[j] >= p.radius[i] * 0.7f && d < dThreat) {
                dThreat = d; threat = static_cast<int>(j); tdx = dx; tdy = dy;
            }
        });

        // cue vectors: direction scaled by proximity
        float fx = 0, fy = 0;
        {
            const float sz = (p.radius[i] - 3.0f) / 5.5f;
            const float pf = std::pow(std::max(0.0f, 1.0f - sz), 1.5f), pc = std::pow(std::max(0.0f, sz), 1.5f);
            const float d = std::max(16.0f, 0.6f * sr);
            auto food = [&](float qx, float qy) {
                return pf * w.R[0].sample(qx, qy, W, H) + pc * w.R[1].sample(qx, qy, W, H);
            };
            const float gx = food(x + d, y) - food(x - d, y);
            const float gy = food(x, y + d) - food(x, y - d);
            const float m = std::hypot(gx, gy), scale = 0.15f * eco.Kmax;
            if (m > 1e-5f) { const float s = std::min(1.0f, m / scale); fx = gx / m * s; fy = gy / m * s; }
        }
        float cx = 0, cy = 0, thx = 0, thy = 0, kcx = 0, kcy = 0;
        if (prey >= 0) { const float s = 0.3f + 0.7f * (1.0f - dPrey / sr); cx = pdx / dPrey * s; cy = pdy / dPrey * s; }
        if (threat >= 0) { const float s = 0.3f + 0.7f * (1.0f - dThreat / sr); thx = tdx / dThreat * s; thy = tdy / dThreat * s; }
        if (kn > 0) {
            const float mx = kx / static_cast<float>(kn), my = ky / static_cast<float>(kn), m = std::hypot(mx, my);
            if (m > 1e-4f) { const float s = std::min(1.0f, m / (0.5f * sr)); kcx = mx / m * s; kcy = my / m * s; }
        }

        const float eFrac = p.energy[i] / p.eMax[i];
        genetics::CircuitInputs inputs{};
        inputs[0] = fx;
        inputs[1] = fy;
        inputs[2] = cx;
        inputs[3] = cy;
        inputs[4] = thx;
        inputs[5] = thy;
        inputs[6] = kcx;
        inputs[7] = kcy;
        inputs[8] = 1.0f - 2.0f * std::clamp(eFrac, 0.0f, 1.0f);
        inputs[9] = std::clamp((w.R[1].sample(x, y, W, H) - w.R[0].sample(x, y, W, H)) /
                                   std::max(1.0f, eco.Kmax),
                               -1.0f, 1.0f);
        const auto [currentX, currentY] = w.flowAt(x, y);
        inputs[10] = std::clamp(currentX / eco.vTop, -1.0f, 1.0f);
        inputs[11] = std::clamp(currentY / eco.vTop, -1.0f, 1.0f);
        const auto expression = genetics::evaluateCircuit(p.circuit[i], inputs);
        p.circuitState[i] = expression.units;
        p.ax[i] = 1.5f * expression.outputs[0];
        p.ay[i] = 1.5f * expression.outputs[1];
        p.harvestGain[i] = std::clamp(1.0f + 0.75f * expression.outputs[2], 0.2f, 1.8f);
        p.resourceBias[i] = expression.outputs[3];
        p.protection[i] = std::max(0.0f, expression.outputs[4]);
        sc.crowd[i] = static_cast<std::uint8_t>(std::min(crowd, 40));
        sc.preyIdx[i] = prey;
        sc.preyDist[i] = dPrey;
        p.preyId[i] = prey >= 0 ? p.id[static_cast<std::size_t>(prey)] : 0;
        p.threatId[i] = threat >= 0 ? p.id[static_cast<std::size_t>(threat)] : 0;
    }

    // ---- pass 2: feeding, predation, upkeep, death (random order, no positional bias) ----
    CounterRng orderRng(cfg.seed, w.tick, 0xBEEFull);
    const std::uint32_t start = static_cast<std::uint32_t>(orderRng.next() % n);
    for (std::uint32_t t = 0; t < n; ++t) {
        const std::uint32_t i = (start + t) % n;
        if (sc.dead[i]) continue;
        CounterRng rng(cfg.seed, w.tick, p.id[i] ^ 0x5A5A5A5Aull);
        const float x = k.px[i], y = k.py[i];
        const std::size_t cell = w.Q.idx(x, y);
        const float eMax = p.eMax[i], aggr = p.aggression[i];

        // grazing: size decides which food channel a body is built for (disruptive selection)
        if (p.energy[i] < eMax) {
            const float sz = std::clamp((p.radius[i] - 3.0f) / 5.5f, 0.0f, 1.0f);
            const float pref[2] = {std::pow(1.0f - sz, 1.5f), std::pow(sz, 1.5f)};
            const float appetite = eco.bite * std::pow(p.mass[i], 0.75f) *
                                   std::pow(1.0f - aggr, 1.5f) * p.harvestGain[i] * dt;
            const float gain = eco.energyPerR * (0.35f + 0.65f * p.eta[i]);
            float room = (eMax - p.energy[i]) / gain;
            float pref0 = std::clamp(pref[0] - 0.35f * p.resourceBias[i], 0.0f, 1.0f);
            float pref1 = std::clamp(pref[1] + 0.35f * p.resourceBias[i], 0.0f, 1.0f);
            const float prefSum = pref0 + pref1;
            if (prefSum > 0.0f) { pref0 /= prefSum; pref1 /= prefSum; }
            else { pref0 = 0.5f; pref1 = 0.5f; }
            for (int ch = 0; ch < 2; ++ch) {
                const float taken = std::min({appetite * (ch == 0 ? pref0 : pref1), w.R[ch].v[cell] * 0.6f, room});
                w.R[ch].v[cell] -= taken;
                p.energy[i] += taken * gain;
                room -= taken;
            }
        }

        // predation
        const int j = sc.preyIdx[i];
        p.digest[i] = std::max(0.0f, p.digest[i] - dt);
        if (aggr > 0.2f && j >= 0 && !sc.dead[static_cast<std::size_t>(j)] && p.digest[i] <= 0.0f &&
            p.energy[i] < eco.huntBelow * eMax) {
            const std::size_t J = static_cast<std::size_t>(j);
            const float reach = p.radius[i] + p.radius[J] + 0.5f * p.vMax[i] * dt + 2.0f;
            if (sc.preyDist[i] <= reach) {
                const float atk = aggr * (0.6f + 0.08f * p.radius[i]) * (0.55f + 0.45f * p.vMax[i] / eco.vTop);
                const float def = 0.45f + 1.4f * (p.defense[J] + 0.35f * p.protection[J]) +
                                  0.5f * p.vMax[J] / eco.vTop;
                p.energy[i] -= eco.lunge;
                if (rng.uniform() < atk / (atk + def)) {
                    const float carcass = std::max(0.0f, p.energy[J]) + eco.carcassPerMass * p.mass[J];
                    p.energy[i] = std::min(eMax * 1.3f, p.energy[i] + eco.predEff * carcass);
                    w.R[0].v[w.Q.idx(k.px[J], k.py[J])] += 0.5f * eco.recycle * (1.0f - eco.predEff) * carcass / eco.energyPerR;
                    w.R[1].v[w.Q.idx(k.px[J], k.py[J])] += 0.5f * eco.recycle * (1.0f - eco.predEff) * carcass / eco.energyPerR;
                    p.digest[i] = eco.digestTime;
                    sc.dead[J] = 1;
                    sc.cause[J] = kKilled;
                    ++w.stats.kills;
                    const std::uint64_t key = (static_cast<std::uint64_t>(p.species[i]) << 32) | p.species[J];
                    w.predation[key] += 1.0f;
                    w.emit({Event::Kill, x, y, k.px[J], k.py[J], p.species[i], p.species[J]});
                }
            }
        }

        // upkeep with individual metabolic noise
        const float speed = std::hypot(k.vx[i], k.vy[i]);
        const float m75 = std::pow(p.mass[i], 0.75f);
        const float noise = std::exp(0.15f * rng.normal());
        const float stress = 1.0f + eco.crowdStress * static_cast<float>(sc.crowd[i]);
        const float cost = stress * eco.basal * m75 * (0.7f + 0.6f * p.eta[i]) * noise +
                           eco.senseCost * p.senseR[i] * p.senseR[i] +
                           eco.moveCost * p.mass[i] * speed * speed + eco.defenseCost * p.defense[i] * m75 +
                           eco.aggrCost * aggr * m75 +
                           eco.defenseCost * 0.35f * p.protection[i] * m75 +
                           eco.circuitCost * static_cast<float>(p.circuit[i].size()) * m75;
        p.energy[i] -= cost * dt;
        w.Q.v[cell] += eco.heatFrac * cost * dt;
        p.age[i] += dt;

        // death
        if (p.energy[i] <= 0.0f) {
            sc.dead[i] = 1; sc.cause[i] = kStarved; ++w.stats.starved;
        } else {
            // frequency-dependent mortality (kill-the-winner): common species carry more disease
            const float share = static_cast<float>(w.species.at(p.species[i]).count) / static_cast<float>(n);
            if (rng.uniform() < 1.0f - std::exp(-eco.pathogen * share * share * dt)) {
                sc.dead[i] = 1; sc.cause[i] = kStarved; ++w.stats.starved;
            }
            const float life = eco.lifespan * (0.8f + 0.25f * std::sqrt(p.mass[i]));
            if (!sc.dead[i] && p.age[i] > 0.5f * life) {
                const float h = 0.5f * std::pow(p.age[i] / life, 4.0f);
                if (rng.uniform() < 1.0f - std::exp(-h * dt)) { sc.dead[i] = 1; sc.cause[i] = kOld; ++w.stats.senescent; }
            }
        }
        if (sc.dead[i] && sc.cause[i] != kKilled) {
            const float carcass = std::max(0.0f, p.energy[i]) + eco.carcassPerMass * p.mass[i];
            w.R[0].v[cell] += 0.5f * eco.recycle * carcass / eco.energyPerR;
            w.R[1].v[cell] += 0.5f * eco.recycle * carcass / eco.energyPerR;
            w.emit({Event::Death, x, y, 0, 0, p.species[i], 0});
        }
    }

    // ---- pass 3: reproduction (asexual, or recombination with a nearby conspecific) ----
    std::vector<Birth> births;
    std::vector<std::uint8_t> mated(n, 0);
    const std::size_t room = cfg.maxAgents > n ? cfg.maxAgents - n : 0;
    for (std::uint32_t i = 0; i < n && births.size() < room; ++i) {
        if (sc.dead[i] || p.age[i] < eco.maturity || p.energy[i] < p.repThresh[i] * p.eMax[i]) continue;
        CounterRng rng(cfg.seed, w.tick, p.id[i] ^ 0xB177ull);
        const float x = k.px[i], y = k.py[i];

        int mate = -1;
        float best = 28.0f * 28.0f;
        w.grid.forNeighbors(x, y, [&](std::uint32_t j) {
            if (j == i || sc.dead[j] || mated[j] || p.species[j] != p.species[i]) return;
            if (p.age[j] < eco.maturity || p.energy[j] < 0.5f * p.repThresh[j] * p.eMax[j]) return;
            const float dx = wrapDelta(k.px[j] - x, W), dy = wrapDelta(k.py[j] - y, H);
            const float d2 = dx * dx + dy * dy;
            if (d2 < best) { best = d2; mate = static_cast<int>(j); }
        });

        Birth b;
        const float* gi = &p.genome[static_cast<std::size_t>(i) * kGenomeLen];
        float rate = 0.004f + 0.076f * gi[genetics::kMutRate];
        if (mate >= 0 && rng.uniform() < 0.7f) {
            const float* gj = &p.genome[static_cast<std::size_t>(mate) * kGenomeLen];
            genetics::crossover(gi, gj, b.genome.data(), rng);
            b.circuit = genetics::crossoverCircuit(p.circuit[i], p.circuit[static_cast<std::size_t>(mate)], rng);
            rate = 0.5f * (rate + 0.004f + 0.076f * gj[genetics::kMutRate]);
            p.energy[static_cast<std::size_t>(mate)] -= 0.08f * p.eMax[static_cast<std::size_t>(mate)];
            mated[static_cast<std::size_t>(mate)] = 1;
        } else {
            std::copy(gi, gi + kGenomeLen, b.genome.begin());
            b.circuit = p.circuit[i];
        }
        genetics::mutate(b.genome.data(), rate, rng);
        genetics::mutateCircuit(b.circuit, rate, rng);

        const auto cph = genetics::decode(b.genome.data(), eco.vTop);
        const float constructionCost = eco.unitBuildCost * static_cast<float>(b.circuit.size());
        b.energy = std::min(0.5f * cph.eMax, 0.4f * p.energy[i] - constructionCost);
        p.energy[i] -= std::max(0.0f, b.energy) + constructionCost + 0.04f * p.eMax[i];
        b.x = wrap(x + p.radius[i] * rng.normal(), W);
        b.y = wrap(y + p.radius[i] * rng.normal(), H);
        b.vx = k.vx[i]; b.vy = k.vy[i];
        b.parentSpecies = p.species[i];
        b.parentId = p.id[i];
        b.generation = p.generation[i] + 1;
        births.push_back(b);
        mated[i] = 1;
    }

    // ---- apply: remove the dead (descending, swap-remove), then spawn ----
    for (std::uint32_t i = n; i-- > 0;)
        if (sc.dead[i]) p.swapRemove(i);

    for (auto& b : births) {
        CounterRng rng(cfg.seed, w.tick, b.parentId ^ 0x5BECull);
        const std::uint32_t sp = w.species.classify(b.genome.data(), b.circuit, b.parentSpecies, w.tick, rng);
        const std::uint32_t idx = w.addAgent(b.genome.data(), b.x, b.y, std::max(0.0f, b.energy), sp,
                                             b.parentId, b.generation, b.circuit);
        p.read().vx[idx] = b.vx; p.read().vy[idx] = b.vy;
        p.write().vx[idx] = b.vx; p.write().vy[idx] = b.vy;
        p.heading[idx] = std::atan2(b.vy, b.vx);
        ++w.stats.births;
        w.emit({Event::Birth, b.x, b.y, 0, 0, sp, 0});
    }
}

}  // namespace alife::sim
