#include "alife/render/renderer.hpp"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <string>

#include "alife/core/rng.hpp"

namespace alife::render {
namespace {

constexpr float kTau = 6.28318530718f;
constexpr Rgb kInstructionColors[] = {
    {114, 177, 155}, {125, 164, 216}, {199, 148, 218}, {224, 151, 111},
    {216, 190, 111}, {107, 185, 194}, {213, 132, 153}, {154, 174, 124},
    {181, 151, 211}, {208, 126, 165}, {110, 173, 190}, {224, 177, 100},
    {187, 148, 218}, {116, 190, 142}, {202, 143, 187}, {120, 164, 203},
    {218, 132, 93}, {234, 206, 144}};
static_assert(std::size(kInstructionColors) == genetics::kVmOpCount);

Color col(Rgb c, unsigned char a = 255) { return Color{c.r, c.g, c.b, a}; }
Rgb scale(Rgb c, float k) {
    auto q = [k](std::uint8_t v) { return static_cast<std::uint8_t>(std::clamp(static_cast<float>(v) * k, 0.0f, 255.0f)); };
    return {q(c.r), q(c.g), q(c.b)};
}
float hash01(std::uint64_t id, std::uint64_t k) {
    return static_cast<float>(mix64(id * 0x9E3779B97F4A7C15ull + k) >> 40) * (1.0f / 16777216.0f);
}

// 64x64 white disc with a soft edge, tinted per draw
Texture2D makeDot() {
    constexpr int N = 64;
    Image img = GenImageColor(N, N, BLANK);
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f) / (N * 0.5f) - 1.0f;
            const float dy = (static_cast<float>(y) + 0.5f) / (N * 0.5f) - 1.0f;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float t = std::clamp((d - 0.55f) / 0.45f, 0.0f, 1.0f);
            const float a = 1.0f - t * t * (3.0f - 2.0f * t);
            ImageDrawPixel(&img, x, y, Color{255, 255, 255, static_cast<unsigned char>(a * 255.0f)});
        }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

// rotated ellipse as a triangle fan
void fillEllipse(float cx, float cy, float a, float b, float cs, float sn, int n, Color c) {
    Vector2 prev{cx + a * cs, cy + a * sn};
    for (int k = 1; k <= n; ++k) {
        const float ang = kTau * static_cast<float>(k) / static_cast<float>(n);
        const float ex = a * std::cos(ang), ey = b * std::sin(ang);
        const Vector2 cur{cx + ex * cs - ey * sn, cy + ex * sn + ey * cs};
        DrawTriangle({cx, cy}, prev, cur, c);
        prev = cur;
    }
}

// local body coordinates (u forward, v left) to world
Vector2 toWorld(float cx, float cy, float cs, float sn, float u, float v) {
    return {cx + u * cs - v * sn, cy + u * sn + v * cs};
}

}  // namespace

Renderer::Renderer(int width, int height, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    SetExitKey(KEY_NULL);
    rlDisableBackfaceCulling();

    dot_ = makeDot();
    const std::string fontPath = std::string(ALIFE_ASSET_DIR) + "/fonts/hud.ttf";
    if (FileExists(fontPath.c_str())) {
        font_ = LoadFontEx(fontPath.c_str(), 32, nullptr, 0);
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
        ownsFont_ = true;
    } else {
        font_ = GetFontDefault();
    }
}

Renderer::~Renderer() {
    if (fieldTex_.id) UnloadTexture(fieldTex_);
    UnloadTexture(dot_);
    if (ownsFont_) UnloadFont(font_);
    CloseWindow();
}

bool Renderer::shouldClose() const { return WindowShouldClose(); }
float Renderer::frameTime() const { return GetFrameTime(); }
int Renderer::fps() const { return GetFPS(); }

void Renderer::fitCamera(float ww, float wh) { camera_.fit(ww, wh, GetScreenWidth(), GetScreenHeight()); }
void Renderer::setZoomForShot(float x, float y, float zoom) {
    camera_.flyTo(x, y, zoom, GetScreenWidth(), GetScreenHeight());
}
void Renderer::screenshot(const char* path) { TakeScreenshot(path); }

