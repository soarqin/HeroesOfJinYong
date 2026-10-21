#include "mapwithevent.hh"

#include "window_command.hh"
#include "mask.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "world/strings.hh"
#include "util/random.hh"

#include <fmt/xchar.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {
namespace {

bool validCharacterId(std::int16_t id) {
    return id >= 0 && static_cast<std::size_t>(id)
        < ::hojy::world::state::gSaveData.charInfo.size()
        && ::hojy::world::state::gSaveData.charInfo[id] != nullptr;
}

bool validSubMapId(std::int16_t id) {
    if (id < 0) { return false; }
    const auto index = static_cast<std::size_t>(id);
    const auto &save = ::hojy::world::state::gSaveData;
    return index < save.subMapInfo.size() && save.subMapInfo[index]
        && index < save.subMapLayerInfo.size();
}

}

bool MapWithEvent::learnSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillId, std::int16_t quiet) {
    if (!validCharacterId(charId) || skillId < 0
        || static_cast<std::size_t>(skillId)
            >= ::hojy::world::state::gSaveData.skillInfo.size()) {
        return true;
    }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    auto *skillInfo = ::hojy::world::state::gSaveData.skillInfo[skillId];
    if (!skillInfo) { return true; }

    int found = -1;
    auto learnId = skillInfo->id;
    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        auto thisId = charInfo->skillId[i];
        if (thisId == skillInfo->id) {
            if (charInfo->skillLevel[i] < ::hojy::content::SkillLevelMaxDiv * 100) {
                charInfo->skillLevel[i] += 100;
            }
            found = -1;
            break;
        }
        if (thisId < 0) {
            if (found < 0) {
                found = i;
            }
        }
    }
    if (found >= 0) {
        charInfo->skillId[found] = learnId;
        charInfo->skillLevel[found] = 0;
    }
    if (quiet) {
        return true;
    }
    postSceneCommand(map, [charId, skillId](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {} {}", GETCHARNAME(charId), GETTEXT(75), GETSKILLNAME(skillId))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::addPotential(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacterId(charId)) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->potential = std::clamp<std::int16_t>(charInfo->potential + value, 0, ::hojy::content::PotentialMax);
    postSceneCommand(map, [charId, value](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(29), GETTEXT(33), std::to_wstring(value))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::setSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillIndex,
                            std::int16_t skillId, std::int16_t level) {
    if (!validCharacterId(charId) || skillIndex < 0
        || skillIndex >= ::hojy::content::LearnSkillCount
        || skillId < -1
        || level < 0 || level > ::hojy::content::SkillLevelStoreMax
        || (skillId >= 0 && static_cast<std::size_t>(skillId)
            >= ::hojy::world::state::gSaveData.skillInfo.size())) {
        return true;
    }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->skillId[skillIndex] = skillId;
    charInfo->skillLevel[skillIndex] = level;
    return true;
}

int MapWithEvent::checkSex(MapWithEvent *map, std::int16_t sex) {
    if (sex < 256) {
        auto *charInfo = ::hojy::world::state::gSaveData.charInfo[0];
        if (!charInfo) { return 0; }
        return charInfo->sex == sex ? 1 : 0;
    }
    /* event 36 for extended functions */
    std::int16_t result = 0;
    if (!map->eventVm_.memory().readWord(0x7000, result)) { return 0; }
    return result == 0 ? 1 : 0;
}

bool MapWithEvent::addIntegrity(MapWithEvent *map, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    charInfo->integrity = std::clamp<std::int16_t>(charInfo->integrity + value, 0, ::hojy::content::IntegrityMax);
    return true;
}

bool MapWithEvent::modifySubMapLayerTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer,
                                        std::int16_t oldTex, std::int16_t newTex) {
    if (subMapId < 0) {
        subMapId = map->subMapId_;
    }
    if (!validSubMapId(subMapId) || layer < 0
        || layer >= ::hojy::content::SubMapLayerCount
        || oldTex < -1 || newTex < -1) {
        return true;
    }
    if (!map || static_cast<std::size_t>(subMapId)
            >= ::hojy::world::state::gSaveData.subMapLayerInfo.size()) {
        return true;
    }
    auto *layerInfo = ::hojy::world::state::gSaveData
        .subMapLayerInfo[subMapId].operator->();
    if (!layerInfo) { return true; }
    std::vector<std::int16_t> candidateLayer(
        layerInfo->data[layer],
        layerInfo->data[layer] + ::hojy::content::SubMapWidth
            * ::hojy::content::SubMapHeight);
    std::vector<std::size_t> changed;
    const bool currentMap = subMapId == map->subMapId_;
    changed.reserve(candidateLayer.size());
    for (int y = 0; y < ::hojy::content::SubMapHeight; ++y) {
        for (int x = 0; x < ::hojy::content::SubMapWidth; ++x) {
            const auto pos = static_cast<std::size_t>(y)
                * ::hojy::content::SubMapWidth + static_cast<std::size_t>(x);
            if (candidateLayer[pos] == oldTex) {
                if (newTex < -1) { return true; }
                if (currentMap && newTex >= 0 && !map->texData(newTex / 2).size()) {
                    return true;
                }
                candidateLayer[pos] = newTex;
                changed.push_back(pos);
            }
        }
    }
    if (changed.empty()) { return true; }
    logic::SubMapStateSnapshot snapshot;
    if (layer == 3) {
        if (static_cast<std::size_t>(subMapId)
                >= ::hojy::world::state::gSaveData.subMapEventInfo.size()) {
            return true;
        }
        auto *eventInfo = ::hojy::world::state::gSaveData
            .subMapEventInfo[subMapId].operator->();
        if (!eventInfo) { return true; }
        std::vector<std::int16_t> eventLayer(
            layerInfo->data[3],
            layerInfo->data[3] + ::hojy::content::SubMapWidth
                * ::hojy::content::SubMapHeight);
        eventLayer = candidateLayer;
        if (!map->validateSubMapStateCandidate(
                subMapId, *eventInfo, eventLayer, snapshot)) {
            return true;
        }
    }
    std::copy(candidateLayer.begin(), candidateLayer.end(), layerInfo->data[layer]);
    if (layer == 3) {
        map->synchronizeCommittedSubMapState(subMapId, snapshot);
        return true;
    }
    if (currentMap) {
        for (const auto pos: changed) {
            const auto x = static_cast<int>(pos % ::hojy::content::SubMapWidth);
            const auto y = static_cast<int>(pos / ::hojy::content::SubMapWidth);
            map->setCellSpriteId(
                x, y, layer,
                candidateLayer[pos] < 0 ? -1 : candidateLayer[pos] / 2);
        }
    }
    return true;
}

bool MapWithEvent::openSubMap(MapWithEvent *, std::int16_t subMapId) {
    if (!validSubMapId(subMapId)) { return true; }
    ::hojy::world::state::gSaveData.subMapInfo[subMapId]->enterCondition = 0;
    return true;
}

bool MapWithEvent::forceDirection(MapWithEvent *map, std::int16_t direction) {
    if (direction < Map::DirUp || direction > Map::DirDown) { return true; }
    map->setDirection(Direction(direction));
    map->updateMainCharSpriteId();
    return true;
}

bool MapWithEvent::addItemToChar(MapWithEvent *map, std::int16_t charId, std::int16_t itemId, std::int16_t itemCount) {
    if (!validCharacterId(charId) || itemId < 0 || itemCount < 0
        || static_cast<std::size_t>(itemId)
            >= ::hojy::world::state::gSaveData.itemInfo.size()) {
        return true;
    }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    int firstEmpty = -1;
    for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
        if (charInfo->item[i] < 0) {
            firstEmpty = i;
            continue;
        }
        if (charInfo->item[i] == itemId) {
            const auto total = static_cast<int>(charInfo->itemCount[i])
                + static_cast<int>(itemCount);
            if (total > std::numeric_limits<std::int16_t>::max()) {
                return true;
            }
            charInfo->itemCount[i] = static_cast<std::int16_t>(total);
            return true;
        }
    }
    if (firstEmpty < 0) {
        return true;
    }
    charInfo->item[firstEmpty] = itemId;
    charInfo->itemCount[firstEmpty] = itemCount;
    return true;
}

