#include "app/input_repeat.hh"

#include "test_support.hh"

#include <iostream>

namespace {

void testKeyRepeatUsesInitialDelayAndStableInterval() {
    hojy::app::InputRepeater repeater;
    repeater.press(7, hojy::app::InputDevice::Keyboard,
                   hojy::app::InputAction::Left, 1000);

    auto first = repeater.drainThrough(1000);
    HOJY_CHECK_EQ(first.size(), 1U);
    HOJY_CHECK_EQ(first.front().timestamp, 1000ULL);

    auto beforeRepeat = repeater.drainThrough(180999);
    HOJY_CHECK_EQ(beforeRepeat.size(), 0U);
    auto repeat = repeater.drainThrough(181000);
    HOJY_CHECK_EQ(repeat.size(), 1U);
    HOJY_CHECK_EQ(repeat.front().timestamp, 181000ULL);

    auto twoRepeats = repeater.drainThrough(221000);
    HOJY_CHECK_EQ(twoRepeats.size(), 2U);
    HOJY_CHECK_EQ(twoRepeats[0].timestamp, 201000ULL);
    HOJY_CHECK_EQ(twoRepeats[1].timestamp, 221000ULL);
}

void testReleaseCancelsPendingRepeats() {
    hojy::app::InputRepeater repeater;
    repeater.press(3, hojy::app::InputDevice::Controller,
                   hojy::app::InputAction::Accept, 5000);
    const auto first = repeater.drainThrough(5000);
    HOJY_CHECK_EQ(first.size(), 1U);
    repeater.release(3);
    const auto afterRelease = repeater.drainThrough(500000);
    HOJY_CHECK_EQ(afterRelease.size(), 0U);
}

}

int main() {
    try {
        testKeyRepeatUsesInitialDelayAndStableInterval();
        testReleaseCancelsPendingRepeats();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
