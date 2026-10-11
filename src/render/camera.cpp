#include "alife/render/camera.hpp"

#include <algorithm>
#include <cmath>

namespace alife::render {

void CameraController::fit(float ww, float wh, int sw, int sh) {
    const float z = std::min(static_cast<float>(sw) / ww, static_cast<float>(sh) / wh) * 0.92f;
    cam_.zoom = targetZoom_ = z;
    cam_.target = {ww * 0.5f, wh * 0.5f};
    cam_.offset = {static_cast<float>(sw) * 0.5f, static_cast<float>(sh) * 0.5f};
    userMoved_ = false;
    moved_ = true;
}

void CameraController::onResize(float ww, float wh, int sw, int sh) {
    if (!userMoved_) {
        fit(ww, wh, sw, sh);
    } else {
        moved_ = true;
    }
}

void CameraController::update(float dt) {
    moved_ = false;

    // zoom toward the cursor: pin the world point under the mouse, then animate zoom
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        const Vector2 m = GetMousePosition();
        cam_.target = GetScreenToWorld2D(m, cam_);
        cam_.offset = m;
        targetZoom_ = std::clamp(targetZoom_ * std::pow(1.15f, wheel), 0.05f, 40.0f);
        userMoved_ = true;
    }
    if (std::fabs(targetZoom_ - cam_.zoom) > 1e-4f * cam_.zoom) {
        const float k = 1.0f - std::exp(-14.0f * dt);
        cam_.zoom += (targetZoom_ - cam_.zoom) * k;
        moved_ = true;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
        IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        const Vector2 d = GetMouseDelta();
        if (d.x != 0.0f || d.y != 0.0f) {
            cam_.target.x -= d.x / cam_.zoom;
            cam_.target.y -= d.y / cam_.zoom;
            userMoved_ = true;
            moved_ = true;
        }
    }
}

}  // namespace alife::render