int MapWithEvent::checkFemaleInTeam(MapWithEvent *map) {
    for (auto id: ::hojy::world::state::gSaveData.baseInfo->members) {
        if (id < 0) { continue; }
        auto *charInfo = ::hojy::world::state::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        if (charInfo->sex == 1) {
            return 1;
        }
    }
    return 0;
}

bool MapWithEvent::animation2(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2, std::int16_t endTex2) {
    if (map->subMapId_ < 0 || eventId < 0
        || eventId >= ::hojy::content::SubMapEventCount
        || eventId2 < 0 || eventId2 >= ::hojy::content::SubMapEventCount) {
        return true;
    }
    map->animEventId_[0] = eventId;
    map->animCurrTex_[0] = begTex;
    map->animEndTex_[0] = endTex;
    map->animEventId_[1] = eventId2;
    map->animCurrTex_[1] = begTex2;
    map->animEndTex_[1] = endTex2;
    return false;
}

bool MapWithEvent::animation3(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2,
                              std::int16_t eventId3, std::int16_t begTex3) {
    if (map->subMapId_ < 0 || eventId < 0
        || eventId >= ::hojy::content::SubMapEventCount
        || eventId2 < 0 || eventId2 >= ::hojy::content::SubMapEventCount
        || eventId3 < 0 || eventId3 >= ::hojy::content::SubMapEventCount) {
        return true;
    }
    std::int16_t endTex2 = 0;
    std::int16_t endTex3 = 0;
    if (!logic::translateSubMapAnimationEnd(
            begTex, endTex, begTex2, endTex2)
        || !logic::translateSubMapAnimationEnd(
            begTex, endTex, begTex3, endTex3)) {
        return true;
    }
    map->animEventId_[0] = eventId;
    map->animCurrTex_[0] = begTex;
    map->animEndTex_[0] = endTex;
    map->animEventId_[1] = eventId2;
    map->animCurrTex_[1] = begTex2;
    map->animEndTex_[1] = endTex2;
    map->animEventId_[2] = eventId3;
    map->animCurrTex_[2] = begTex3;
    map->animEndTex_[2] = endTex3;
    return false;
}

