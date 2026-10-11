#include "alife/render/hud.hpp"

#include <algorithm>
#include <cmath>
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
    DrawRectangleRounded(r, 0.04f, 8, col(theme::panel, 215));
    DrawRectangleRoundedLinesEx(r, 0.04f, 8, 1.0f, col(theme::textDim, 45));
}

// horizontal 0..1 bar with a label on the left and the value on the right
void bar(const Font& f, const char* name, float v, float x, float y, float w, Color c) {
    label(f, name, x, y, 12, col(theme::textDim));
    const float bx = x + 78, bw = w - 78 - 36;
    DrawRectangleRounded({bx, y + 3, bw, 7}, 0.5f, 4, col(theme::textDim, 40));
    DrawRectangleRounded({bx, y + 3, std::max(2.0f, bw * std::clamp(v, 0.0f, 1.0f)), 7}, 0.5f, 4, c);
    char b[16];
    std::snprintf(b, sizeof b, "%.0f%%", static_cast<double>(v * 100.0f));
    rightLabel(f, b, x + w, y, 12, col(theme::text));
}

// signed bar centered on a midline, v in [-2, 2]
void signedBar(const Font& f, const char* name, float v, float x, float y, float w) {
    label(f, name, x, y, 12, col(theme::textDim));
    const float bx = x + 78, bw = w - 78 - 36, mid = bx + bw * 0.5f;
    DrawRectangleRounded({bx, y + 3, bw, 7}, 0.5f, 4, col(theme::textDim, 40));
    const float len = std::clamp(v / 2.0f, -1.0f, 1.0f) * bw * 0.5f;
    const Color c = v >= 0 ? col(theme::accent) : col({176, 84, 96});
    DrawRectangleRec({std::min(mid, mid + len), y + 3, std::max(2.0f, std::fabs(len)), 7}, c);
    DrawLineEx({mid, y + 1}, {mid, y + 12}, 1.0f, col(theme::text, 140));
    char b[16];
    std::snprintf(b, sizeof b, "%+.1f", static_cast<double>(v));
    rightLabel(f, b, x + w, y, 12, col(theme::text));
}

struct ChartArgs {
    const char* name;
    const telemetry::RingBuffer<float>& series;
    Color line;
    const char* value;
    bool zeroBased;
    bool markerLabels;
};

void chart(const Font& f, Rectangle r, const ChartArgs& a, const std::vector<telemetry::PhaseMarker>& markers) {
    label(f, a.name, r.x, r.y, 13, col(theme::textDim));
    rightLabel(f, a.value, r.x + r.width, r.y, 13, col(theme::text));

    const Rectangle plot{r.x, r.y + 20, r.width, r.height - 20};
    DrawLineEx({plot.x, plot.y + plot.height}, {plot.x + plot.width, plot.y + plot.height}, 1.0f, col(theme::textDim, 50));

    const std::size_t n = a.series.size();
    if (n < 2) return;
    float lo = a.series.at(0), hi = lo;
    for (std::size_t i = 1; i < n; ++i) {
        lo = std::min(lo, a.series.at(i));
        hi = std::max(hi, a.series.at(i));
    }
    if (a.zeroBased) lo = 0.0f;
    if (hi - lo < 1e-6f) hi = lo + 1.0f;

    const float step = plot.width / static_cast<float>(std::max<std::size_t>(n, 90) - 1);
    const float xRight = plot.x + plot.width;
    auto px = [&](std::size_t i) { return xRight - static_cast<float>(n - 1 - i) * step; };
    auto py = [&](float v) { return plot.y + plot.height - 2.0f - (v - lo) / (hi - lo) * (plot.height - 6.0f); };

    const std::uint64_t first = a.series.totalPushed() - n;
    const telemetry::PhaseMarker* latest = nullptr;
    for (const auto& m : markers) {
        if (m.sample < first) continue;
        const float x = px(static_cast<std::size_t>(m.sample - first));
        DrawLineEx({x, plot.y}, {x, plot.y + plot.height}, 1.0f, col(theme::accent, 80));
        latest = &m;
    }
    if (a.markerLabels && latest) {
        const float w = MeasureTextEx(f, latest->label.c_str(), 11, 1.0f).x;
        label(f, latest->label.c_str(), plot.x + plot.width - w, r.y + r.height + 1, 11, col(theme::accent, 220));
    }
    for (std::size_t i = 0; i + 1 < n; ++i)
        DrawLineEx({px(i), py(a.series.at(i))}, {px(i + 1), py(a.series.at(i + 1))}, 1.6f, a.line);
}

