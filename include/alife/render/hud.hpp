#pragma once
#include <raylib.h>

#include <cstdint>
#include <vector>

#include "alife/render/snapshot.hpp"
#include "alife/telemetry/metrics.hpp"

namespace alife::render {

enum class ColorMode { Species, Energy };

struct ViewState {
    ColorMode color = ColorMode::Species;
    bool wakes = true;    // motion streaks
    bool fields = true;   // food and heat underlay
    bool hud = true;
    bool paused = false;
    float speed = 12.0f;  // sim seconds per real second (target)
    float simRate = 0.0f; // measured, set by the app
    std::uint64_t selected = 0;      // organism id, 0 = none
    std::int64_t focusSpecies = -1;  // species id, -1 = none
    bool follow = false;
};

// Overlay: stats + species list (left), food web (right top), charts (right bottom),
// organism inspector (bottom left).
class Hud {
   public:
    void draw(const Font& font, const RenderSnapshot& snap, const telemetry::TelemetryHistory& hist,
              const ViewState& vs, int fps, int sw, int sh);
    bool overPanel(Vector2 p) const;
    std::int64_t speciesRowAt(Vector2 p) const;  // species id under the cursor, -1 if none

   private:
    std::vector<Rectangle> panels_;
    std::vector<std::pair<Rectangle, std::uint32_t>> rows_;
};

}  // namespace alife::render