void Renderer::focusSpecies(ViewState& vs, const RenderSnapshot& snap, std::int64_t id) {
    const SpeciesVis* s = id >= 0 ? snap.findSpecies(static_cast<std::uint32_t>(id)) : nullptr;
    if (!s) return;
    vs.focusSpecies = id;
    vs.follow = false;
    camera_.setFollow(false);
    camera_.flyTo(s->labelX, s->labelY, 2.6f, GetScreenWidth(), GetScreenHeight());
}

int Renderer::pickAgent(const RenderSnapshot& snap, Vector2 screen, float slackPx) const {
    const Camera2D& cam = camera_.get();
    const Vector2 m = GetScreenToWorld2D(screen, cam);
    int best = -1;
    float bestD = 1e18f;
    for (std::size_t i = 0; i < snap.agents.size(); ++i) {
        const AgentVis& a = snap.agents[i];
        const float dx = a.x - m.x, dy = a.y - m.y, d2 = dx * dx + dy * dy;
        const float reach = std::max(a.r * 1.3f, slackPx / cam.zoom);
        if (d2 < reach * reach && d2 < bestD) { bestD = d2; best = static_cast<int>(i); }
    }
    return best;
}

void Renderer::pollInput(ViewState& vs, const RenderSnapshot& snap) {
    const float dt = GetFrameTime();
    clock_ += dt;
    const int sw = GetScreenWidth(), sh = GetScreenHeight();
    const Vector2 mouse = GetMousePosition();
    const float wheel = GetMouseWheelMove();
    const bool scrollingSpecies =
        vs.hud && hud_.scrollSpecies(wheel, mouse, sh, snap.species.size(), snap.selected.valid);
    const float inspectorWidth = std::min(static_cast<float>(sw) * 0.48f, 720.0f);
    const float inspectorHeight = std::min(static_cast<float>(sh) - 24.0f, 760.0f);
    const Rectangle inspectorPanel{static_cast<float>(sw) - inspectorWidth - 12.0f,
                                   (static_cast<float>(sh) - inspectorHeight) * 0.5f,
                                   inspectorWidth, inspectorHeight};
    const bool overInspector = vs.unitView && snap.selected.valid &&
                               CheckCollisionPointRec(mouse, inspectorPanel);
    bool scrollingUnits = false;
    if (!scrollingSpecies && overInspector && wheel != 0.0f) {
        constexpr std::size_t kColumns = 4;
        constexpr float kCellHeight = 34.0f;
        const std::size_t visibleRows = std::max<std::size_t>(
            1, static_cast<std::size_t>(std::max(1.0f, (inspectorHeight - 116.0f) / kCellHeight)));
        const std::size_t totalRows = (snap.selected.units.size() + kColumns - 1) / kColumns;
        const std::size_t maxScroll = totalRows > visibleRows ? totalRows - visibleRows : 0;
        const int delta = static_cast<int>(wheel * 3.0f);
        if (delta != 0) {
            const auto next = static_cast<long long>(unitScrollRows_) - delta;
            unitScrollRows_ = static_cast<std::size_t>(
                std::clamp(next, 0LL, static_cast<long long>(maxScroll)));
            scrollingUnits = true;
        }
    }

    if (IsKeyPressed(KEY_SPACE)) vs.paused = !vs.paused;
    if (IsKeyPressed(KEY_ONE)) vs.color = ColorMode::Species;
    if (IsKeyPressed(KEY_TWO)) vs.color = ColorMode::Energy;
    if (IsKeyPressed(KEY_W)) vs.wakes = !vs.wakes;
    if (IsKeyPressed(KEY_G)) vs.fields = !vs.fields;
    if (IsKeyPressed(KEY_H)) vs.hud = !vs.hud;
    if (IsKeyPressed(KEY_UP)) vs.speed = std::min(vs.speed * 1.5f, 96.0f);
    if (IsKeyPressed(KEY_DOWN)) vs.speed = std::max(vs.speed / 1.5f, 1.0f);
    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_R)) {
        camera_.fit(snap.worldW, snap.worldH, sw, sh);
        vs.follow = false;
        vs.focusSpecies = -1;
        vs.unitView = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        vs.selected = 0;
        vs.focusSpecies = -1;
        vs.follow = false;
        camera_.setFollow(false);
        vs.unitView = false;
        unitScrollRows_ = 0;
    }
    if (IsKeyPressed(KEY_TAB) && !snap.species.empty()) {
        std::vector<std::uint32_t> ids;
        for (const auto& s : snap.species)
            if (s.established) ids.push_back(s.id);
        if (!ids.empty()) {
            const int dir = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? -1 : 1;
            int cur = -1;
            for (std::size_t i = 0; i < ids.size(); ++i)
                if (static_cast<std::int64_t>(ids[i]) == vs.focusSpecies) cur = static_cast<int>(i);
            const int n = static_cast<int>(ids.size());
            const int next = cur < 0 ? (dir > 0 ? 0 : n - 1) : ((cur + dir) % n + n) % n;
            focusSpecies(vs, snap, ids[static_cast<std::size_t>(next)]);
        }
    }
    if (IsKeyPressed(KEY_ENTER) && vs.selected != 0 && snap.selected.valid) {
        vs.follow = !vs.follow;
        camera_.setFollow(vs.follow);
        if (vs.follow) camera_.flyTo(snap.selected.a.x, snap.selected.a.y, 7.0f, sw, sh);
    }
    if (IsKeyPressed(KEY_U) && vs.selected != 0 && snap.selected.valid) {
        vs.unitView = !vs.unitView;
        unitScrollRows_ = 0;
        vs.follow = true;
        camera_.setFollow(true);
        camera_.flyTo(snap.selected.a.x, snap.selected.a.y, vs.unitView ? 28.0f : 7.0f, sw, sh,
                      vs.unitView ? static_cast<float>(sw) * 0.14f : 0.0f);
    }

    // click = press and release without much movement
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) pressPos_ = mouse;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
        std::hypot(mouse.x - pressPos_.x, mouse.y - pressPos_.y) < 5.0f) {
        const std::int64_t row = hud_.speciesRowAt(mouse);
        if (row >= 0) {
            focusSpecies(vs, snap, row);
        } else if (!hud_.overPanel(mouse) &&
                   !(vs.unitView && CheckCollisionPointRec(mouse, inspectorPanel))) {
            const int i = pickAgent(snap, mouse, 10.0f);
            vs.selected = i >= 0 ? snap.agents[static_cast<std::size_t>(i)].id : 0;
            unitScrollRows_ = 0;
            vs.unitView = false;
            vs.follow = i >= 0;
            camera_.setFollow(vs.follow);
            if (i >= 0) {
                const AgentVis& a = snap.agents[static_cast<std::size_t>(i)];
                camera_.flyTo(a.x, a.y, 7.0f, sw, sh);
            }
        }
    }

    if (IsWindowResized()) camera_.onResize(snap.worldW, snap.worldH, sw, sh);
    camera_.update(dt, !scrollingSpecies && !scrollingUnits);
    if (camera_.interrupted()) vs.follow = false;

    // the selected organism died
    if (vs.selected != 0 && snap.selectedRequested == vs.selected && !snap.selected.valid) {
        vs.selected = 0;
        vs.follow = false;
        vs.unitView = false;
        unitScrollRows_ = 0;
        camera_.setFollow(false);
    }
    if (vs.follow && snap.selected.valid) camera_.setGoal(snap.selected.a.x, snap.selected.a.y);

    hover_ = hud_.overPanel(mouse) ? -1 : pickAgent(snap, mouse, 8.0f);
}