const char* roleName(RoleVis r) {
    switch (r) {
        case RoleVis::Grazer: return "grazer";
        case RoleVis::Omnivore: return "omnivore";
        default: return "predator";
    }
}
char roleLetter(RoleVis r) { return r == RoleVis::Grazer ? 'G' : (r == RoleVis::Omnivore ? 'O' : 'P'); }

void arrow(Vector2 a, Vector2 b, float thick, Color c) {
    DrawLineEx(a, b, thick, c);
    const float dx = b.x - a.x, dy = b.y - a.y, d = std::hypot(dx, dy);
    if (d < 1.0f) return;
    const float ux = dx / d, uy = dy / d, h = 6.0f + thick;
    const Vector2 p1{b.x - ux * h - uy * h * 0.5f, b.y - uy * h + ux * h * 0.5f};
    const Vector2 p2{b.x - ux * h + uy * h * 0.5f, b.y - uy * h - ux * h * 0.5f};
    DrawTriangle(b, p1, p2, c);
}

}  // namespace

bool Hud::overPanel(Vector2 p) const {
    for (const auto& r : panels_)
        if (CheckCollisionPointRec(p, r)) return true;
    return false;
}

std::int64_t Hud::speciesRowAt(Vector2 p) const {
    for (const auto& [r, id] : rows_)
        if (CheckCollisionPointRec(p, r)) return static_cast<std::int64_t>(id);
    return -1;
}

