#include "demo_source.hpp"

#include <algorithm>
#include <cmath>

#include "alife/core/grid.hpp"
#include "alife/core/rng.hpp"
#include "alife/math/entropy.hpp"
#include "alife/math/integrator.hpp"

namespace moriarty {
namespace {
constexpr std::size_t kMaxSpecies = 64;
constexpr std::size_t kStartSpecies = 6;
constexpr std::uint64_t kEventEvery = 700;  // ticks between speciation / extinction events
}  // namespace

DemoSource::DemoSource(std::size_t n, float w, float h, std::uint64_t seed)
    : n_(n), w_(w), h_(h), seed_(seed), x_(n), y_(n), vx_(n), vy_(n), energy_(n), species_(n),
      centers_(kMaxSpecies), alive_(kMaxSpecies, false) {
    for (std::size_t s = 0; s < kStartSpecies; ++s) {
        alife::CounterRng r(seed_, 0, 1000 + s);
        centers_[s] = {r.uniform() * w_, r.uniform() * h_, 80.0f + r.uniform() * 220.0f,
                       0.04f + r.uniform() * 0.08f, r.uniform() * 6.2831853f};
        alive_[s] = true;
    }
    nextSpecies_ = kStartSpecies;

    for (std::size_t i = 0; i < n_; ++i) {
        alife::CounterRng r(seed_, 0, i);
        species_[i] = static_cast<std::uint32_t>(r.next() % kStartSpecies);
        centerPos(species_[i], x_[i], y_[i]);
        x_[i] = alife::wrap(x_[i] + r.normal() * 40.0f, w_);
        y_[i] = alife::wrap(y_[i] + r.normal() * 40.0f, h_);
        energy_[i] = r.uniform();
    }
}

void DemoSource::centerPos(std::size_t s, float& x, float& y) const {
    const Center& c = centers_[s];
    x = alife::wrap(c.bx + c.radius * std::cos(c.omega * time_ + c.phase), w_);
    y = alife::wrap(c.by + c.radius * std::sin(c.omega * time_ + c.phase), h_);
}

void DemoSource::step(float dt) {
    dt = std::min(dt, 0.05f);
    time_ += dt;
    ++tick_;

    const alife::math::EulerMaruyama integ{1.5f, 60.0f};
    for (std::size_t i = 0; i < n_; ++i) {
        alife::CounterRng rng(seed_, tick_, i);
        float cx, cy;
        centerPos(species_[i], cx, cy);
        const float fx = 0.5f * alife::wrapDelta(cx - x_[i], w_);
        const float fy = 0.5f * alife::wrapDelta(cy - y_[i], h_);
        integ.step(x_[i], y_[i], vx_[i], vy_[i], fx, fy, 1.0f, dt, rng);
        x_[i] = alife::wrap(x_[i], w_);
        y_[i] = alife::wrap(y_[i], h_);
        energy_[i] = std::clamp(energy_[i] + 0.35f * (rng.uniform() - 0.5f) * dt * 4.0f, 0.0f, 1.0f);
    }

    if (tick_ % kEventEvery == 0) {
        std::size_t living = 0;
        for (bool a : alive_) living += a;
        if (living < 14 && ((tick_ / kEventEvery) % 3 != 0)) speciationEvent();
        else extinctionEvent();
    }
}

void DemoSource::speciationEvent() {
    if (nextSpecies_ >= kMaxSpecies) return;
    // split a random living species, half its members found the new one
    alife::CounterRng r(seed_, tick_, 777);
    std::uint32_t parent = 0;
    do parent = static_cast<std::uint32_t>(r.next() % nextSpecies_); while (!alive_[parent]);

    const std::uint32_t child = nextSpecies_++;
    const Center& p = centers_[parent];
    centers_[child] = {p.bx + (r.uniform() - 0.5f) * 300.0f, p.by + (r.uniform() - 0.5f) * 300.0f,
                       p.radius * (0.6f + r.uniform() * 0.8f), p.omega * (r.uniform() < 0.5f ? -1.0f : 1.0f),
                       r.uniform() * 6.2831853f};
    alive_[child] = true;
    for (std::size_t i = 0; i < n_; ++i)
        if (species_[i] == parent && (alife::mix64(seed_ ^ (i * 31 + tick_)) & 1)) species_[i] = child;
    events_.push_back("speciation");
}

void DemoSource::extinctionEvent() {
    // merge the smallest living species into the largest
    std::vector<std::uint32_t> cnt(kMaxSpecies, 0);
    for (auto s : species_) ++cnt[s];
    std::size_t big = 0, small = kMaxSpecies;
    for (std::size_t s = 0; s < kMaxSpecies; ++s) {
        if (!alive_[s]) continue;
        if (cnt[s] > cnt[big] || !alive_[big]) big = s;
        if (small == kMaxSpecies || cnt[s] < cnt[small]) small = s;
    }
    if (small == kMaxSpecies || small == big) return;
    for (auto& s : species_) if (s == small) s = static_cast<std::uint32_t>(big);
    alive_[small] = false;
    events_.push_back("extinction");
}

void DemoSource::fill(alife::render::RenderSnapshot& out) const {
    // population breathes so the HUD has something to show
    const float breath = 0.82f + 0.18f * std::sin(0.35f * time_);
    const std::size_t live = static_cast<std::size_t>(static_cast<float>(n_) * breath);

    out.tick = tick_;
    out.worldW = w_;
    out.worldH = h_;
    out.x.assign(x_.begin(), x_.begin() + static_cast<std::ptrdiff_t>(live));
    out.y.assign(y_.begin(), y_.begin() + static_cast<std::ptrdiff_t>(live));
    out.energy.assign(energy_.begin(), energy_.begin() + static_cast<std::ptrdiff_t>(live));
    out.species.assign(species_.begin(), species_.begin() + static_cast<std::ptrdiff_t>(live));
}

alife::telemetry::Telemetry DemoSource::telemetry() const {
    const float breath = 0.82f + 0.18f * std::sin(0.35f * time_);
    const std::size_t live = static_cast<std::size_t>(static_cast<float>(n_) * breath);

    std::vector<std::uint32_t> cnt(kMaxSpecies, 0);
    double e = 0.0;
    for (std::size_t i = 0; i < live; ++i) {
        ++cnt[species_[i]];
        e += static_cast<double>(energy_[i]);
    }
    std::uint32_t nsp = 0;
    for (auto c : cnt) nsp += c > 0;

    alife::telemetry::Telemetry t;
    t.tick = tick_;
    t.population = static_cast<std::uint32_t>(live);
    t.speciesCount = nsp;
    t.shannonBits = static_cast<float>(alife::math::shannonBits(cnt));
    t.meanEnergy = live ? static_cast<float>(e / static_cast<double>(live)) : 0.0f;
    return t;
}

std::vector<std::string> DemoSource::takeEvents() {
    std::vector<std::string> out;
    out.swap(events_);
    return out;
}

}  // namespace moriarty
