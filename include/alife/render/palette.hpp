#pragma once
#include <algorithm>
#include <cstdint>

namespace alife::render {

struct Rgb {
    std::uint8_t r, g, b;
};

constexpr Rgb lerp(Rgb a, Rgb b, float t) {
    auto mix = [t](std::uint8_t p, std::uint8_t q) {
        return static_cast<std::uint8_t>(static_cast<float>(p) + (static_cast<float>(q) - static_cast<float>(p)) * t + 0.5f);
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b)};
}

// Mellow autumn palette: warm charcoal ground, muted earth tones for species.
namespace theme {
inline constexpr Rgb bg{27, 23, 20};
inline constexpr Rgb grid{255, 240, 215};  // drawn at very low alpha
inline constexpr Rgb text{232, 220, 200};
inline constexpr Rgb textDim{150, 138, 120};
inline constexpr Rgb panel{36, 31, 27};
inline constexpr Rgb accent{214, 152, 72};
inline constexpr Rgb accentSoft{176, 108, 66};
}  // namespace theme

inline Rgb speciesColor(std::uint32_t id) {
    static constexpr Rgb kBase[] = {
        {201, 100, 66},   // terracotta
        {224, 168, 74},   // ochre
        {138, 154, 91},   // olive
        {164, 84, 98},    // madder
        {212, 129, 52},   // burnt orange
        {120, 160, 142},  // sage teal
        {181, 130, 160},  // dusty plum
        {196, 178, 120},  // wheat
        {150, 96, 64},    // umber
        {222, 140, 120},  // clay rose
        {96, 130, 110},   // fern
        {232, 196, 140},  // apricot
    };
    constexpr std::uint32_t n = sizeof(kBase) / sizeof(kBase[0]);
    // later species cycling through the table get progressively lighter, so ids stay distinguishable
    const float lift = static_cast<float>((id / n) % 3) * 0.14f;
    return lerp(kBase[id % n], Rgb{250, 238, 220}, lift);
}

// Low energy: deep plum, mid: rust, high: amber to cream.
inline Rgb energyColor(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    constexpr Rgb s0{86, 52, 78}, s1{190, 92, 60}, s2{232, 176, 84}, s3{250, 236, 205};
    if (t < 0.4f) return lerp(s0, s1, t / 0.4f);
    if (t < 0.75f) return lerp(s1, s2, (t - 0.4f) / 0.35f);
    return lerp(s2, s3, (t - 0.75f) / 0.25f);
}

}  // namespace alife::render
