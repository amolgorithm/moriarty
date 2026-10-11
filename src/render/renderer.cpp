#include "alife/render/renderer.hpp"

#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "alife/render/palette.hpp"

namespace alife::render {
namespace {

Color col(Rgb c, unsigned char a = 255) { return Color{c.r, c.g, c.b, a}; }

// 64x64 white disc with a soft edge, tinted per agent at draw time
Texture2D makeDot() {
    constexpr int N = 64;
    Image img = GenImageColor(N, N, BLANK);
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f) / (N * 0.5f) - 1.0f;
            const float dy = (static_cast<float>(y) + 0.5f) / (N * 0.5f) - 1.0f;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float t = std::clamp((d - 0.55f) / 0.45f, 0.0f, 1.0f);
            const float a = 1.0f - t * t * (3.0f - 2.0f * t);  // smoothstep falloff
            ImageDrawPixel(&img, x, y, Color{255, 255, 255, static_cast<unsigned char>(a * 255.0f)});
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
}

}  // namespace

Renderer::Renderer(int width, int height, const char* title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    SetExitKey(KEY_NULL);

    dot_ = makeDot();

    const std::string fontPath = std::string(ALIFE_ASSET_DIR) + "/fonts/hud.ttf";
    if (FileExists(fontPath.c_str())) {
        font_ = LoadFontEx(fontPath.c_str(), 32, nullptr, 0);
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
        ownsFont_ = true;
    } else {
        font_ = GetFontDefault();
    }
    recreateTrail();
}

Renderer::~Renderer() {
    UnloadRenderTexture(trail_);
    UnloadTexture(dot_);
    if (ownsFont_) UnloadFont(font_);
    CloseWindow();
}

bool Renderer::shouldClose() const { return WindowShouldClose(); }
float Renderer::frameTime() const { return GetFrameTime(); }
int Renderer::fps() const { return GetFPS(); }

void Renderer::recreateTrail() {
    if (trail_.id != 0) UnloadRenderTexture(trail_);
    trail_ = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    clearTrail_ = true;
}

void Renderer::fitCamera(float ww, float wh) { camera_.fit(ww, wh, GetScreenWidth(), GetScreenHeight()); }

void Renderer::pollInput(ViewState& vs, float ww, float wh) {
    if (IsKeyPressed(KEY_SPACE)) vs.paused = !vs.paused;
    if (IsKeyPressed(KEY_ONE)) vs.color = ColorMode::Species;
    if (IsKeyPressed(KEY_TWO)) vs.color = ColorMode::Energy;
    if (IsKeyPressed(KEY_T)) vs.trails = !vs.trails;
    if (IsKeyPressed(KEY_H)) vs.hud = !vs.hud;
    if (IsKeyPressed(KEY_UP)) vs.speed = std::min(vs.speed * 1.5f, 16.0f);
    if (IsKeyPressed(KEY_DOWN)) vs.speed = std::max(vs.speed / 1.5f, 0.125f);
    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_R)) camera_.fit(ww, wh, GetScreenWidth(), GetScreenHeight());

    if (IsWindowResized()) {
        recreateTrail();
        camera_.onResize(ww, wh, GetScreenWidth(), GetScreenHeight());
    }
    camera_.update(GetFrameTime());
}

void Renderer::drawAgents(const RenderSnapshot& s, ColorMode mode, float alpha, float radiusScale) {
    const Camera2D& cam = camera_.get();
    const Vector2 tl = GetScreenToWorld2D({0, 0}, cam);
    const Vector2 br = GetScreenToWorld2D(
        {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}, cam);

    // never let dots shrink below ~1.6 px on screen
    const float baseR = std::max(3.2f, 1.6f / cam.zoom) * radiusScale;
    const unsigned char a = static_cast<unsigned char>(255.0f * alpha);

    for (std::size_t i = 0; i < s.size(); ++i) {
        const float x = s.x[i], y = s.y[i];
        if (x < tl.x - baseR * 2 || x > br.x + baseR * 2 || y < tl.y - baseR * 2 || y > br.y + baseR * 2)
            continue;

        const Rgb c = mode == ColorMode::Species ? speciesColor(s.species[i]) : energyColor(s.energy[i]);
        const float r = baseR * (0.8f + 0.5f * s.energy[i]);

        rlCheckRenderBatchLimit(4);
        rlSetTexture(dot_.id);
        rlBegin(RL_QUADS);
        rlColor4ub(c.r, c.g, c.b, a);
        rlTexCoord2f(0, 0); rlVertex2f(x - r, y - r);
        rlTexCoord2f(0, 1); rlVertex2f(x - r, y + r);
        rlTexCoord2f(1, 1); rlVertex2f(x + r, y + r);
        rlTexCoord2f(1, 0); rlVertex2f(x + r, y - r);
        rlEnd();
    }
    rlSetTexture(0);
}

void Renderer::drawWorldFrame(const RenderSnapshot& snap) {
    const Camera2D& cam = camera_.get();
    const float px = 1.0f / cam.zoom;

    // grid every 128 world units, hidden when it would get too dense
    constexpr float kStep = 128.0f;
    if (kStep * cam.zoom > 20.0f) {
        const Color g = col(theme::grid, 9);
        for (float x = kStep; x < snap.worldW; x += kStep) DrawLineEx({x, 0}, {x, snap.worldH}, px, g);
        for (float y = kStep; y < snap.worldH; y += kStep) DrawLineEx({0, y}, {snap.worldW, y}, px, g);
    }
    DrawRectangleLinesEx({0, 0, snap.worldW, snap.worldH}, 1.5f * px, col(theme::textDim, 90));
}

void Renderer::draw(const RenderSnapshot& snap, const telemetry::TelemetryHistory& hist,
                    const ViewState& vs) {
    const int sw = GetScreenWidth(), sh = GetScreenHeight();
    const Color bg = col(theme::bg);

    // trail layer: fade the previous frame, stamp this frame's agents small and dim
    if (vs.trails) {
        const bool reset = clearTrail_ || camera_.movedThisFrame() || !prevTrails_;
        if (reset || !vs.paused) {
            BeginTextureMode(trail_);
            if (reset) ClearBackground(bg);
            else DrawRectangle(0, 0, sw, sh, Fade(bg, 0.07f));
            BeginMode2D(camera_.get());
            drawAgents(snap, vs.color, 0.45f, 0.7f);
            EndMode2D();
            EndTextureMode();
            clearTrail_ = false;
        }
    }
    prevTrails_ = vs.trails;

    BeginDrawing();
    ClearBackground(bg);
    if (vs.trails) {
        DrawTextureRec(trail_.texture,
                       {0, 0, static_cast<float>(trail_.texture.width), -static_cast<float>(trail_.texture.height)},
                       {0, 0}, WHITE);
    }
    BeginMode2D(camera_.get());
    drawWorldFrame(snap);
    drawAgents(snap, vs.color, 0.95f, 1.0f);
    EndMode2D();
    hud_.draw(font_, hist, vs, GetFPS(), sw, sh);
    EndDrawing();
}

}  // namespace alife::render