bool MapWithEvent::addSpeed(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacterId(charId)) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->speed = std::clamp<std::int16_t>(charInfo->speed + value, 0, ::hojy::content::SpeedMax);
    postSceneCommand(map, [charId, value](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(9), GETTEXT(33), std::to_wstring(value))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::addMaxMP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacterId(charId)) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->maxMp = std::clamp<std::int16_t>(charInfo->maxMp + value, 0, ::hojy::content::MpMax);
    postSceneCommand(map, [charId, value](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(7), GETTEXT(33), std::to_wstring(value))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::addAttack(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacterId(charId)) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->attack = std::clamp<std::int16_t>(charInfo->attack + value, 0, ::hojy::content::AttackMax);
    postSceneCommand(map, [charId, value](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(8), GETTEXT(33), std::to_wstring(value))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::addMaxHP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacterId(charId)) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->maxHp = std::clamp<std::int16_t>(charInfo->maxHp + value, 0, ::hojy::content::HpMax);
    postSceneCommand(map, [charId, value](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(2), GETTEXT(33), std::to_wstring(value))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::setMPType(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacterId(charId) || value < 0 || value > 2) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->mpType = value;
    return true;
}

int MapWithEvent::checkHas5Item(MapWithEvent *map, std::int16_t itemId0, std::int16_t itemId1, std::int16_t itemId2,
                                std::int16_t itemId3, std::int16_t itemId4) {
    return ::hojy::world::state::gBag[itemId0] > 0
        && ::hojy::world::state::gBag[itemId1] > 0
        && ::hojy::world::state::gBag[itemId2] > 0
        && ::hojy::world::state::gBag[itemId3] > 0
        && ::hojy::world::state::gBag[itemId4] > 0 ? 1 : 0;
}

}
