#pragma once

#include "fixed_scheduler.hh"
#include "input.hh"
#include "rate_scheduler.hh"
#include "sdl_input.hh"
#include "scene/window.hh"

#include <cstdint>

namespace hojy::app {

class Application final {
public:
    static constexpr std::uint64_t FixedTickMicros = 16666;
    static constexpr std::uint32_t CompatibilityDivisor = 4;

    Application(int width, int height, double animationSpeed = 1.0);
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    int run();
    void stop();

private:
    static std::uint64_t wallTimeMicros();

    scene::Window window_;
    InputQueue inputQueue_;
    SdlInputCollector inputCollector_;
    FixedTickAccumulator scheduler_;
    RateScheduler compatibilityScheduler_;
    std::uint64_t simulationTime_ = 0;
    std::uint64_t lastWallTime_ = 0;
    bool running_ = false;
};

}