void Renderer::sprite(float x, float y, float r, Rgb c, unsigned char a) {
    rlCheckRenderBatchLimit(4);
    rlSetTexture(dot_.id);
    rlBegin(RL_QUADS);
    rlColor4ub(c.r, c.g, c.b, a);
    rlTexCoord2f(0, 0); rlVertex2f(x - r, y - r);
    rlTexCoord2f(0, 1); rlVertex2f(x - r, y + r);
    rlTexCoord2f(1, 1); rlVertex2f(x + r, y + r);
    rlTexCoord2f(1, 0); rlVertex2f(x + r, y - r);
    rlEnd();
    rlSetTexture(0);
}

void Renderer::spawnEffects(const RenderSnapshot& snap) {
    if (snap.tick == lastTick_) return;  // draw() called twice for the same snapshot (paused)
    lastTick_ = snap.tick;
    for (const auto& e : snap.events) {
        if (effects_.size() > 1400 || e.type == EffectVis::Death) continue;
        const Rgb c = e.sp < colorOf_.size() ? colorOf_[e.sp] : Rgb{200, 200, 200};
        effects_.push_back({e.x, e.y, e.x2, e.y2, 0.0f, e.type == EffectVis::Kill ? 0.55f : (e.type == EffectVis::Birth ? 0.7f : 0.9f), c,
                            static_cast<std::uint8_t>(e.type)});
    }
}

