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
        HOJY_CHECK_EQ(random.next(1), 0);
        HOJY_CHECK_EQ(random.callCount(), 3U);

        hojy::battle::SequenceRandom boundary({5});
        HOJY_CHECK_EQ(boundary.next(0), 0);
        HOJY_CHECK_EQ(boundary.next(1), 0);
        HOJY_CHECK_EQ(boundary.next(30001), 0);
        HOJY_CHECK_EQ(boundary.callCount(), 0U);
        HOJY_CHECK_EQ(boundary.next(10), 5);
        HOJY_CHECK_THROWS(std::invalid_argument, boundary.next(4, 3));
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
