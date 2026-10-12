#pragma once
#include <raylib.h>

namespace alife::render {

// Smooth pan and zoom. Wheel zooms toward the cursor, drag pans. flyTo() glides to a
// world point, and follow mode keeps re-aiming the goal at a moving target.
class CameraController {
   public:
    void fit(float worldW, float worldH, int screenW, int screenH);
    void onResize(float worldW, float worldH, int screenW, int screenH);
    void flyTo(float x, float y, float zoom, int screenW, int screenH, float horizontalShift = 0.0f);
    void setGoal(float x, float y) { goal_ = {x, y}; }
    void setFollow(bool on) { following_ = on; if (!on) goalActive_ = false; }
    void update(float dt, bool allowWheelZoom = true);

    const Camera2D& get() const noexcept { return cam_; }
    float zoom() const noexcept { return cam_.zoom; }
    bool movedThisFrame() const noexcept { return moved_; }
    bool interrupted() const noexcept { return interrupted_; }  // user dragged away this frame

   private:
    Camera2D cam_{{0, 0}, {0, 0}, 0.0f, 1.0f};
    Vector2 goal_{0, 0};
    float targetZoom_ = 1.0f;
    bool goalActive_ = false, following_ = false;
    bool userMoved_ = false, moved_ = false, interrupted_ = false;
};

}  // namespace alife::render
