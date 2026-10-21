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

void testConfirmationLayoutMatchesOriginalHorizontalMenu() {
    const auto layout = hojy::scene::detail::titleConfirmationLayout(
        50, 120, 80, 16, 5, {20, 24});
    HOJY_CHECK_EQ(layout.panelX, 140);
    HOJY_CHECK_EQ(layout.panelY, 115);
    HOJY_CHECK_EQ(layout.panelWidth, 59);
    HOJY_CHECK_EQ(layout.panelHeight, 26);
    HOJY_CHECK_EQ(layout.choiceX[0], 145);
    HOJY_CHECK_EQ(layout.choiceX[1], 170);
    HOJY_CHECK_EQ(layout.choiceY, 120);
}

void testConfirmationChoicesStayOnPromptRow() {
    const auto layout = hojy::scene::detail::titleConfirmationLayout(
        10, 180, 42, 24, 8, {18, 19});
    HOJY_CHECK_EQ(layout.choiceY, 180);
    HOJY_CHECK_EQ(layout.panelY, 172);
    HOJY_CHECK_EQ(layout.panelHeight, 40);
}

}

int main() {
    try {
        testSelectionOffsetsStartAtBaseRow();
        testSelectionOffsetsUseIntegerScaledGridStep();
        testConfirmationLayoutMatchesOriginalHorizontalMenu();
        testConfirmationChoicesStayOnPromptRow();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
