#pragma once

#include <array>
#include <utility>

namespace hojy::scene::detail {

inline std::array<int, 3> titleMenuSelectionOffsets(
        int baseY, std::pair<int, int> scale) {
    const auto step = 20 * scale.first / scale.second;
    return {baseY, baseY + step, baseY + step * 2};
}

}
