#include "app/fixed_scheduler.hh"
#include "app/input.hh"

#include "test_support.hh"

#include <iostream>

namespace {

void testInputQueueOrdersByTimestampAndPreservesEqualTimestampOrder() {
    hojy::app::InputQueue queue;
    queue.push({30, hojy::app::InputDevice::Keyboard,
                hojy::app::InputAction::Right, 0, L""});
    queue.push({10, hojy::app::InputDevice::Keyboard,
                hojy::app::InputAction::Left, 0, L""});
    queue.push({10, hojy::app::InputDevice::Keyboard,
                hojy::app::InputAction::Up, 0, L""});

    const auto first = queue.pop();
    const auto second = queue.pop();
    const auto third = queue.pop();
    HOJY_CHECK_EQ(first.has_value(), true);
    HOJY_CHECK_EQ(second.has_value(), true);
    HOJY_CHECK_EQ(third.has_value(), true);
    HOJY_CHECK_EQ(first->action, hojy::app::InputAction::Left);
    HOJY_CHECK_EQ(second->action, hojy::app::InputAction::Up);
    HOJY_CHECK_EQ(third->action, hojy::app::InputAction::Right);
}

void testInputQueueDrainsOnlyEventsAtOrBeforeTimestamp() {
    hojy::app::InputQueue queue;
    queue.push({10, hojy::app::InputDevice::Keyboard,
                hojy::app::InputAction::Left, 0, L""});
    queue.push({20, hojy::app::InputDevice::Keyboard,
                hojy::app::InputAction::Right, 0, L""});

    const auto ready = queue.drainThrough(10);
    HOJY_CHECK_EQ(ready.size(), 1U);
    HOJY_CHECK_EQ(ready.front().action, hojy::app::InputAction::Left);
    HOJY_CHECK_EQ(queue.size(), 1U);
}

void testFixedSchedulerEmitsOneCompatibilityTickEveryFourFixedTicks() {
    hojy::app::FixedTickAccumulator scheduler(16666, 4);
    const auto first = scheduler.advance(16666 * 3);
    HOJY_CHECK_EQ(first.fixedTicks, 3U);
    HOJY_CHECK_EQ(first.compatibilityTicks, 0U);

    const auto second = scheduler.advance(16666);
    HOJY_CHECK_EQ(second.fixedTicks, 1U);
    HOJY_CHECK_EQ(second.compatibilityTicks, 1U);
}

void testFixedSchedulerRetainsSubTickRemainder() {
    hojy::app::FixedTickAccumulator scheduler(1000, 4);
    const auto first = scheduler.advance(2500);
    HOJY_CHECK_EQ(first.fixedTicks, 2U);
    HOJY_CHECK_EQ(first.compatibilityTicks, 0U);
    const auto second = scheduler.advance(500);
    HOJY_CHECK_EQ(second.fixedTicks, 1U);
    HOJY_CHECK_EQ(second.compatibilityTicks, 0U);
}

void testFixedSchedulerDropsExcessCatchUpToAvoidSpiralOfDeath() {
    hojy::app::FixedTickAccumulator scheduler(1000, 4, 2);
    const auto batch = scheduler.advance(10000);
    HOJY_CHECK_EQ(batch.fixedTicks, 2U);
    HOJY_CHECK_EQ(batch.compatibilityTicks, 0U);
    HOJY_CHECK_EQ(scheduler.remainderMicros(), 0ULL);
}

}

int main() {
    try {
        testInputQueueOrdersByTimestampAndPreservesEqualTimestampOrder();
        testInputQueueDrainsOnlyEventsAtOrBeforeTimestamp();
        testFixedSchedulerEmitsOneCompatibilityTickEveryFourFixedTicks();
        testFixedSchedulerRetainsSubTickRemainder();
        testFixedSchedulerDropsExcessCatchUpToAvoidSpiralOfDeath();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
