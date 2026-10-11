#pragma once
#include <raylib.h>

#include "alife/render/camera.hpp"
#include "alife/render/hud.hpp"
#include "alife/render/snapshot.hpp"
#include "alife/telemetry/metrics.hpp"

namespace alife::render {

// Owns the window, camera, trail buffer and HUD. The sim never touches raylib,
// it only hands over a RenderSnapshot and a TelemetryHistory.
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
    void pollInput(ViewState& vs, float worldW, float worldH);
    void draw(const RenderSnapshot& snap, const telemetry::TelemetryHistory& hist,
              const ViewState& vs);

   private:
    void recreateTrail();
    void drawAgents(const RenderSnapshot& snap, ColorMode mode, float alpha, float radiusScale);
    void drawWorldFrame(const RenderSnapshot& snap);

    CameraController camera_;
    Hud hud_;
    Font font_{};
    bool ownsFont_ = false;
    Texture2D dot_{};
    RenderTexture2D trail_{};
    bool clearTrail_ = true;
    bool prevTrails_ = true;
};

}  // namespace alife::render
