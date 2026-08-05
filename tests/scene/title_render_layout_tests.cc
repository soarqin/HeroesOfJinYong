#include "scene/title_render.hh"

#include "test_support.hh"

#include <iostream>

namespace {

void testSelectionOffsetsStartAtBaseRow() {
    const auto offsets = hojy::scene::detail::titleMenuSelectionOffsets(100, {2, 1});
    HOJY_CHECK_EQ(offsets[0], 100);
    HOJY_CHECK_EQ(offsets[1], 140);
    HOJY_CHECK_EQ(offsets[2], 180);
}

void testSelectionOffsetsUseIntegerScaledGridStep() {
    const auto offsets = hojy::scene::detail::titleMenuSelectionOffsets(100, {1, 2});
    HOJY_CHECK_EQ(offsets[0], 100);
    HOJY_CHECK_EQ(offsets[1], 110);
    HOJY_CHECK_EQ(offsets[2], 120);
}

}

int main() {
    try {
        testSelectionOffsetsStartAtBaseRow();
        testSelectionOffsetsUseIntegerScaledGridStep();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