void Renderer::updateFieldTexture(const RenderSnapshot& snap) {
    if (snap.fieldW == 0) return;
    const std::size_t n = static_cast<std::size_t>(snap.fieldW) * static_cast<std::size_t>(snap.fieldH);
    if (fieldPix_.size() != n) {
        fieldPix_.assign(n, BLACK);
        if (fieldTex_.id) UnloadTexture(fieldTex_);
        Image img{fieldPix_.data(), snap.fieldW, snap.fieldH, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        fieldTex_ = LoadTextureFromImage(img);
        SetTextureFilter(fieldTex_, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(fieldTex_, TEXTURE_WRAP_CLAMP);
    }
    // ground + sage glow (fine food) + amber glow (coarse food) + rust tint (waste heat)
    for (std::size_t i = 0; i < n; ++i) {
        const float f0 = snap.food0[i], f1 = snap.food1[i], h = snap.heat[i];
        const float r = 33.0f + 62.0f * f0 + 120.0f * f1 + 150.0f * h;
        const float g = 28.0f + 92.0f * f0 + 78.0f * f1 + 40.0f * h;
        const float b = 24.0f + 58.0f * f0 + 30.0f * f1 + 30.0f * h;
        fieldPix_[i] = Color{static_cast<unsigned char>(std::min(255.0f, r * 0.62f + 8.0f)),
                             static_cast<unsigned char>(std::min(255.0f, g * 0.62f + 6.0f)),
                             static_cast<unsigned char>(std::min(255.0f, b * 0.62f + 5.0f)), 255};
    }
    UpdateTexture(fieldTex_, fieldPix_.data());
}

void Renderer::drawWorld(const RenderSnapshot& snap, const ViewState& vs) {
    const Camera2D& cam = camera_.get();
    const float px = 1.0f / cam.zoom;
    if (vs.fields && fieldTex_.id) {
        DrawTexturePro(fieldTex_, {0, 0, static_cast<float>(fieldTex_.width), static_cast<float>(fieldTex_.height)},
                       {0, 0, snap.worldW, snap.worldH}, {0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(0, 0, static_cast<int>(snap.worldW), static_cast<int>(snap.worldH), col({36, 31, 27}));
    }
    DrawRectangleLinesEx({0, 0, snap.worldW, snap.worldH}, 1.5f * px, col(theme::textDim, 90));

    // marine snow: tracers drifting with the stochastic current
    const float r = std::max(0.9f, 1.1f * px);
    for (std::size_t i = 0; i < snap.tracerX.size(); ++i) sprite(snap.tracerX[i], snap.tracerY[i], r, theme::text, 46);
}

void Renderer::drawBody(const AgentVis& a, Rgb c, float alpha, int lod) {
    const float cs = std::cos(a.heading), sn = std::sin(a.heading);
    const float r = a.r;
    const float A = 1.0f + std::min(0.65f, 0.12f * std::log2(static_cast<float>(a.programLength) + 1.0f));
    const float ea = r * std::sqrt(A), eb = r / std::sqrt(A);
    const unsigned char al = static_cast<unsigned char>(255.0f * alpha);
    const float zoom = camera_.zoom();

    const Rgb outline = lerp(c, Rgb{250, 240, 225}, 0.18f);
    const Rgb fill = lerp(scale(c, 0.28f), scale(c, 0.92f), 0.2f + 0.8f * a.energy);
    const float pulse = lod >= 3 ? 1.0f + 0.03f * std::sin(clock_ * 3.0f + static_cast<float>(a.id % 97)) : 1.0f;
    const int seg = lod >= 3 ? 28 : 12;
    fillEllipse(a.x, a.y, ea * pulse, eb * pulse, cs, sn, seg, col(outline, al));
    fillEllipse(a.x, a.y, ea * pulse * 0.8f, eb * pulse * 0.8f, cs, sn, seg, col(fill, al));
    if (lod < 3) return;

    const float unitRadius = std::max(r * (0.19f / std::sqrt(std::max(1.0f, static_cast<float>(a.programLength)))),
                                     0.8f / zoom);
    std::uint64_t ordinal = 0;
    for (std::size_t op = 0; op < a.opCounts.size(); ++op) {
        for (std::uint16_t unit = 0; unit < a.opCounts[op]; ++unit, ++ordinal) {
            float u = 2.0f * hash01(a.id, ordinal * 2 + 41) - 1.0f;
            float v = 2.0f * hash01(a.id, ordinal * 2 + 42) - 1.0f;
            const float d2 = u * u + v * v;
            if (d2 > 0.85f) { u *= 0.72f; v *= 0.72f; }
            const Vector2 point = toWorld(a.x, a.y, cs, sn, ea * u * 0.65f, eb * v * 0.65f);
            const float active = 0.45f + 0.55f *
                static_cast<float>(a.executedInstructions) / std::max(1.0f, static_cast<float>(a.programLength));
            sprite(point.x, point.y, unitRadius, kInstructionColors[op],
                   static_cast<unsigned char>(180.0f * alpha * active));
        }
    }
}

void Renderer::drawAgents(const RenderSnapshot& snap, const ViewState& vs) {
    const Camera2D& cam = camera_.get();
    const float z = cam.zoom;
    const Vector2 tl = GetScreenToWorld2D({0, 0}, cam);
    const Vector2 br = GetScreenToWorld2D({static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}, cam);
    const bool focus = vs.focusSpecies >= 0;

    for (const AgentVis& a : snap.agents) {
        const float m = a.r * 2.5f + 12.0f;
        if (a.x < tl.x - m || a.x > br.x + m || a.y < tl.y - m || a.y > br.y + m) continue;

        const float dim = (focus && static_cast<std::int64_t>(a.species) != vs.focusSpecies) ? 0.22f : 1.0f;
        Rgb c = vs.color == ColorMode::Species ? (a.species < colorOf_.size() ? colorOf_[a.species] : Rgb{200, 200, 200})
                                               : energyColor(a.energy);
        const float rpx = a.r * z;
        if (vs.wakes && z > 0.55f && rpx > 1.5f) {
            // comet-style wake: a faint triangle trailing the body, longer when faster
            const float sp = std::hypot(a.vx, a.vy);
            if (sp > 4.0f) {
                const float ux = a.vx / sp, uy = a.vy / sp, len = std::min(sp * 0.9f, 4.0f * a.r + 10.0f), w = a.r * 0.7f;
                const Vector2 l{a.x - uy * w, a.y + ux * w}, r2{a.x + uy * w, a.y - ux * w};
                DrawTriangle(l, r2, {a.x - ux * len, a.y - uy * len}, col(c, static_cast<unsigned char>(34.0f * dim)));
            }
        }
        if (rpx < 2.2f) {
            sprite(a.x, a.y, std::max(a.r * 0.95f, 1.8f / z), c, static_cast<unsigned char>(235.0f * dim));
        } else {
            drawBody(a, c, dim, rpx < 7.0f ? 2 : 3);
        }
    }
}

void Renderer::drawSelection(const RenderSnapshot& snap) {
    if (!snap.selected.valid) return;
    const SelectedVis& s = snap.selected;
    const float z = camera_.zoom();
    const float px = 1.0f / z;
    const float pulse = 1.0f + 0.12f * std::sin(clock_ * 4.0f);
    const float senseR = 14.0f + 34.0f * s.a.sense;

    DrawRing({s.a.x, s.a.y}, senseR - px * 0.6f, senseR, 0, 360, 72, col(theme::accent, 70));
    DrawRing({s.a.x, s.a.y}, (s.a.r + 4.0f) * pulse - 1.6f * px, (s.a.r + 4.0f) * pulse, 0, 360, 40, col(theme::accent, 230));
    if (s.hasPrey) DrawLineEx({s.a.x, s.a.y}, {s.preyX, s.preyY}, 1.6f * px, col({224, 112, 84}, 190));
    if (s.hasThreat) DrawLineEx({s.a.x, s.a.y}, {s.threatX, s.threatY}, 1.6f * px, col({236, 196, 92}, 190));
}

void Renderer::drawProgramInspector(const RenderSnapshot& snap) {
    if (!snap.selected.valid || snap.selected.units.empty()) return;
    const auto& units = snap.selected.units;
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float width = std::min(sw * 0.48f, 720.0f);
    const float height = std::min(sh - 24.0f, 760.0f);
    const Rectangle panel{sw - width - 12.0f, (sh - height) * 0.5f, width, height};
    DrawRectangleRounded(panel, 0.025f, 8, col(theme::panel, 242));
    DrawRectangleRoundedLinesEx(panel, 0.025f, 8, 1.5f, col(theme::accent, 210));

    char title[160];
    std::snprintf(title, sizeof title, "VIRTUAL GENOME  |  %zu instructions  |  organism #%llu",
                  units.size(), static_cast<unsigned long long>(snap.selected.a.id));
    DrawTextEx(font_, title, {panel.x + 16.0f, panel.y + 12.0f}, 18.0f, 1.0f, col(theme::accent));
    DrawTextEx(font_, "U close  |  S00-01 food  S02-03 prey  S04-05 threat  S06-07 kin  S08 energy",
               {panel.x + 16.0f, panel.y + 36.0f}, 9.0f, 1.0f, col(theme::textDim));
    DrawTextEx(font_, "S09 mix  S10-11 current  S12 group  S13 alignment  S14 food  S15 age",
               {panel.x + 16.0f, panel.y + 48.0f}, 9.0f, 1.0f, col(theme::textDim));
    DrawTextEx(font_, "S16 heat  S17 prey value  S18 threat urgency  S19 previous energy reward",
               {panel.x + 16.0f, panel.y + 60.0f}, 9.0f, 1.0f, col(theme::textDim));
    DrawTextEx(font_, "d: destination  a/b: source registers  #: immediate/branch offset  v: register value",
               {panel.x + 16.0f, panel.y + 72.0f}, 9.0f, 1.0f, col(theme::textDim));

    static constexpr const char* kOps[] = {
        "nop", "sense", "load", "move", "add", "sub", "mul", "min", "max", "abs",
        "tanh", "memory", "learn", "gt", "lt", "jpos", "jneg", "output"};
    static_assert(std::size(kOps) == genetics::kVmOpCount);
    static constexpr const char* kPorts[] = {
        "x-force", "y-force", "uptake", "resource", "defense", "cohesion", "hunt", "reproduction"};
    static constexpr Rgb kUnitColors[] = {
        {114, 177, 155}, {125, 164, 216}, {199, 148, 218}, {224, 151, 111},
        {216, 190, 111}, {107, 185, 194}, {213, 132, 153}, {154, 174, 124},
        {181, 151, 211}, {208, 126, 165}, {110, 173, 190}};
    constexpr std::size_t kColumns = 4;
    constexpr float kCellHeight = 34.0f;
    const float left = panel.x + 12.0f, top = panel.y + 104.0f;
    const float cellW = (width - 24.0f) / static_cast<float>(kColumns);
    const std::size_t visibleRows = std::max<std::size_t>(
        1, static_cast<std::size_t>(std::max(1.0f, (height - 116.0f) / kCellHeight)));
    const std::size_t totalRows = (units.size() + kColumns - 1) / kColumns;
    const std::size_t maxScroll = totalRows > visibleRows ? totalRows - visibleRows : 0;
    unitScrollRows_ = std::min(unitScrollRows_, maxScroll);
    const std::size_t firstUnit = unitScrollRows_ * kColumns;
    const std::size_t lastUnit = std::min(units.size(), firstUnit + visibleRows * kColumns);
    char range[96];
    std::snprintf(range, sizeof range, "Wheel: scroll units  |  showing %zu-%zu of %zu",
                  firstUnit + 1, lastUnit, units.size());
    DrawTextEx(font_, range, {panel.x + 16.0f, panel.y + 86.0f}, 9.0f, 1.0f, col(theme::accent));
    auto position = [&](std::size_t i) {
        const std::size_t visibleIndex = i - firstUnit;
        const std::size_t row = visibleIndex / kColumns, column = visibleIndex % kColumns;
        return Rectangle{left + static_cast<float>(column) * cellW + 3.0f,
                         top + static_cast<float>(row) * kCellHeight + 2.0f,
                         cellW - 6.0f, kCellHeight - 4.0f};
    };
    for (std::size_t i = firstUnit; i < lastUnit; ++i) {
        const auto& unit = units[i];
        const Rectangle r = position(i);
        const auto op = std::min<std::size_t>(unit.op, std::size(kOps) - 1);
        DrawRectangleRounded(r, 0.12f, 5, col(kInstructionColors[op], 65));
        DrawRectangleRoundedLinesEx(r, 0.12f, 5, 1.0f, col(kInstructionColors[op], 180));
        char line[48];
        std::snprintf(line, sizeof line, "I%03zu  %s", i, kOps[op]);
        DrawTextEx(font_, line, {r.x + 6.0f, r.y + 2.0f}, 9.0f, 1.0f, col(theme::text));
        if (unit.op == static_cast<std::uint8_t>(genetics::VmOp::Output)) {
            const Vector2 outSize = MeasureTextEx(font_, kPorts[unit.dst % genetics::kVmOutputs], 8.0f, 1.0f);
            DrawTextEx(font_, kPorts[unit.dst % genetics::kVmOutputs],
                       {r.x + r.width - outSize.x - 4.0f, r.y + 2.0f}, 8.0f, 1.0f, col(theme::accent));
        }
        if (unit.op == static_cast<std::uint8_t>(genetics::VmOp::Sense)) {
            std::snprintf(line, sizeof line, "d%02u <- S%02u", static_cast<unsigned>(unit.dst),
                          static_cast<unsigned>(unit.sourceA));
        } else {
            std::snprintf(line, sizeof line, "d%02u a%02u b%02u #%d", static_cast<unsigned>(unit.dst),
                          static_cast<unsigned>(unit.sourceA), static_cast<unsigned>(unit.sourceB),
                          static_cast<int>(unit.immediate));
        }
        DrawTextEx(font_, line, {r.x + 6.0f, r.y + 12.0f}, 8.0f, 1.0f, col(theme::textDim));
        std::snprintf(line, sizeof line, "register %.2f", static_cast<double>(unit.value));
        DrawTextEx(font_, line, {r.x + 6.0f, r.y + 21.0f}, 8.0f, 1.0f, col(theme::textDim));
    }
}

void Renderer::drawEffects() {
    const float dt = GetFrameTime();
    const float z = camera_.zoom();
    const float px = 1.0f / z;
    for (auto& e : effects_) {
        e.age += dt;
        if (z < 0.4f) continue;
        const float t = std::clamp(e.age / e.life, 0.0f, 1.0f);
        const unsigned char a = static_cast<unsigned char>(255.0f * (1.0f - t));
        if (e.type == EffectVis::Kill) {
            DrawLineEx({e.x, e.y}, {e.x2, e.y2}, 2.0f * px, col({250, 230, 200}, static_cast<unsigned char>(a * 0.8f)));
            const float r = 3.0f + 8.0f * t;
            DrawRing({e.x2, e.y2}, r - 1.6f * px, r, 0, 360, 24, col({226, 96, 70}, static_cast<unsigned char>(a * 0.8f)));
        } else if (e.type == EffectVis::Birth) {
            const float r = 2.5f + 6.0f * t;
            DrawRing({e.x, e.y}, r - 1.2f * px, r, 0, 360, 20, col(e.c, static_cast<unsigned char>(a * 0.6f)));
        }
    }
    effects_.erase(std::remove_if(effects_.begin(), effects_.end(), [](const Effect& e) { return e.age >= e.life; }), effects_.end());
}

void Renderer::drawLabels(const RenderSnapshot& snap, const ViewState& vs) {
    const Camera2D& cam = camera_.get();
    const float fade = std::clamp((1.35f - cam.zoom) / 0.5f, 0.0f, 1.0f);
    std::vector<Rectangle> placed;
    const float W = static_cast<float>(GetScreenWidth()), H = static_cast<float>(GetScreenHeight());
    for (const auto& s : snap.species) {
        if (!s.established || s.count < 10) continue;
        const bool focused = vs.focusSpecies == static_cast<std::int64_t>(s.id);
        float alpha = focused ? 1.0f : fade;
        if (vs.focusSpecies >= 0 && !focused) alpha *= 0.35f;
        if (alpha <= 0.02f) continue;
        const Vector2 p = GetWorldToScreen2D({s.labelX, s.labelY}, cam);
        const float size = focused ? 17.0f : 14.0f;
        const Vector2 d = MeasureTextEx(font_, s.fullName.c_str(), size, 1.0f);
        Rectangle r{p.x - d.x * 0.5f, p.y - 34.0f, d.x, d.y + 14.0f};
        if (r.x < 0 || r.y < 0 || r.x + r.width > W || r.y + r.height > H) continue;
        bool clash = false;
        for (const auto& q : placed) clash = clash || CheckCollisionRecs(r, q);
        if (clash && !focused) continue;
        placed.push_back(r);
        const unsigned char a = static_cast<unsigned char>(255.0f * alpha);
        DrawRectangleRounded({r.x - 6, r.y - 2, r.width + 12, d.y + 15}, 0.3f, 6, col({20, 17, 14}, static_cast<unsigned char>(150.0f * alpha)));
        DrawTextEx(font_, s.fullName.c_str(), {r.x + 1, r.y + 1}, size, 1.0f, col({10, 8, 6}, static_cast<unsigned char>(a * 0.7f)));
        DrawTextEx(font_, s.fullName.c_str(), {r.x, r.y}, size, 1.0f, col(lerp(s.color, Rgb{250, 240, 225}, 0.25f), a));
        char buf[40];
        std::snprintf(buf, sizeof buf, "%u", s.count);
        const Vector2 cd = MeasureTextEx(font_, buf, 11, 1.0f);
        DrawTextEx(font_, buf, {p.x - cd.x * 0.5f, r.y + d.y + 1}, 11, 1.0f, col(theme::textDim, a));
        DrawLineEx({p.x, r.y + d.y + 14}, {p.x, p.y - 4}, 1.0f, col(s.color, static_cast<unsigned char>(a * 0.5f)));
    }
}

void Renderer::drawTooltip(const RenderSnapshot& snap, const ViewState&) {
    // hover_ was picked against last frame's snapshot, and the population may have shrunk
    // since (deaths, swap-remove). Re-pick against the snapshot we are about to index.
    hover_ = hud_.overPanel(GetMousePosition()) ? -1 : pickAgent(snap, GetMousePosition(), 8.0f);
    if (hover_ < 0 || static_cast<std::size_t>(hover_) >= snap.agents.size() || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return;
    const AgentVis& a = snap.agents[static_cast<std::size_t>(hover_)];
    const SpeciesVis* s = snap.findSpecies(a.species);
    if (!s) return;
    char l1[64], l2[64];
    std::snprintf(l1, sizeof l1, "%s", s->fullName.c_str());
    const char* role = s->role == RoleVis::Grazer ? "grazer" : (s->role == RoleVis::Omnivore ? "omnivore" : "predator");
    std::snprintf(l2, sizeof l2, "%s, energy %.0f%%", role, static_cast<double>(a.energy * 100.0f));
    const Vector2 m = GetMousePosition();
    const float w = std::max(MeasureTextEx(font_, l1, 13, 1.0f).x, MeasureTextEx(font_, l2, 11, 1.0f).x) + 20;
    const Rectangle r{std::min(m.x + 16, static_cast<float>(GetScreenWidth()) - w - 8), m.y + 14, w, 42};
    DrawRectangleRounded(r, 0.2f, 6, col(theme::panel, 235));
    DrawRectangleRoundedLinesEx(r, 0.2f, 6, 1.0f, col(s->color, 160));
    DrawTextEx(font_, l1, {r.x + 10, r.y + 6}, 13, 1.0f, col(s->color));
    DrawTextEx(font_, l2, {r.x + 10, r.y + 24}, 11, 1.0f, col(theme::textDim));
}

void Renderer::draw(const RenderSnapshot& snap, const telemetry::TelemetryHistory& hist, const ViewState& vs) {
    // species id -> color lookup for this frame
    std::uint32_t maxId = 0;
    for (const auto& s : snap.species) maxId = std::max(maxId, s.id);
    colorOf_.assign(maxId + 1, Rgb{200, 200, 200});
    for (const auto& s : snap.species) colorOf_[s.id] = s.color;

    spawnEffects(snap);
    updateFieldTexture(snap);

    BeginDrawing();
    ClearBackground(col(theme::bg));
    BeginMode2D(camera_.get());
    drawWorld(snap, vs);
    drawAgents(snap, vs);
    drawSelection(snap);
    drawEffects();
    EndMode2D();
    drawLabels(snap, vs);
    drawTooltip(snap, vs);
    hud_.draw(font_, snap, hist, vs, GetFPS(), GetScreenWidth(), GetScreenHeight());
    if (vs.unitView) drawProgramInspector(snap);
    EndDrawing();
}

}  // namespace alife::render
