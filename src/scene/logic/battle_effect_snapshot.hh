#pragma once

#include <cstddef>
#include <vector>

namespace hojy::scene::logic {

struct BattleEffectCell final {
    int x = 0;
    int y = 0;
};

struct BattleEffectOverlaySnapshot final {
    int effectAssetId = -1;
    int frameIndex = -1;
    std::vector<std::size_t> cellIndices;
};

bool buildBattleEffectOverlaySnapshot(
    int width,
    int height,
    int effectAssetId,
    int frameIndex,
    const std::vector<BattleEffectCell> &cells,
    BattleEffectOverlaySnapshot &output) noexcept;

}
