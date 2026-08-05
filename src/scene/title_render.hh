#pragma once

#include <array>
#include <utility>

namespace hojy::scene::detail {

struct TitleConfirmationLayout final {
    int panelX = 0;
    int panelY = 0;
    int panelWidth = 0;
    int panelHeight = 0;
    std::array<int, 2> choiceX{};
    int choiceY = 0;
};

inline std::array<int, 3> titleMenuSelectionOffsets(
        int baseY, std::pair<int, int> scale) {
    const auto step = 20 * scale.first / scale.second;
    return {baseY, baseY + step, baseY + step * 2};
}

inline TitleConfirmationLayout titleConfirmationLayout(
        int baseX, int promptY, int promptWidth, int fontSize,
        int windowBorder, std::array<int, 2> choiceWidths) {
    TitleConfirmationLayout result;
    result.panelX = baseX + promptWidth + windowBorder * 2;
    result.panelY = promptY - windowBorder;
    result.panelWidth = choiceWidths[0] + choiceWidths[1]
        + windowBorder * 3;
    result.panelHeight = fontSize + windowBorder * 2;
    result.choiceX[0] = result.panelX + windowBorder;
    result.choiceX[1] = result.choiceX[0] + choiceWidths[0] + windowBorder;
    result.choiceY = promptY;
    return result;
}

}
