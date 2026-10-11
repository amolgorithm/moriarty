#pragma once
#include <raylib.h>

#include <cstdint>
#include <vector>

#include "alife/render/camera.hpp"
#include "alife/render/hud.hpp"
#include "alife/render/palette.hpp"
#include "alife/render/snapshot.hpp"
#include "alife/telemetry/metrics.hpp"

namespace alife::render {

// Owns the window, camera and HUD. The sim never touches raylib, it only hands over
// a RenderSnapshot and a TelemetryHistory.
//
// Level of detail by on-screen body size:
//   < 2.2 px   soft dot in the species color, species names float over the clusters
//   2.2 - 7 px simplified body: outline, fill, eye, tail
//   > 7 px     full organism: flagella, cilia, spines, fangs, organelles, pigment spots
class Renderer {
   public:
    Renderer(int width, int height, const char* title);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool shouldClose() const;
    float frameTime() const;
    int fps() const;

    void fitCamera(float worldW, float worldH);
    void pollInput(ViewState& vs, const RenderSnapshot& snap);
    void draw(const RenderSnapshot& snap, const telemetry::TelemetryHistory& hist, const ViewState& vs);
    void screenshot(const char* path);
    void focusSpecies(ViewState& vs, const RenderSnapshot& snap, std::int64_t id);
    void setZoomForShot(float x, float y, float zoom);

   private:
    struct Effect {
        float x, y, x2, y2, age, life;
        Rgb c;
        std::uint8_t type;
    };

    void spawnEffects(const RenderSnapshot& snap);
    void updateFieldTexture(const RenderSnapshot& snap);
    void drawWorld(const RenderSnapshot& snap, const ViewState& vs);
    void drawAgents(const RenderSnapshot& snap, const ViewState& vs);
    void drawBody(const AgentVis& a, Rgb color, float alpha, int lod);
    void drawSelection(const RenderSnapshot& snap);
    void drawEffects();
    void drawLabels(const RenderSnapshot& snap, const ViewState& vs);
    void drawTooltip(const RenderSnapshot& snap, const ViewState& vs);
    void sprite(float x, float y, float r, Rgb c, unsigned char a);
    int pickAgent(const RenderSnapshot& snap, Vector2 screen, float slackPx) const;

    CameraController camera_;
    Hud hud_;
    Font font_{};
    bool ownsFont_ = false;
    Texture2D dot_{}, fieldTex_{};
    std::vector<Color> fieldPix_;
    std::vector<Rgb> colorOf_;
    std::vector<Effect> effects_;
    float clock_ = 0.0f;
    Vector2 pressPos_{0, 0};
    int hover_ = -1;
    std::uint64_t lastTick_ = ~0ull;
};

}  // namespace alife::render
