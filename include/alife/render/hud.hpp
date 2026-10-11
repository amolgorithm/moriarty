#pragma once
#include <raylib.h>

#include "alife/telemetry/metrics.hpp"

namespace alife::render {

enum class ColorMode { Species, Energy };

struct ViewState {
    ColorMode color = ColorMode::Species;
    bool trails = true;
    bool hud = true;
    bool paused = false;
    float speed = 1.0f;
};

// Minimal overlay: stat block top-left, live charts bottom-right, key hints bottom-left.
class Hud {
   public:
    void draw(const Font& font, const telemetry::TelemetryHistory& hist, const ViewState& vs,
              int fps, int sw, int sh) const;
};

}  // namespace alife::render
