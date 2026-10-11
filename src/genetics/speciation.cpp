#include "alife/genetics/speciation.hpp"

#include <algorithm>
#include <cmath>

#include "alife/core/grid.hpp"

namespace alife::genetics {
namespace {

const char* kOnset[] = {"v", "k", "l", "m", "n", "r", "s", "t", "z", "br", "th", "dr", "pl", "gl"};
const char* kVowel[] = {"a", "e", "i", "o", "u", "ae", "ia", "ei"};
const char* kEnd[] = {"a", "us", "is", "um", "on", "ix", "ensis", "ina"};

std::string makeEpithet(std::uint64_t seed) {
    CounterRng r(seed, 0, 0xE91);
    auto pick = [&](auto& arr) { return arr[r.next() % (sizeof(arr) / sizeof(arr[0]))]; };
    std::string s = pick(kOnset);
    s += pick(kVowel);
    if (r.uniform() < 0.6f) { s += pick(kOnset); s += pick(kVowel); }
    s += pick(kEnd);
    return s;
}

float clampf(float v, float lo, float hi) { return std::min(hi, std::max(lo, v)); }

}  // namespace

void SpeciesTable::project(const float* g, float* out) {
    for (std::size_t i = 0; i < kDistGenes; ++i) out[i] = g[kDistIndex[i]];
}

float SpeciesTable::distance(const float* p, const std::array<float, kDistGenes>& ex) {
    float s = 0.0f;
    for (std::size_t i = 0; i < kDistGenes; ++i) {
        const float d = p[i] - ex[i];
        s += d * d;
    }
    return std::sqrt(s / static_cast<float>(kDistGenes));
}

std::uint32_t SpeciesTable::addFounder(const float* genome, const char* genus, const char* epithet,
                                       float hue, std::uint32_t genusIndex, std::uint64_t tick) {
    Species s;
    s.id = static_cast<std::uint32_t>(sp_.size());
    s.genus = genusIndex;
    s.genusName = genus;
    s.epithet = epithet;
    s.hue = hue; s.sat = 0.58f; s.val = 0.88f;
    project(genome, s.exemplar.data());
    s.founded = tick;
    s.announced = true;
    sp_.push_back(std::move(s));
    return sp_.back().id;
}

std::uint32_t SpeciesTable::classify(const float* genome, std::uint32_t parentSp, std::uint64_t tick,
                                     CounterRng& rng) {
    float p[kDistGenes];
    project(genome, p);

    if (parentSp != kNone && parentSp < sp_.size() && sp_[parentSp].alive &&
        distance(p, sp_[parentSp].exemplar) <= delta)
        return parentSp;

    std::uint32_t best = kNone;
    float bestD = delta;
    for (const auto& s : sp_) {
        if (!s.alive) continue;
        const float d = distance(p, s.exemplar);
        if (d <= bestD) { bestD = d; best = s.id; }
    }
    if (best != kNone) return best;

    // new species, branching off the parent's clade
    Species s;
    s.id = static_cast<std::uint32_t>(sp_.size());
    s.parent = parentSp;
    const Species* par = (parentSp != kNone && parentSp < sp_.size()) ? &sp_[parentSp] : nullptr;
    s.genus = par ? par->genus : s.id;
    s.genusName = par ? par->genusName : "Incerta";
    s.epithet = makeEpithet(rng.next() ^ (tick * 0x9E37ull));
    const float h0 = par ? par->hue : 30.0f;
    s.hue = std::fmod(h0 + (rng.uniform() * 2.0f - 1.0f) * 26.0f + 360.0f, 360.0f);
    s.sat = clampf((par ? par->sat : 0.6f) + (rng.uniform() - 0.5f) * 0.22f, 0.40f, 0.78f);
    s.val = clampf((par ? par->val : 0.85f) + (rng.uniform() - 0.5f) * 0.22f, 0.62f, 0.95f);
    std::copy(p, p + kDistGenes, s.exemplar.begin());
    s.founded = tick;
    sp_.push_back(std::move(s));
    return sp_.back().id;
}

void SpeciesTable::revive(std::uint32_t id, std::uint64_t tick) {
    Species& s = sp_[id];
    s.alive = true;
    s.extinct = 0;
    s.founded = tick;
    s.peak = 0;
    s.announced = false;
}

void SpeciesTable::refresh(const AgentPool& pool, float W, float H, std::uint64_t tick,
                           std::vector<std::string>& markers, LineageLog* lineage) {
    const std::size_t ns = sp_.size();
    constexpr int GX = 12, GY = 8;  // coarse grid for finding where a species mostly lives
    std::vector<std::array<float, kDistGenes>> sum(ns);
    std::vector<std::vector<std::uint16_t>> hist(ns);
    for (auto& s : sp_) {
        s.count = 0;
        s.meanAggr = s.meanDef = s.meanSize = s.meanSpeed = 0;
    }
    for (auto& a : sum) a.fill(0.0f);

    const auto& k = pool.read();
    const std::uint32_t n = pool.count();
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uint32_t id = pool.species[i];
        Species& s = sp_[id];
        ++s.count;
        const float* g = &pool.genome[static_cast<std::size_t>(i) * kGenomeLen];
        for (std::size_t d = 0; d < kDistGenes; ++d) sum[id][d] += g[kDistIndex[d]];
        s.meanAggr += pool.aggression[i];
        s.meanDef += pool.defense[i];
        s.meanSize += pool.radius[i];
        s.meanSpeed += pool.vMax[i];
        if (hist[id].empty()) hist[id].assign(GX * GY, 0);
        const int cx = std::min(GX - 1, static_cast<int>(k.px[i] / W * GX));
        const int cy = std::min(GY - 1, static_cast<int>(k.py[i] / H * GY));
        ++hist[id][static_cast<std::size_t>(cy * GX + cx)];
    }

