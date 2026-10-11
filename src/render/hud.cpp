#include "alife/render/hud.hpp"

#include <algorithm>
#include <cstdio>

#include "alife/render/palette.hpp"

namespace alife::render {
namespace {

Color col(Rgb c, unsigned char a = 255) { return Color{c.r, c.g, c.b, a}; }

void label(const Font& f, const char* s, float x, float y, float size, Color c) {
    DrawTextEx(f, s, {x, y}, size, 1.0f, c);
}

void rightLabel(const Font& f, const char* s, float xRight, float y, float size, Color c) {
    const Vector2 d = MeasureTextEx(f, s, size, 1.0f);
    DrawTextEx(f, s, {xRight - d.x, y}, size, 1.0f, c);
}

void panel(Rectangle r) {
    DrawRectangleRounded(r, 0.06f, 8, col(theme::panel, 205));
    DrawRectangleRoundedLinesEx(r, 0.06f, 8, 1.0f, col(theme::textDim, 40));
}

struct ChartArgs {
    const char* name;
    const telemetry::RingBuffer<float>& series;
    Color line;
    const char* value;
    bool zeroBased;
    bool markerLabels;
};

void chart(const Font& f, Rectangle r, const ChartArgs& a,
           const std::vector<telemetry::PhaseMarker>& markers) {
    label(f, a.name, r.x, r.y, 13, col(theme::textDim));
    rightLabel(f, a.value, r.x + r.width, r.y, 13, col(theme::text));

    const Rectangle plot{r.x, r.y + 20, r.width, r.height - 20};
    DrawLineEx({plot.x, plot.y + plot.height}, {plot.x + plot.width, plot.y + plot.height}, 1.0f,
               col(theme::textDim, 50));

    const std::size_t n = a.series.size();
    if (n < 2) return;

    float lo = a.series.at(0), hi = lo;
    for (std::size_t i = 1; i < n; ++i) {
        lo = std::min(lo, a.series.at(i));
        hi = std::max(hi, a.series.at(i));
    }
    if (a.zeroBased) lo = 0.0f;
    if (hi - lo < 1e-6f) hi = lo + 1.0f;

    const float step = plot.width / static_cast<float>(a.series.capacity() - 1);
    const float xRight = plot.x + plot.width;
    auto px = [&](std::size_t i) { return xRight - static_cast<float>(n - 1 - i) * step; };
    auto py = [&](float v) {
        return plot.y + plot.height - 2.0f - (v - lo) / (hi - lo) * (plot.height - 6.0f);
    };

    // phase markers behind the curve
    const std::uint64_t first = a.series.totalPushed() - n;
    for (const auto& m : markers) {
        if (m.sample < first) continue;
        const float x = px(static_cast<std::size_t>(m.sample - first));
        DrawLineEx({x, plot.y}, {x, plot.y + plot.height}, 1.0f, col(theme::accent, 90));
        if (a.markerLabels) label(f, m.label.c_str(), x + 4, plot.y + 2, 11, col(theme::accent, 200));
    }

    for (std::size_t i = 0; i + 1 < n; ++i) {
        DrawLineEx({px(i), py(a.series.at(i))}, {px(i + 1), py(a.series.at(i + 1))}, 1.6f, a.line);
    }
}

}  // namespace

void Hud::draw(const Font& font, const telemetry::TelemetryHistory& hist, const ViewState& vs,
               int fps, int sw, int sh) const {
    if (!vs.hud) return;
    const auto& t = hist.latest();
    char buf[64];

    // stat block
    const Rectangle sp{16, 16, 220, 150};
    panel(sp);
    label(font, "moriarty", sp.x + 14, sp.y + 12, 20, col(theme::accent));
    label(font, vs.paused ? "paused" : "running", sp.x + 14, sp.y + 38, 12,
          col(vs.paused ? theme::accentSoft : theme::textDim));

    struct Row { const char* k; char v[32]; };
    Row rows[4] = {{"tick", {}}, {"population", {}}, {"species", {}}, {"entropy", {}}};
    std::snprintf(rows[0].v, sizeof rows[0].v, "%llu", static_cast<unsigned long long>(t.tick));
    std::snprintf(rows[1].v, sizeof rows[1].v, "%u", t.population);
    std::snprintf(rows[2].v, sizeof rows[2].v, "%u", t.speciesCount);
    std::snprintf(rows[3].v, sizeof rows[3].v, "%.2f bits", static_cast<double>(t.shannonBits));
    for (int i = 0; i < 4; ++i) {
        const float y = sp.y + 64 + static_cast<float>(i) * 20;
        label(font, rows[i].k, sp.x + 14, y, 13, col(theme::textDim));
        rightLabel(font, rows[i].v, sp.x + sp.width - 14, y, 13, col(theme::text));
    }

    // charts
    const float cw = 340, ch = 78, pad = 14;
    const Rectangle cp{static_cast<float>(sw) - cw - 2 * pad - 16,
                       static_cast<float>(sh) - 3 * ch - 4 * pad - 16, cw + 2 * pad, 3 * ch + 4 * pad};
    panel(cp);

    char v0[32], v1[32], v2[32];
    std::snprintf(v0, sizeof v0, "%u", t.population);
    std::snprintf(v1, sizeof v1, "%.2f bits", static_cast<double>(t.shannonBits));
    std::snprintf(v2, sizeof v2, "%u", t.speciesCount);

    chart(font, {cp.x + pad, cp.y + pad, cw, ch},
          {"population", hist.population, col(theme::accent), v0, false, true}, hist.markers);
    chart(font, {cp.x + pad, cp.y + 2 * pad + ch, cw, ch},
          {"shannon entropy", hist.entropy, col(speciesColor(2)), v1, true, false}, hist.markers);
    chart(font, {cp.x + pad, cp.y + 3 * pad + 2 * ch, cw, ch},
          {"species", hist.species, col(speciesColor(3)), v2, true, false}, hist.markers);

    // footer
    std::snprintf(buf, sizeof buf, "%d fps   x%.2g", fps, static_cast<double>(vs.speed));
    label(font, buf, 16, static_cast<float>(sh) - 40, 12, col(theme::textDim));
    label(font, "space pause   1/2 color   t trails   h hud   f fit   up/down speed   drag + wheel to move",
          16, static_cast<float>(sh) - 24, 12, col(theme::textDim, 150));
}

}  // namespace alife::render
