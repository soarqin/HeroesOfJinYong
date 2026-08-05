#include "battle_presentation_snapshot_builder.hh"

#include "world/savedata.hh"
#include "world/strings.hh"

#include <fmt/xchar.h>

namespace hojy::scene {

std::vector<std::wstring> buildBattleItemResultMessages(
        std::int16_t itemId,
        const std::map<::hojy::world::state::PropType, std::int16_t> &changes) {
    std::vector<std::wstring> messages;
    if (itemId < 0 || static_cast<std::size_t>(itemId)
            >= ::hojy::world::state::gSaveData.itemInfo.size()
        || !::hojy::world::state::gSaveData.itemInfo[itemId]) {
        messages.push_back(GETTEXT(115));
        return messages;
    }
    messages.push_back(GETTEXT(37) + L' ' + GETITEMNAME(itemId));
    messages.reserve(changes.size() + 1);
    for (const auto &[property, value]: changes) {
        messages.push_back(fmt::format(
            L"{} {} {}", ::hojy::world::state::propToName(property),
            GETTEXT(value ? 34 : 35), value));
    }
    return messages;
}

std::wstring buildBattleSkillLevelMessage(
        std::int16_t skillId, std::int16_t level) {
    if (skillId <= 0 || level <= 0) { return GETTEXT(115); }
    return fmt::format(GETTEXT(81), GETSKILLNAME(skillId), level);
}

}
