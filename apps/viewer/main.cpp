#include "alife/render/renderer.hpp"
#include "alife/telemetry/metrics.hpp"
#include "demo_source.hpp"

int main() {
    constexpr float kWorld = 2048.0f;
    constexpr std::size_t kAgents = 30'000;

    alife::render::Renderer renderer(1440, 900, "moriarty");
    renderer.fitCamera(kWorld, kWorld);

    moriarty::DemoSource source(kAgents, kWorld, kWorld, 0xC0FFEEULL);
    alife::telemetry::TelemetryHistory history;
    alife::render::RenderSnapshot snap;
    alife::render::ViewState view;

    int frame = 0;
    while (!renderer.shouldClose()) {
        renderer.pollInput(view, kWorld, kWorld);

        if (!view.paused) {
            source.step(renderer.frameTime() * view.speed);
            for (auto& e : source.takeEvents()) history.mark(std::move(e));
        }
        source.fill(snap);
        if (!view.paused && frame++ % 6 == 0) history.push(source.telemetry());

        renderer.draw(snap, history, view);
    }
    return 0;
}
