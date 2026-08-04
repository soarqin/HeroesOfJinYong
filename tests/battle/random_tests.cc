#include "battle/random.hh"
#include "battle/game_random.hh"
#include "test_support.hh"

#include <exception>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

int main() {
    try {
        hojy::battle::SequenceRandom random({7, 2, 9});
        HOJY_CHECK_EQ(random.next(10), 7);
        HOJY_CHECK_EQ(random.next(1, 3), 3);
        HOJY_CHECK_EQ(random.next(4), 1);
        HOJY_CHECK_EQ(random.callCount(), 3U);
        HOJY_CHECK_EQ(random.calls()[1].minimum, 1);
        HOJY_CHECK_EQ(random.calls()[1].maximum, 3);
        HOJY_CHECK_EQ(random.next(1), 0);
        HOJY_CHECK_EQ(random.callCount(), 3U);

        hojy::battle::SequenceRandom boundary({5});
        HOJY_CHECK_EQ(boundary.next(0), 0);
        HOJY_CHECK_EQ(boundary.next(1), 0);
        HOJY_CHECK_EQ(boundary.next(30001), 0);
        HOJY_CHECK_EQ(boundary.callCount(), 0U);
        HOJY_CHECK_EQ(boundary.next(10), 5);
        HOJY_CHECK_THROWS(std::invalid_argument, boundary.next(4, 3));
        HOJY_CHECK_THROWS(std::out_of_range, boundary.next(10));

        hojy::battle::SequenceRandom wide({-1});
        HOJY_CHECK_EQ(
            wide.next(std::numeric_limits<int>::min(),
                      std::numeric_limits<int>::max()),
            std::numeric_limits<int>::max());

        hojy::battle::SequenceRandom wideSource({-1});
        hojy::battle::RecordingRandom recording(wideSource);
        HOJY_CHECK_EQ(
            recording.next(std::numeric_limits<int>::min(),
                            std::numeric_limits<int>::max()),
            std::numeric_limits<int>::max());
        HOJY_CHECK_EQ(
            recording.calls()[0].rawValue,
            static_cast<std::int64_t>(std::numeric_limits<int>::max())
                - std::numeric_limits<int>::min());
        HOJY_CHECK_EQ(recording.calls()[0].result,
                      std::numeric_limits<int>::max());

        hojy::battle::GameRandom gameRandom;
        bool observedNonMinimum = false;
        for (int index = 0; index < 32; ++index) {
            const int value = gameRandom.next(-5, 5);
            if (value < -5 || value > 5) {
                throw std::runtime_error("GameRandom returned an out-of-range value");
            }
            observedNonMinimum = observedNonMinimum || value != -5;
        }
        HOJY_CHECK_EQ(observedNonMinimum, true);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
