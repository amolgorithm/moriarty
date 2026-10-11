#pragma once
#include <raylib.h>

namespace alife::render {

// Smooth pan and zoom. Wheel zooms toward the cursor, any mouse button drags.
class CameraController {
   public:
    void fit(float worldW, float worldH, int screenW, int screenH);
    void onResize(float worldW, float worldH, int screenW, int screenH);
    void update(float dt);

    const Camera2D& get() const noexcept { return cam_; }
    bool movedThisFrame() const noexcept { return moved_; }

   private:
    Camera2D cam_{{0, 0}, {0, 0}, 0.0f, 1.0f};
    float targetZoom_ = 1.0f;
    bool userMoved_ = false;
    bool moved_ = false;
};

}  // namespace alife::render
