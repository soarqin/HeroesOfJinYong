#include "battle/random.hh"
#include "test_support.hh"

#include <exception>
#include <iostream>
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
        HOJY_CHECK_THROWS(std::out_of_range, random.next(1));

        hojy::battle::SequenceRandom invalidRange({1});
        HOJY_CHECK_THROWS(std::invalid_argument, invalidRange.next(0));
        HOJY_CHECK_THROWS(std::invalid_argument, invalidRange.next(4, 3));
        HOJY_CHECK_EQ(invalidRange.callCount(), 0U);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
