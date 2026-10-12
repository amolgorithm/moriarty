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
    goalActive_ = following_ = false;
    moved_ = true;
}

void CameraController::onResize(float ww, float wh, int sw, int sh) {
    if (!userMoved_ && !goalActive_) fit(ww, wh, sw, sh);
    else {
        cam_.offset = {static_cast<float>(sw) * 0.5f, static_cast<float>(sh) * 0.5f};
        moved_ = true;
    }
}

void CameraController::flyTo(float x, float y, float zoom, int sw, int sh, float horizontalShift) {
    const Vector2 c{static_cast<float>(sw) * 0.5f, static_cast<float>(sh) * 0.5f};
    cam_.target = GetScreenToWorld2D(c, cam_);  // keep the view steady while we re-anchor
    cam_.offset = {c.x - horizontalShift, c.y};
    goal_ = {x, y};
    targetZoom_ = std::clamp(zoom, 0.05f, 40.0f);
    goalActive_ = true;
    userMoved_ = true;
}

void CameraController::update(float dt, bool allowWheelZoom) {
    moved_ = false;
    interrupted_ = false;

    const float wheel = allowWheelZoom ? GetMouseWheelMove() : 0.0f;
    if (wheel != 0.0f) {
        if (!goalActive_) {
            // zoom toward the cursor: pin the world point under the mouse
            const Vector2 m = GetMousePosition();
            cam_.target = GetScreenToWorld2D(m, cam_);
            cam_.offset = m;
        }
        targetZoom_ = std::clamp(targetZoom_ * std::pow(1.15f, wheel), 0.05f, 40.0f);
        userMoved_ = true;
    }

    const bool drag = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
                      IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    if (drag) {
        const Vector2 d = GetMouseDelta();
        if (std::fabs(d.x) + std::fabs(d.y) > 0.5f) {
            if (goalActive_) {  // dragging cancels follow and fly-to
                goalActive_ = following_ = false;
                interrupted_ = true;
            }
            cam_.target.x -= d.x / cam_.zoom;
            cam_.target.y -= d.y / cam_.zoom;
            userMoved_ = true;
            moved_ = true;
        }
    }

    if (goalActive_) {
        const float k = 1.0f - std::exp(-6.0f * dt);
        cam_.target.x += (goal_.x - cam_.target.x) * k;
        cam_.target.y += (goal_.y - cam_.target.y) * k;
        moved_ = true;
    }
    if (std::fabs(targetZoom_ - cam_.zoom) > 1e-4f * cam_.zoom) {
        const float k = 1.0f - std::exp(-(goalActive_ ? 5.0f : 14.0f) * dt);
        cam_.zoom += (targetZoom_ - cam_.zoom) * k;
        moved_ = true;
    }
    if (goalActive_ && !following_ && std::fabs(goal_.x - cam_.target.x) < 0.5f &&
        std::fabs(goal_.y - cam_.target.y) < 0.5f && std::fabs(targetZoom_ - cam_.zoom) < 0.01f * cam_.zoom)
        goalActive_ = false;
}

}  // namespace alife::render
