#include "application.hh"

#include <algorithm>
#include <chrono>
#include <limits>

namespace hojy::app {

Application::Application(int width, int height, double animationSpeed):
    window_(width, height),
    scheduler_(FixedTickMicros, CompatibilityDivisor),
    compatibilityScheduler_(60.0, LegacyLogicRateHz * std::max(0.0, animationSpeed)) {
    simulationTime_ = window_.currTime();
    lastWallTime_ = wallTimeMicros();
    window_.setSimulationTime(simulationTime_);
}

std::uint64_t Application::wallTimeMicros() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now);
    return micros.count() < 0 ? 0 : static_cast<std::uint64_t>(micros.count());
}

int Application::run() {
    if (!window_.ready()) {
        return 1;
    }
    running_ = true;
    while (running_ && !window_.quitRequested()) {
        inputCollector_.collect(inputQueue_);

        const auto now = wallTimeMicros();
        const auto elapsed = now >= lastWallTime_ ? now - lastWallTime_ : 0;
        lastWallTime_ = now;
        const auto batch = scheduler_.advance(elapsed);

        for (std::uint32_t tick = 0; tick < batch.fixedTicks; ++tick) {
            if (simulationTime_ > std::numeric_limits<std::uint64_t>::max() - FixedTickMicros) {
                simulationTime_ = std::numeric_limits<std::uint64_t>::max();
            } else {
                simulationTime_ += FixedTickMicros;
            }
            window_.setSimulationTime(simulationTime_);
            for (const auto &event : inputQueue_.drainThrough(simulationTime_)) {
                window_.dispatchInput(event);
            }
            window_.updateFixed();
            const auto compatibilityTicks = compatibilityScheduler_.advance();
            for (std::uint32_t compatibilityTick = 0;
                 compatibilityTick < compatibilityTicks; ++compatibilityTick) {
                window_.compatibilityUpdate();
            }
            if (window_.quitRequested()) { break; }
        }

        if (window_.quitRequested()) { break; }

        if (!window_.prepareRender()) {
            return 1;
        }
        window_.render();
        if (!window_.flush()) {
            continue;
        }
    }
    return 0;
}

void Application::stop() {
    running_ = false;
    window_.requestQuit();
}

}
