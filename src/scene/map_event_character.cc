#include "mapwithevent.hh"

#include "window.hh"
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
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {
bool MapWithEvent::learnSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillId, std::int16_t quiet) {
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
    gWindow->popupMessageBox({fmt::format(L"{} {} {}", GETCHARNAME(charId), GETTEXT(75), GETSKILLNAME(skillId))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addPotential(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->potential = std::clamp<std::int16_t>(charInfo->potential + value, 0, ::hojy::content::PotentialMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(29), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::setSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillIndex,
                            std::int16_t skillId, std::int16_t level) {
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
    auto &l = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId]->data[layer];
    auto pos = 0;
    bool currentMap = subMapId == map->subMapId_;
    for (int y = 0; y < ::hojy::content::SubMapHeight; ++y) {
        for (int x = 0; x < ::hojy::content::SubMapWidth; ++x) {
            if (l[pos] == oldTex) {
                l[pos] = newTex;
                if (currentMap) {
                    map->setCellTexture(x, y, layer, newTex >> 1);
                }
            }
            ++pos;
        }
    }
    if (currentMap) { map->drawDirty_ = true; }
    return true;
}

bool MapWithEvent::openSubMap(MapWithEvent *, std::int16_t subMapId) {
    ::hojy::world::state::gSaveData.subMapInfo[subMapId]->enterCondition = 0;
    return true;
}

bool MapWithEvent::forceDirection(MapWithEvent *map, std::int16_t direction) {
    map->setDirection(Direction(direction));
    map->updateMainCharTexture();
    return true;
}

bool MapWithEvent::addItemToChar(MapWithEvent *map, std::int16_t charId, std::int16_t itemId, std::int16_t itemCount) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    int firstEmpty = -1;
    for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
        if (charInfo->item[i] < 0) {
            firstEmpty = i;
            continue;
        }
        if (charInfo->item[i] == itemId) {
            charInfo->itemCount[i] += itemCount;
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
    if (map->subMapId_ < 0) { return true; }
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
    if (map->subMapId_ < 0) { return true; }
    map->animEventId_[0] = eventId;
    map->animCurrTex_[0] = begTex;
    map->animEndTex_[0] = endTex;
    map->animEventId_[1] = eventId2;
    map->animCurrTex_[1] = begTex2;
    map->animEndTex_[1] = begTex2 + (endTex - begTex);
    map->animEventId_[1] = eventId3;
    map->animCurrTex_[1] = begTex3;
    map->animEndTex_[1] = begTex3 + (endTex - begTex);
    return false;
}

bool MapWithEvent::addSpeed(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->speed = std::clamp<std::int16_t>(charInfo->speed + value, 0, ::hojy::content::SpeedMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(9), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addMaxMP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->maxMp = std::clamp<std::int16_t>(charInfo->maxMp + value, 0, ::hojy::content::MpMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(7), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addAttack(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->attack = std::clamp<std::int16_t>(charInfo->attack + value, 0, ::hojy::content::AttackMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(8), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addMaxHP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->maxHp = std::clamp<std::int16_t>(charInfo->maxHp + value, 0, ::hojy::content::HpMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(2), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::setMPType(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
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
