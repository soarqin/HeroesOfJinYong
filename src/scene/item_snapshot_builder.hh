#pragma once

#include "logic/presentation.hh"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace hojy::scene {

using ItemSelectionEntry = std::pair<std::int16_t, std::int16_t>;

[[nodiscard]] std::vector<ItemViewEntrySnapshot> buildItemViewSnapshot(
    const std::vector<ItemSelectionEntry> &items,
    std::optional<std::pair<int, int>> currentPosition = std::nullopt);

[[nodiscard]] std::vector<ItemViewEntrySnapshot> buildBattleItemViewSnapshot(
    const std::vector<ItemSelectionEntry> &items);

}
