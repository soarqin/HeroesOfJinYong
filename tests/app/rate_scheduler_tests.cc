#include "app/rate_scheduler.hh"

#include "test_support.hh"

#include <iostream>

namespace {

void testDefaultCompatibilityRateTicksEveryFourFixedUpdates() {
    hojy::app::RateScheduler scheduler(60.0, 15.0);
    HOJY_CHECK_EQ(scheduler.advance(), 0U);
    HOJY_CHECK_EQ(scheduler.advance(), 0U);
    HOJY_CHECK_EQ(scheduler.advance(), 0U);
    HOJY_CHECK_EQ(scheduler.advance(), 1U);
}

void testFractionalAnimationSpeedKeepsLongTermRate() {
    hojy::app::RateScheduler scheduler(60.0, 22.5);
    std::uint32_t ticks = 0;
    for (int i = 0; i < 8; ++i) {
        ticks += scheduler.advance();
    }
    HOJY_CHECK_EQ(ticks, 3U);
}

}

int main() {
    try {
        testDefaultCompatibilityRateTicksEveryFourFixedUpdates();
        testFractionalAnimationSpeedKeepsLongTermRate();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
