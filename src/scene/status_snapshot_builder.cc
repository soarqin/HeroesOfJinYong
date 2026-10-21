#include "status_snapshot_builder.hh"

#include "content/factors.hh"
#include "world/action.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <algorithm>
#include <cstddef>

namespace hojy::scene {
namespace {

void copyCharacterFields(CharacterStatusSnapshot &snapshot,
                         const ::hojy::world::state::CharacterData &data) {
    snapshot.id = data.id;
    snapshot.headId = data.headId;
    snapshot.level = data.level;
    snapshot.exp = data.exp;
    snapshot.hp = data.hp;
    snapshot.maxHp = data.maxHp;
    snapshot.hurt = data.hurt;
    snapshot.poisoned = data.poisoned;
    snapshot.stamina = data.stamina;
    snapshot.mpType = data.mpType;
    snapshot.mp = data.mp;
    snapshot.maxMp = data.maxMp;
    snapshot.attack = data.attack;
    snapshot.speed = data.speed;
    snapshot.defence = data.defence;
    snapshot.medic = data.medic;
    snapshot.poison = data.poison;
    snapshot.depoison = data.depoison;
    snapshot.fist = data.fist;
    snapshot.sword = data.sword;
    snapshot.blade = data.blade;
    snapshot.special = data.special;
    snapshot.throwing = data.throwing;
    snapshot.knowledge = data.knowledge;
    snapshot.integrity = data.integrity;
    snapshot.reputation = data.reputation;
    snapshot.potential = data.potential;
    snapshot.equip = {data.equip[0], data.equip[1]};
    snapshot.learningItem = data.learningItem;
    snapshot.expForItem = data.expForItem;
    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        snapshot.skillId[static_cast<std::size_t>(i)] = data.skillId[i];
        snapshot.skillLevel[static_cast<std::size_t>(i)] = data.skillLevel[i];
    }
}

bool validItemId(std::int16_t itemId,
                 const ::hojy::world::state::ItemInfo &items) noexcept {
    return itemId >= 0
        && static_cast<std::size_t>(itemId) < items.size()
        && items[itemId] != nullptr;
}

std::optional<CharacterStatusSnapshot> buildSnapshot(
    const ::hojy::world::state::CharacterData &source,
    const ::hojy::world::state::ItemInfo &items,
    bool simpleMode, bool showPotential) {
    // Work from a value copy.  Equipment bonuses must never mutate the live
    // character while a presentation request is being prepared.
    auto effective = source;
    ::hojy::world::state::addUpPropFromEquipToChar(items, &effective);

    CharacterStatusSnapshot snapshot;
    copyCharacterFields(snapshot, effective);
    snapshot.simpleMode = simpleMode;
    snapshot.showPotential = showPotential;
    snapshot.name = GETCHARNAME(effective.id);
    snapshot.expForLevelUp =
        ::hojy::world::state::getExpForLevelUp(effective.level);
    std::tie(snapshot.mpColorR, snapshot.mpColorG, snapshot.mpColorB) =
        ::hojy::world::state::calcColorForMpType(effective.mpType);

    for (std::int16_t id = 0; id < 138; ++id) {
        snapshot.labels[static_cast<std::size_t>(id)] = GETTEXT(id);
    }

    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        const auto index = static_cast<std::size_t>(i);
        const auto skillId = snapshot.skillId[index];
        if (skillId > 0) {
            snapshot.skillNames[index] = GETSKILLNAME(skillId);
        }
    }

    for (int i = 0; i < 2; ++i) {
        const auto itemId = snapshot.equip[static_cast<std::size_t>(i)];
        if (validItemId(itemId, items)) {
            snapshot.equipNames[static_cast<std::size_t>(i)] = GETITEMNAME(itemId);
        }
    }
    if (validItemId(snapshot.learningItem, items)) {
        snapshot.learningItemName = GETITEMNAME(snapshot.learningItem);
        const auto *item = items[snapshot.learningItem];
        snapshot.learningSkillId = item->skillId;
    } else {
        snapshot.learningItem = -1;
        snapshot.expForItem = 0;
    }

    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        const auto index = static_cast<std::size_t>(i);
        if (snapshot.skillId[index] <= 0
            || snapshot.skillId[index] != snapshot.learningSkillId) {
            continue;
        }
        snapshot.learningLevel = std::clamp<std::int16_t>(
            static_cast<std::int16_t>(snapshot.skillLevel[index] / 100), 0, 9) + 1;
        break;
    }
    if (snapshot.learningItem >= 0) {
        snapshot.expForSkillLearn =
            ::hojy::world::state::getExpForSkillLearn(
                items, snapshot.learningItem, snapshot.learningLevel - 1,
                snapshot.potential);
    }
    return snapshot;
}

}

std::optional<CharacterStatusSnapshot>
buildCharacterStatusSnapshot(std::int16_t charId, bool simpleMode,
                             bool showPotential) {
    if (charId < 0 || static_cast<std::size_t>(charId)
            >= ::hojy::world::state::gSaveData.charInfo.size()) {
        return std::nullopt;
    }
    const auto *source = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!source) { return std::nullopt; }
    return buildSnapshot(*source, ::hojy::world::state::gSaveData.itemInfo,
                         simpleMode, showPotential);
}

std::optional<CharacterStatusSnapshot>
buildCharacterStatusSnapshot(
    const ::hojy::world::state::CharacterData &character,
    const ::hojy::world::state::ItemInfo &items,
    bool simpleMode, bool showPotential) {
    return buildSnapshot(character, items, simpleMode, showPotential);
}

}
