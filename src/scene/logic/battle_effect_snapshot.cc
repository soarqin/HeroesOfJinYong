#include "battle_effect_snapshot.hh"

#include <cstdint>
#include <limits>
#include <new>
#include <utility>

namespace hojy::scene::logic {

bool buildBattleEffectOverlaySnapshot(
    int width,
    int height,
    int effectAssetId,
    int frameIndex,
    const std::vector<BattleEffectCell> &cells,
    BattleEffectOverlaySnapshot &output) noexcept {
    if (width <= 0 || height <= 0 || effectAssetId < 0) { return false; }
    const auto mapWidth = static_cast<std::size_t>(width);
    const auto mapHeight = static_cast<std::size_t>(height);
    if (mapWidth > std::numeric_limits<std::size_t>::max() / mapHeight) {
        return false;
    }
    const auto cellCount = mapWidth * mapHeight;
    try {
        BattleEffectOverlaySnapshot candidate;
        candidate.effectAssetId = effectAssetId;
        candidate.frameIndex = frameIndex;
        candidate.cellIndices.reserve(cells.size());
        std::vector<std::uint8_t> seen(cellCount, 0);
        for (const auto &cell: cells) {
            if (cell.x < 0 || cell.y < 0
                || cell.x >= width || cell.y >= height) {
                return false;
            }
            const auto index = static_cast<std::size_t>(cell.y) * mapWidth
                + static_cast<std::size_t>(cell.x);
            if (seen[index] != 0) { continue; }
            seen[index] = 1;
            candidate.cellIndices.push_back(index);
        }
        output = std::move(candidate);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

}
