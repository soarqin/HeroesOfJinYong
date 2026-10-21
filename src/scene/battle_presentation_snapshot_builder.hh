#pragma once

#include "world/action.hh"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace hojy::scene {

[[nodiscard]] std::vector<std::wstring> buildBattleItemResultMessages(
    std::int16_t itemId,
    const std::map<::hojy::world::state::PropType, std::int16_t> &changes);

[[nodiscard]] std::wstring buildBattleSkillLevelMessage(
    std::int16_t skillId,
    std::int16_t level);

}