    // label anchor: mean position of members in the densest coarse cell and its neighbors
    std::vector<std::array<float, 3>> anchor(ns, {0, 0, 0});
    std::vector<int> modeCell(ns, -1);
    for (std::size_t id = 0; id < ns; ++id) {
        if (hist[id].empty()) continue;
        int best = 0;
        std::uint32_t bestScore = 0;
        for (int c = 0; c < GX * GY; ++c) {
            std::uint32_t score = 0;
            const int cx = c % GX, cy = c / GX;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    score += hist[id][static_cast<std::size_t>(((cy + dy + GY) % GY) * GX + (cx + dx + GX) % GX)];
            if (score > bestScore) { bestScore = score; best = c; }
        }
        modeCell[id] = best;
    }
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uint32_t id = pool.species[i];
        if (modeCell[id] < 0) continue;
        const int mx = modeCell[id] % GX, my = modeCell[id] / GX;
        const int cx = std::min(GX - 1, static_cast<int>(k.px[i] / W * GX));
        const int cy = std::min(GY - 1, static_cast<int>(k.py[i] / H * GY));
        const int ddx = std::min((cx - mx + GX) % GX, (mx - cx + GX) % GX);
        const int ddy = std::min((cy - my + GY) % GY, (my - cy + GY) % GY);
        if (ddx > 1 || ddy > 1) continue;
        const float refx = (static_cast<float>(mx) + 0.5f) * W / GX;
        const float refy = (static_cast<float>(my) + 0.5f) * H / GY;
        anchor[id][0] += wrapDelta(k.px[i] - refx, W);
        anchor[id][1] += wrapDelta(k.py[i] - refy, H);
        anchor[id][2] += 1.0f;
    }

    for (auto& s : sp_) {
        if (!s.alive) continue;
        if (s.count == 0) {
            if (tick > s.founded + 2) {
                s.alive = false;
                s.extinct = tick;
                if (s.peak >= kEstablished) markers.push_back("extinct: " + s.shortName());
                if (lineage) lineage->record({tick, 0, 0, s.id, LineageEvent::SpeciesExtinct});
            }
            continue;
        }
        const float inv = 1.0f / static_cast<float>(s.count);
        for (std::size_t d = 0; d < kDistGenes; ++d)
            s.exemplar[d] = 0.7f * s.exemplar[d] + 0.3f * (sum[s.id][d] * inv);
        s.meanAggr *= inv; s.meanDef *= inv; s.meanSize *= inv; s.meanSpeed *= inv;
        s.peak = std::max(s.peak, s.count);
        s.role = s.meanAggr < 0.22f ? Role::Grazer : (s.meanAggr < 0.55f ? Role::Omnivore : Role::Predator);
        if (anchor[s.id][2] > 0 && modeCell[s.id] >= 0) {
            const int mx = modeCell[s.id] % GX, my = modeCell[s.id] / GX;
            const float refx = (static_cast<float>(mx) + 0.5f) * W / GX;
            const float refy = (static_cast<float>(my) + 0.5f) * H / GY;
            s.labelX = wrap(refx + anchor[s.id][0] / anchor[s.id][2], W);
            s.labelY = wrap(refy + anchor[s.id][1] / anchor[s.id][2], H);
        }
        if (!s.announced && s.count >= kEstablished) {
            s.announced = true;
            markers.push_back("new species: " + s.shortName());
            if (lineage) lineage->record({tick, 0, 0, s.id, LineageEvent::SpeciesFounded});
        }
    }
}

}  // namespace alife::genetics