void Hud::draw(const Font& font, const RenderSnapshot& snap, const telemetry::TelemetryHistory& hist,
               const ViewState& vs, int fps, int sw, int sh) {
    panels_.clear();
    rows_.clear();
    if (!vs.hud) return;
    const auto& t = hist.latest();
    char buf[96];
    const float M = 16.0f;

    // ---------------- left: stats + species list ----------------
    const int nRows = static_cast<int>(std::min<std::size_t>(11, snap.species.size()));
    const Rectangle sp{M, M, 270, 166.0f + static_cast<float>(nRows) * 22.0f + 22.0f};
    panel(sp);
    panels_.push_back(sp);
    label(font, "moriarty", sp.x + 14, sp.y + 12, 20, col(theme::accent));
    std::snprintf(buf, sizeof buf, "%s   t = %.0f s", vs.paused ? "paused" : "running", snap.simTime);
    label(font, buf, sp.x + 14, sp.y + 38, 12, col(vs.paused ? theme::accentSoft : theme::textDim));

    struct Row { const char* k; char v[40]; };
    Row rows[4] = {{"population", {}}, {"species", {}}, {"diversity", {}}, {"births / kills", {}}};
    std::snprintf(rows[0].v, sizeof rows[0].v, "%zu", snap.agents.size());
    std::uint32_t est = 0;
    for (const auto& s : snap.species) est += s.established;
    std::snprintf(rows[1].v, sizeof rows[1].v, "%u", est);
    std::snprintf(rows[2].v, sizeof rows[2].v, "%.2f bits", static_cast<double>(t.shannonBits));
    std::snprintf(rows[3].v, sizeof rows[3].v, "%llu / %llu", static_cast<unsigned long long>(snap.births),
                  static_cast<unsigned long long>(snap.kills));
    for (int i = 0; i < 4; ++i) {
        const float y = sp.y + 60 + static_cast<float>(i) * 18;
        label(font, rows[i].k, sp.x + 14, y, 13, col(theme::textDim));
        rightLabel(font, rows[i].v, sp.x + sp.width - 14, y, 13, col(theme::text));
    }

    label(font, "species  (click to focus, tab to cycle)", sp.x + 14, sp.y + 140, 11, col(theme::textDim, 170));
    const std::uint32_t maxCount = snap.species.empty() ? 1 : snap.species.front().count;
    for (int i = 0; i < nRows; ++i) {
        const SpeciesVis& s = snap.species[static_cast<std::size_t>(i)];
        const Rectangle row{sp.x + 8, sp.y + 160 + static_cast<float>(i) * 22.0f, sp.width - 16, 20};
        const float frac = static_cast<float>(s.count) / static_cast<float>(maxCount);
        DrawRectangleRec({row.x, row.y, row.width * frac, row.height}, col(s.color, 38));
        if (vs.focusSpecies == static_cast<std::int64_t>(s.id)) DrawRectangleLinesEx(row, 1.5f, col(s.color, 230));
        DrawCircleV({row.x + 12, row.y + 10}, 5.0f, col(s.color));
        label(font, s.shortName.c_str(), row.x + 24, row.y + 3, 13, col(s.established ? theme::text : theme::textDim));
        std::snprintf(buf, sizeof buf, "%c", roleLetter(s.role));
        label(font, buf, row.x + row.width - 62, row.y + 3, 12, col(theme::textDim));
        std::snprintf(buf, sizeof buf, "%u", s.count);
        rightLabel(font, buf, row.x + row.width - 6, row.y + 3, 13, col(theme::text));
        rows_.push_back({row, s.id});
    }
    if (snap.species.size() > static_cast<std::size_t>(nRows)) {
        std::snprintf(buf, sizeof buf, "+ %zu more", snap.species.size() - static_cast<std::size_t>(nRows));
        label(font, buf, sp.x + 14, sp.y + sp.height - 16, 11, col(theme::textDim, 170));
    }

    // ---------------- right top: food web ----------------
    const Rectangle fw{static_cast<float>(sw) - M - 270, M, 270, 250};
    panel(fw);
    panels_.push_back(fw);
    label(font, "interactions", fw.x + 14, fw.y + 12, 13, col(theme::accent));
    label(font, "arrows point from hunter to prey", fw.x + 14, fw.y + fw.height - 18, 11, col(theme::textDim, 170));
    {
        std::vector<const SpeciesVis*> nodes;
        for (const auto& s : snap.species)
            if (s.established && nodes.size() < 8) nodes.push_back(&s);
        const Vector2 c{fw.x + fw.width * 0.5f, fw.y + 124};
        std::vector<Vector2> pos(nodes.size());
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const float a = -1.5708f + 6.28318f * static_cast<float>(i) / static_cast<float>(nodes.size());
            pos[i] = nodes.size() == 1 ? c : Vector2{c.x + std::cos(a) * 82.0f, c.y + std::sin(a) * 70.0f};
        }
        auto nodeR = [&](const SpeciesVis* s) { return 6.0f + 7.0f * std::sqrt(static_cast<float>(s->count) / static_cast<float>(maxCount)); };
        bool any = false;
        for (const auto& e : snap.edges) {
            int ia = -1, ib = -1;
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i]->id == e.pred) ia = static_cast<int>(i);
                if (nodes[i]->id == e.prey) ib = static_cast<int>(i);
            }
            if (ia < 0 || ib < 0 || ia == ib) continue;
            any = true;
            const Vector2 a = pos[static_cast<std::size_t>(ia)], b = pos[static_cast<std::size_t>(ib)];
            const float dx = b.x - a.x, dy = b.y - a.y, d = std::max(1.0f, std::hypot(dx, dy));
            const float ra = nodeR(nodes[static_cast<std::size_t>(ia)]) + 2, rb = nodeR(nodes[static_cast<std::size_t>(ib)]) + 3;
            arrow({a.x + dx / d * ra, a.y + dy / d * ra}, {b.x - dx / d * rb, b.y - dy / d * rb},
                  1.0f + std::min(3.0f, std::log2(1.0f + e.rate)), col(nodes[static_cast<std::size_t>(ia)]->color, 200));
        }
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            DrawCircleV(pos[i], nodeR(nodes[i]), col(nodes[i]->color));
            DrawCircleLinesV(pos[i], nodeR(nodes[i]), col(theme::bg, 200));
            const Vector2 d = MeasureTextEx(font, nodes[i]->shortName.c_str(), 10, 1.0f);
            label(font, nodes[i]->shortName.c_str(), pos[i].x - d.x * 0.5f, pos[i].y + nodeR(nodes[i]) + 3, 10, col(theme::textDim));
        }
        if (!any) {
            const char* msg = "no recent predation";
            const Vector2 d = MeasureTextEx(font, msg, 12, 1.0f);
            label(font, msg, c.x - d.x * 0.5f, c.y - 6, 12, col(theme::textDim, 190));
        }
    }

    // ---------------- right bottom: charts ----------------
    const float cw = 242, ch = 48, pad = 13;
    const Rectangle cp{static_cast<float>(sw) - M - cw - 2 * pad, static_cast<float>(sh) - M - 4 * ch - 5 * pad, cw + 2 * pad,
                       4 * ch + 5 * pad};
    panel(cp);
    panels_.push_back(cp);
    char v0[32], v1[32], v2[32], v3[32];
    std::snprintf(v0, sizeof v0, "%u", t.population);
    std::snprintf(v1, sizeof v1, "%.2f bits", static_cast<double>(t.shannonBits));
    std::snprintf(v2, sizeof v2, "%u", t.speciesCount);
    std::snprintf(v3, sizeof v3, "%.1f", static_cast<double>(t.meanCircuitUnits));
    chart(font, {cp.x + pad, cp.y + pad, cw, ch}, {"population", hist.population, col(theme::accent), v0, false, true}, hist.markers);
    chart(font, {cp.x + pad, cp.y + 2 * pad + ch, cw, ch}, {"diversity", hist.entropy, col(speciesColor(2)), v1, true, false}, hist.markers);
    chart(font, {cp.x + pad, cp.y + 3 * pad + 2 * ch, cw, ch}, {"species", hist.species, col(speciesColor(3)), v2, true, false}, hist.markers);
    chart(font, {cp.x + pad, cp.y + 4 * pad + 3 * ch, cw, ch},
          {"mean computational units", hist.complexity, col({154, 174, 124}), v3, false, false}, hist.markers);

    // ---------------- bottom left: inspector + hints ----------------
    std::snprintf(buf, sizeof buf, "%d fps    sim x%.1f (target x%.0f)", fps, static_cast<double>(vs.simRate), static_cast<double>(vs.speed));
    label(font, buf, M, static_cast<float>(sh) - 40, 12, col(theme::textDim));
    label(font, "space pause  |  1/2 color  |  w wakes  |  g fields  |  h hud  |  f fit  |  tab species  |  click follows  |  u subunits  |  up/down speed",
          M, static_cast<float>(sh) - 24, 11, col(theme::textDim, 150));

    if (snap.selected.valid) {
        const SelectedVis& s = snap.selected;
        const SpeciesVis* sv = snap.findSpecies(s.a.species);
        const Rectangle ip{M, static_cast<float>(sh) - 64 - 350, 270, 350};
        panel(ip);
        panels_.push_back(ip);
        if (sv) {
            DrawCircleV({ip.x + 20, ip.y + 24}, 6.0f, col(sv->color));
            label(font, sv->fullName.c_str(), ip.x + 34, ip.y + 14, 15, col(sv->color));
            std::snprintf(buf, sizeof buf, "%s  |  %s", roleName(sv->role), s.state.c_str());
            label(font, buf, ip.x + 14, ip.y + 36, 12, col(theme::textDim));
        }
        std::snprintf(buf, sizeof buf, "generation %u   age %.0f s   id %llu", s.generation, static_cast<double>(s.ageSec),
                      static_cast<unsigned long long>(s.a.id));
        label(font, buf, ip.x + 14, ip.y + 52, 11, col(theme::textDim, 190));

        const float x = ip.x + 14, w = ip.width - 28;
        float y = ip.y + 74;
        bar(font, "energy", s.a.energy, x, y, w, col(theme::accent)); y += 18;
        const Color tc = col(sv ? sv->color : theme::accent, 220);
        bar(font, "size", (s.a.r - 3.0f) / 5.5f, x, y, w, tc); y += 16;
        bar(font, "speed", s.a.speed, x, y, w, tc); y += 16;
        bar(font, "senses", s.a.sense, x, y, w, tc); y += 16;
        bar(font, "aggression", s.a.aggr, x, y, w, tc); y += 16;
        bar(font, "defense", s.a.def, x, y, w, tc); y += 16;
        bar(font, "efficiency", s.a.eta, x, y, w, tc); y += 24;
        label(font, "circuit outputs (right now)", x, y, 11, col(theme::textDim, 190)); y += 16;
        signedBar(font, "drive x", s.outputs[0], x, y, w); y += 16;
        signedBar(font, "drive y", s.outputs[1], x, y, w); y += 16;
        signedBar(font, "uptake", s.outputs[2], x, y, w); y += 16;
        signedBar(font, "resource", s.outputs[3], x, y, w); y += 16;
        signedBar(font, "defense", s.outputs[4], x, y, w);
        std::snprintf(buf, sizeof buf, "%zu computational units  |  U microscope",
                      s.units.size());
        label(font, buf, x, ip.y + ip.height - 14, 10, col(theme::accent, 210));
        std::snprintf(buf, sizeof buf, "%zu computational units  |  U microscope  |  click follows",
                      s.units.size());
        label(font, buf, x, ip.y + ip.height - 14, 10, col(theme::accent, 210));
    }
}

}  // namespace alife::render
