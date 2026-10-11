#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "alife/concurrency/thread_pool.hpp"
#include "alife/core/world.hpp"
#include "alife/render/renderer.hpp"
#include "alife/render/snapshot_builder.hpp"
#include "alife/sim/scheduler.hpp"
#include "alife/telemetry/collect.hpp"

namespace {

struct Options {
    const char* shot = nullptr;  // --shot out.png : render a few frames, save, exit (for docs and CI)
    double warm = 0.0;           // --warm S       : simulate S seconds before opening the window
    float zoom = 0.0f;           // --zoom Z       : zoom on the largest species when taking a shot
    int frames = 90;
};

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--shot") && i + 1 < argc) o.shot = argv[++i];
        else if (!std::strcmp(argv[i], "--warm") && i + 1 < argc) o.warm = std::atof(argv[++i]);
        else if (!std::strcmp(argv[i], "--zoom") && i + 1 < argc) o.zoom = static_cast<float>(std::atof(argv[++i]));
        else if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) o.frames = std::atoi(argv[++i]);
    }
    return o;
}

}  // namespace

int main(int argc, char** argv) {
    const Options opt = parse(argc, argv);

    alife::World world;
    alife::ThreadPool pool;
    alife::sim::Scheduler sched(world, pool);
    const float dt = world.cfg.dtPhys;

    alife::telemetry::TelemetryHistory history;
    std::uint64_t lastTelemetry = 0;
    auto pushTelemetry = [&] {
        history.push(alife::telemetry::collect(world));
        lastTelemetry = world.tick;
    };

    // optional warm start, records telemetry so the charts have history
    const auto warmTicks = static_cast<std::uint64_t>(opt.warm / static_cast<double>(dt));
    for (std::uint64_t t = 0; t < warmTicks; ++t) {
        sched.stepOnce();
        if (world.tick - lastTelemetry >= static_cast<std::uint64_t>(world.cfg.evoEvery)) {
            for (auto& m : world.pendingMarkers) history.mark(std::move(m));
            world.pendingMarkers.clear();
            pushTelemetry();
        }
    }

    alife::render::Renderer renderer(1440, 900, "moriarty");
    renderer.fitCamera(world.cfg.worldW, world.cfg.worldH);
    alife::render::RenderSnapshot snap;
    alife::render::ViewState view;
    alife::render::buildSnapshot(world, snap, 0);

    double debt = 0.0;
    int frame = 0;
    bool shotPrepared = false;
    using clock = std::chrono::steady_clock;

    while (!renderer.shouldClose()) {
        renderer.pollInput(view, snap);

        // run the sim against a wall-clock budget so the UI stays fluid at any speed
        if (!view.paused) {
            debt += static_cast<double>(renderer.frameTime() * view.speed);
            const auto t0 = clock::now();
            std::uint64_t ran = 0;
            while (debt >= static_cast<double>(dt) && std::chrono::duration<double>(clock::now() - t0).count() < 0.012) {
                sched.stepOnce();
                debt -= static_cast<double>(dt);
                ++ran;
            }
            if (debt > 0.5) debt = 0.5;  // can't keep up, drop the backlog
            const float rate = static_cast<float>(static_cast<double>(ran) * static_cast<double>(dt) / std::max(1e-4f, renderer.frameTime()));
            view.simRate = view.simRate * 0.9f + rate * 0.1f;
        }

        if (world.tick - lastTelemetry >= static_cast<std::uint64_t>(world.cfg.evoEvery)) {
            for (auto& m : world.pendingMarkers) history.mark(std::move(m));
            world.pendingMarkers.clear();
            pushTelemetry();
        }

        alife::render::buildSnapshot(world, snap, view.selected);

        if (opt.shot && !shotPrepared && frame == 1) {
            shotPrepared = true;
            if (opt.zoom > 0.0f && !snap.species.empty()) {
                renderer.focusSpecies(view, snap, snap.species.front().id);
                renderer.setZoomForShot(snap.species.front().labelX, snap.species.front().labelY, opt.zoom);
            }
        }
        renderer.draw(snap, history, view);

        if (opt.shot && ++frame >= opt.frames) {
            renderer.screenshot(opt.shot);
            break;
        }
        if (!opt.shot) ++frame;
    }
    return 0;
}
