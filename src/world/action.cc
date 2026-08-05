/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "action.hh"

#include "battle/combat_rules.hh"
#include "battle/formulas.hh"
#include "battle/game_random.hh"
#include "battle/resource_items.hh"
#include "bag.hh"
#include "item_slots.hh"
#include "savedata.hh"
#include "strings.hh"
#include "content/factors.hh"
#include "util/random.hh"
#include <algorithm>
#include <cstring>
#include <optional>
#include <vector>

namespace hojy::world::state {

namespace {

battle::RandomSource &battleRandom() {
    static battle::GameRandom random;
    return random;
}

std::optional<battle::ResourceItemKind> resourceItemKind(PropType type) {
    switch (type) {
    case PropType::Hp:
        return battle::ResourceItemKind::Hp;
    case PropType::Mp:
        return battle::ResourceItemKind::Mp;
    case PropType::Stamina:
        return battle::ResourceItemKind::Stamina;
    case PropType::Poisoned:
        return battle::ResourceItemKind::Poisoned;
    default:
        return std::nullopt;
    }
}

}

const std::wstring &propToName(PropType type) {
    return GETTEXT(std::int16_t(type) + 1);
}

void addUpPropFromEquipToChar(const ItemInfo &items, CharacterData *info) {
    if (!info) { return; }
    for (auto id: info->equip) {
        if (id < 0) { continue; }
        const auto *itemInfo = items[id];
        if (!itemInfo) { continue; }
#define AddProp(M, N) info->M += itemInfo->add##N
        AddProp(attack, Attack);
        AddProp(speed, Speed);
        AddProp(defence, Defence);
        AddProp(medic, Medic);
        AddProp(poison, Poison);
        AddProp(depoison, Depoison);
        AddProp(antipoison, Antipoison);
        AddProp(fist, Fist);
        AddProp(sword, Sword);
        AddProp(blade, Blade);
        AddProp(special, Special);
        AddProp(throwing, Throwing);
        AddProp(knowledge, Knowledge);
        AddProp(poisonAmp, PoisonAmp);
#undef AddProp
    }
}

void addUpPropFromEquipToChar(CharacterData *info) {
    addUpPropFromEquipToChar(::hojy::world::state::gSaveData.itemInfo, info);
}

std::uint16_t getExpForLevelUp(std::int16_t level) {
    if (level <= 0
        || static_cast<size_t>(level) > ::hojy::content::gFactors.expForLevelUp.size()) {
        return 0;
    }
    return ::hojy::content::gFactors.expForLevelUp[static_cast<size_t>(level - 1)];
}

std::uint16_t getExpForSkillLearn(const ItemInfo &items,
                                  std::int16_t itemId, std::int16_t level,
                                  std::int16_t potential) {
    const auto *itemInfo = items[itemId];
    if (!itemInfo) { return 0; }
    const int tier = battle::potentialTier(potential);
    if (itemInfo->skillId < 0) {
        return std::uint16_t(std::clamp<int>(itemInfo->reqExp * tier * 2, 0, 65535));
    }
    if (level >= ::hojy::content::SkillLevelMaxDiv) { return 0; }
    return std::uint16_t(std::clamp<int>(itemInfo->reqExp * (level + 1) * tier, 0, 65535));
}

std::uint16_t getExpForSkillLearn(std::int16_t itemId, std::int16_t level,
                                  std::int16_t potential) {
    return getExpForSkillLearn(::hojy::world::state::gSaveData.itemInfo,
                               itemId, level, potential);
}

std::uint16_t getExpForMakeItem(std::int16_t itemId, std::int16_t potential) {
    const auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[itemId];
    if (!itemInfo || itemInfo->reqExpForMakeItem <= 0) { return 0; }
    return std::uint16_t(std::clamp<int>(
        itemInfo->reqExpForMakeItem * battle::potentialTier(potential), 0, 65535));
}

bool leaveTeam(SaveData &saveData, std::int16_t id) {
    if (id <= 0) { return false; }
    auto *charInfo = saveData.charInfo[id];
    if (!charInfo) { return false; }
    for (int i = 0; i < ::hojy::content::TeamMemberCount; ++i) {
        if (saveData.baseInfo->members[i] != id) { continue; }
        for (auto &eq: charInfo->equip) {
            if (eq >= 0) {
                auto *itemInfo = saveData.itemInfo[eq];
                if (itemInfo) { itemInfo->user = -1; }
                eq = -1;
            }
        }
        if (charInfo->learningItem >= 0) {
            auto *itemInfo = saveData.itemInfo[charInfo->learningItem];
            if (itemInfo) { itemInfo->user = -1; }
            charInfo->learningItem = -1;
        }
        if (i < ::hojy::content::TeamMemberCount - 1) {
            memmove(saveData.baseInfo->members + i,
                    saveData.baseInfo->members + i + 1,
                    sizeof(std::int16_t) * (::hojy::content::TeamMemberCount - i - 1));
        }
        saveData.baseInfo->members[::hojy::content::TeamMemberCount - 1] = -1;
        return true;
    }
    return false;
}

bool skillFull(const SaveData &saveData, std::int16_t charId) {
    if (charId < 0) { return true; }
    const auto *charInfo = saveData.charInfo[charId];
    if (!charInfo) { return true; }
    for (auto id: charInfo->skillId) {
        if (id <= 0) { return false; }
    }
    return true;
}

bool leaveTeam(std::int16_t id) {
    return leaveTeam(::hojy::world::state::gSaveData, id);
}

bool skillFull(std::int16_t charId) {
    return skillFull(::hojy::world::state::gSaveData, charId);
}

bool equipItem(SaveData &saveData, std::int16_t charId, std::int16_t itemId) {
    if (charId < 0) { return false; }
    auto *charInfo = saveData.charInfo[charId];
    if (!charInfo) { return false; }
    auto *itemInfo = saveData.itemInfo[itemId];
    if (!itemInfo) { return false; }
    switch (itemInfo->itemType) {
    case 1:
        if (itemInfo->equipType < 0 || itemInfo->equipType > 1) { return false; }
        break;
    case 2:
        break;
    default:
        return false;
    }
    if (!canUseItem(charInfo, itemInfo)) { return false; }
    if (itemInfo->user >= 0) {
        /* unequip from old char first */
        auto *charInfo2 = saveData.charInfo[itemInfo->user];
        if (charInfo2) {
            if (itemInfo->itemType == 1) {
                charInfo2->equip[itemInfo->equipType] = -1;
            } else {
                charInfo2->learningItem = -1;
            }
        }
    }
    if (itemInfo->itemType == 1) {
        itemInfo->user = charId;
        if (charInfo->equip[itemInfo->equipType] >= 0) {
            auto *itemInfo2 = saveData.itemInfo[charInfo->equip[itemInfo->equipType]];
            if (itemInfo2) { itemInfo2->user = -1; }
        }
        charInfo->equip[itemInfo->equipType] = itemId;
    } else {
        itemInfo->user = charId;
        if (charInfo->learningItem >= 0) {
            auto *itemInfo2 = saveData.itemInfo[charInfo->learningItem];
            if (itemInfo2) { itemInfo2->user = -1; }
        }
        charInfo->learningItem = itemId;
    }
    return true;
}

bool equipItem(std::int16_t charId, std::int16_t itemId) {
    return equipItem(::hojy::world::state::gSaveData, charId, itemId);
}

bool useItem(const ItemInfo &items, Bag &bag, CharacterData *charInfo,
             std::int16_t itemId,
             std::map<PropType, std::int16_t> &changes) {
    if (!charInfo) { return false; }
    const auto *itemInfo = items[itemId];
    if (!itemInfo) { return false; }
    if (bag[itemId] <= 0) { return false; }
    if (!canUseItem(charInfo, itemInfo)) { return false; }
    if (!applyItemChanges(charInfo, itemInfo, changes)) { return false; }
    (void)bag.remove(itemId, 1);
    return true;
}

bool useItem(Bag &bag, CharacterData *charInfo, std::int16_t itemId,
             std::map<PropType, std::int16_t> &changes) {
    return useItem(::hojy::world::state::gSaveData.itemInfo, bag, charInfo,
                   itemId, changes);
}

bool useItem(CharacterData *charInfo, std::int16_t itemId,
             std::map<PropType, std::int16_t> &changes) {
    return useItem(gBag, charInfo, itemId, changes);
}

std::int16_t tryUseBagItem(const Bag &bag, CharacterData *charInfo,
                          PropType type, std::int16_t value) {
    if (!charInfo) { return -1; }
    (void)value; // The DOS scan ignores the requested delta.
    std::vector<battle::ResourceItemOption> items;
    for (const auto &[itemId, count]: bag.orderedItems()) {
        if (itemId < 0 || count <= 0) { continue; }
        const auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[itemId];
        if (!itemInfo || itemInfo->itemType != 3) { continue; }
        items.push_back(battle::ResourceItemOption{
            itemId, itemInfo->addHp, itemInfo->addMp,
            itemInfo->addStamina, itemInfo->addPoisoned,
        });
    }
    const auto kind = resourceItemKind(type);
    if (!kind) { return -1; }
    const auto selected = battle::chooseFirstResourceItem(items, *kind);
    return selected ? static_cast<std::int16_t>(*selected) : -1;
}

std::int16_t tryUseBagItem(CharacterData *charInfo, PropType type, std::int16_t value) {
    return tryUseBagItem(gBag, charInfo, type, value);
}

bool useNpcItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes) {
    if (!charInfo) { return false; }
    auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return false; }
    if (!canUseItem(charInfo, itemInfo)) { return false; }
    for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
        if (charInfo->item[i] != itemId || charInfo->itemCount[i] <= 0) { continue; }
        if (!applyItemChanges(charInfo, itemInfo, changes)) { return false; }
        return consumeNpcItemAt(charInfo, i, itemId);
    }
    return false;
}

std::int16_t tryUseNpcItem(CharacterData *charInfo, PropType type, std::int16_t value) {
    if (!charInfo) { return -1; }
    (void)value; // The DOS scan ignores the requested delta.
    std::vector<battle::ResourceItemOption> items;
    for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
        auto itemId = charInfo->item[i];
        if (itemId < 0 || charInfo->itemCount[i] <= 0) { continue; }
        const auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[itemId];
        if (!itemInfo || itemInfo->itemType != 3) { continue; }
        items.push_back(battle::ResourceItemOption{
            itemId, itemInfo->addHp, itemInfo->addMp,
            itemInfo->addStamina, itemInfo->addPoisoned,
        });
    }
    const auto kind = resourceItemKind(type);
    if (!kind) { return -1; }
    const auto selected = battle::chooseFirstResourceItem(items, *kind);
    return selected ? static_cast<std::int16_t>(*selected) : -1;
}

bool applyItemChanges(CharacterData *charInfo, const ItemData *itemInfo, std::map<PropType, std::int16_t> &changes) {
#define ChangeProp(N, M) \
    if (itemInfo->add##M != 0) { \
        auto oldVal = charInfo->N; \
        charInfo->N = std::clamp<std::int16_t>(charInfo->N + itemInfo->add##M, 0, ::hojy::content::M##Max); \
        if (oldVal != charInfo->N) { changes[PropType::M] = charInfo->N - oldVal; } \
    }
#define ChangeProp2(N, M) \
    if (itemInfo->add##M != 0) { \
        auto oldVal = charInfo->N; \
        charInfo->N = std::clamp<std::int16_t>(charInfo->N + itemInfo->add##M, 0, charInfo->max##M); \
        if (oldVal != charInfo->N) { changes[PropType::M] = charInfo->N - oldVal; } \
    }
    ChangeProp2(hp, Hp)
    ChangeProp(maxHp, MaxHp)
    ChangeProp(poisoned, Poisoned)
    ChangeProp(stamina, Stamina)
    if (itemInfo->changeMpType > 0 && charInfo->mpType < 2 && charInfo->mpType != itemInfo->changeMpType) {
        charInfo->mpType = itemInfo->changeMpType;
        changes[PropType::MpType] = itemInfo->changeMpType;
    }
    ChangeProp2(mp, Mp)
    ChangeProp(maxMp, MaxMp)
    ChangeProp(attack, Attack)
    ChangeProp(speed, Speed)
    ChangeProp(defence, Defence)
    ChangeProp(medic, Medic)
    ChangeProp(poison, Poison)
    ChangeProp(depoison, Depoison)
    ChangeProp(antipoison, Antipoison)
    ChangeProp(fist, Fist)
    ChangeProp(sword, Sword)
    ChangeProp(blade, Blade)
    ChangeProp(special, Special)
    ChangeProp(throwing, Throwing)
    ChangeProp(knowledge, Knowledge)
    ChangeProp(integrity, Integrity)
    if (itemInfo->addDoubleAttack > 0 && charInfo->doubleAttack != itemInfo->addDoubleAttack) {
        charInfo->doubleAttack = itemInfo->addDoubleAttack;
        changes[PropType::DoubleAttack] = itemInfo->addDoubleAttack;
    }
    ChangeProp(poisonAmp, PoisonAmp)
#undef ChangeProp
#undef ChangeProp2
    return !changes.empty();
}

void applyBookChanges(CharacterData *charInfo, const ItemData *itemInfo) {
    if (!charInfo || !itemInfo) { return; }
    charInfo->maxHp = std::int16_t(charInfo->maxHp + itemInfo->addMaxHp);
    if (charInfo->maxHp > ::hojy::content::MaxHpMax) { charInfo->maxHp = ::hojy::content::MaxHpMax; }
    if (itemInfo->changeMpType == 2) { charInfo->mpType = 2; }
    charInfo->maxMp = std::int16_t(charInfo->maxMp + itemInfo->addMaxMp);
    if (charInfo->maxMp > ::hojy::content::MaxMpMax) { charInfo->maxMp = ::hojy::content::MaxMpMax; }
#define BookStat(N, M) \
    charInfo->N = std::int16_t(battle::applyBookStat(charInfo->N, itemInfo->add##M, ::hojy::content::M##Max))
    BookStat(attack, Attack);
    BookStat(speed, Speed);
    BookStat(defence, Defence);
    BookStat(medic, Medic);
    BookStat(poison, Poison);
    BookStat(depoison, Depoison);
    BookStat(antipoison, Antipoison);
    BookStat(fist, Fist);
    BookStat(sword, Sword);
    BookStat(blade, Blade);
    BookStat(special, Special);
    BookStat(throwing, Throwing);
    BookStat(knowledge, Knowledge);
    BookStat(integrity, Integrity);
#undef BookStat
    if (charInfo->doubleAttack == 0) { charInfo->doubleAttack = itemInfo->addDoubleAttack; }
    charInfo->poisonAmp = std::int16_t(battle::applyBookStat(
        charInfo->poisonAmp, itemInfo->addPoisonAmp, ::hojy::content::PoisonAmpMax));
}

bool canUseItem(const CharacterData *charInfo, const ItemData *itemInfo) {
    if (itemInfo->itemType == 1 || itemInfo->itemType == 2) {
        if (itemInfo->charOnly >= 0 && itemInfo->charOnly != charInfo->id) { return false; }
        if (itemInfo->reqMpType == 0 || itemInfo->reqMpType == 1) {
            if (charInfo->mpType < 2 && itemInfo->reqMpType != charInfo->mpType) { return false; }
        }
    }
    auto check = [](std::int16_t v, std::int16_t n)->bool {
        if (n < 0) {
            return v < -n;
        }
        return v >= n;
    };
    return check(charInfo->mp, itemInfo->reqMp)
        && check(charInfo->attack, itemInfo->reqAttack)
        && check(charInfo->speed, itemInfo->reqSpeed)
        && check(charInfo->poison, itemInfo->reqPoison)
        && check(charInfo->medic, itemInfo->reqMedic)
        && check(charInfo->depoison, itemInfo->reqDepoison)
        && check(charInfo->fist, itemInfo->reqFist)
        && check(charInfo->sword, itemInfo->reqSword)
        && check(charInfo->blade, itemInfo->reqBlade)
        && check(charInfo->special, itemInfo->reqSpecial)
        && check(charInfo->throwing, itemInfo->reqThrowing)
        && check(charInfo->potential, itemInfo->reqPotential);
}

std::int16_t getLeaveEventId(std::int16_t id) {
    for (size_t i = 0; i < ::hojy::content::gFactors.leaveTeamChars.size(); ++i) {
        if (::hojy::content::gFactors.leaveTeamChars[i] == id) {
            return ::hojy::content::gFactors.leaveTeamStartEvents + std::int16_t(i) * 2;
        }
    }
    return -1;
}

std::int16_t calcRealAttack(const CharacterData *c, std::int16_t knowledge, const SkillData *skill, std::int16_t level) {
    if (!c || !skill) { return 0; }
    int equipmentAttack = 0;
    for (auto eq: c->equip) {
        if (eq < 0) { continue; }
        const auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[eq];
        if (itemInfo) { equipmentAttack += itemInfo->addAttack; }
    }
    int skillWeaponBonus = 0;
    auto &bindings = ::hojy::content::gFactors.skillWeaponsBindings;
    for (size_t i = 0; i + 2 < bindings.size(); i += 3) {
        if (bindings[i + 1] == skill->id && bindings[i] == c->equip[0]) {
            skillWeaponBonus = bindings[i + 2];
            break;
        }
    }
    return battle::calcRealAttack(*c, knowledge, *skill, level,
                                  static_cast<std::int16_t>(equipmentAttack),
                                  static_cast<std::int16_t>(skillWeaponBonus));
}

std::int16_t calcRealDefense(const CharacterData *c, std::int16_t knowledge) {
    return c ? battle::calcRealDefense(*c, knowledge) : 0;
}

std::int16_t calcPredictDamage(std::int16_t atk, std::int16_t def, std::int16_t stamina, std::int16_t hurt, std::int16_t distance) {
    return battle::calcPredictDamage(atk, def, stamina, hurt, distance);
}

std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currMp) {
    return battle::calcRealSkillLevel(reqMp, level, currMp);
}

std::int16_t calcSkillMpCost(const SkillData *skill, std::int16_t level) {
    return skill ? std::int16_t(battle::skillMpCost(skill->reqMp, level)) : 0;
}

bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledgeSelf,
               std::int16_t knowledgeOther, int distance, int index, int level,
               std::int16_t &damage, std::int16_t &poisoned,
               std::int16_t &exp, bool &dead) {
    battle::GameRandom random;
    return actDamage(c1, c2, knowledgeSelf, knowledgeOther, distance, index,
                     level, damage, poisoned, exp, dead, random);
}

bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledgeSelf,
               std::int16_t knowledgeOther, int distance, int index, int level,
               std::int16_t &damage, std::int16_t &poisoned,
               std::int16_t &exp, bool &dead,
               battle::RandomSource &random) {
    if (!c1 || !c2) { return false; }
    index = std::clamp(index, 0, ::hojy::content::LearnSkillCount - 1);
    auto skillId = c1->skillId[index];
    const auto *skill = ::hojy::world::state::gSaveData.skillInfo[skillId];
    if (!skill) { return false; }
    exp = 0;
    poisoned = 0;
    int equipmentAttack = 0;
    for (auto eq: c1->equip) {
        if (eq < 0) { continue; }
        const auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[eq];
        if (itemInfo) { equipmentAttack += itemInfo->addAttack; }
    }
    int skillWeaponBonus = 0;
    auto &bindings = ::hojy::content::gFactors.skillWeaponsBindings;
    for (size_t i = 0; i + 2 < bindings.size(); i += 3) {
        if (bindings[i + 1] == skill->id && bindings[i] == c1->equip[0]) {
            skillWeaponBonus = bindings[i + 2];
            break;
        }
    }
    const auto oldHp = c2->hp;
    const auto result = battle::applyDamage(*c1, *c2, knowledgeSelf, knowledgeOther, distance,
                                            *skill, level, random,
                                            static_cast<std::int16_t>(equipmentAttack),
                                            static_cast<std::int16_t>(skillWeaponBonus),
                                            c1->skillLevel[index]);
    damage = result.damage;
    poisoned = result.poisoned;
    dead = result.dead;
    if (result.applied && skill->damageType == 0) {
        exp = static_cast<std::int16_t>(result.damage / 5);
        if (result.damage > oldHp) {
            exp = static_cast<std::int16_t>(exp + c2->level * 10);
        }
    }
    return result.applied;
}

void postDamage(CharacterData *c, int index, int level, std::int16_t stamina, bool &levelup) {
    battle::GameRandom random;
    postDamage(c, index, level, stamina, levelup, random);
}

void postDamage(CharacterData *c, int index, int level, std::int16_t stamina,
                bool &levelup, battle::RandomSource &random) {
    if (!c) { return; }
    index = std::clamp(index, 0, ::hojy::content::LearnSkillCount - 1);
    const int oldLevel = c->skillLevel[index] / 100;
    c->skillLevel[index] = static_cast<std::int16_t>(
        c->skillLevel[index] + random.next(1, 2));
    if (c->skillLevel[index] > ::hojy::content::SkillLevelStoreMax) {
        c->skillLevel[index] = ::hojy::content::SkillLevelStoreMax;
    }
    levelup = c->skillLevel[index] / 100 != oldLevel;
    const auto *skill = ::hojy::world::state::gSaveData.skillInfo[c->skillId[index]];
    if (skill) {
        c->mp = static_cast<std::int16_t>(
            std::max(0, static_cast<int>(c->mp) - battle::skillMpCost(skill->reqMp, level)));
    }
    if (stamina) {
        c->stamina = static_cast<std::int16_t>(
            std::max(0, static_cast<int>(c->stamina) - stamina));
    }
}

std::int16_t actPoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    return battle::applyPoison(*c1, *c2, stamina);
}

std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    battle::GameRandom random;
    return actMedic(c1, c2, stamina, random);
}

std::int16_t actMedic(CharacterData *c1, CharacterData *c2,
                      std::int16_t stamina, battle::RandomSource &random) {
    if (!c1 || !c2) { return 0; }
    return battle::applyMedic(*c1, *c2, stamina, random);
}

std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    battle::GameRandom random;
    return actDepoison(c1, c2, stamina, random);
}

std::int16_t actDepoison(CharacterData *c1, CharacterData *c2,
                         std::int16_t stamina, battle::RandomSource &random) {
    if (!c1 || !c2) { return 0; }
    return battle::applyDepoison(*c1, *c2, stamina, random);
}

std::int16_t actThrow(CharacterData *c1, CharacterData *c2, std::int16_t itemId, std::int16_t stamina, bool &dead) {
    battle::GameRandom random;
    return actThrow(c1, c2, itemId, stamina, dead, random);
}

std::int16_t actThrow(CharacterData *c1, CharacterData *c2,
                      std::int16_t itemId, std::int16_t stamina, bool &dead,
                      battle::RandomSource &random) {
    if (!c1 || !c2) { return 0; }
    const auto *itemInfo = ::hojy::world::state::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return 0; }
    return battle::applyThrow(*c1, *c2, *itemInfo, stamina, random, dead);
}

std::int16_t actRoundEndDrain(CharacterData *c, bool inactive) {
    if (!c || c->hp <= 0) { return 0; }
    if (c->hurt <= 0 && (c->poisoned <= 0 || c->stamina <= 0 || inactive)) { return 0; }
    if (c->stamina < 0) { c->stamina = 1; }
    return battle::applyRoundEndDamage(*c);
}

void actRest(CharacterData *c, bool moved) {
    actRest(c, moved, battleRandom());
}

void actRest(CharacterData *c, bool moved, battle::RandomSource &random) {
    if (!c) { return; }
    const auto gain = battle::restGain(c->stamina, moved, random);
    c->stamina = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->stamina + gain.stamina), 0, ::hojy::content::StaminaMax);
    if (gain.hp) {
        c->hp = std::min<std::int16_t>(
            static_cast<std::int16_t>(c->hp + gain.hp), c->maxHp);
    }
    if (gain.mp) {
        c->mp = std::min<std::int16_t>(
            static_cast<std::int16_t>(c->mp + gain.mp), c->maxMp);
    }
}

void actLevelup(CharacterData *c, int gainedLevels) {
    actLevelup(c, gainedLevels, battleRandom());
}

void actLevelup(CharacterData *c, int gainedLevels,
                battle::RandomSource &random) {
    if (!c || gainedLevels <= 0) { return; }
    const auto factor = battle::levelUpFactor(c->potential, random);
    const auto gain = battle::levelUpGain(
        gainedLevels, c->hpAddOnLevelUp, factor, random);
    c->level = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->level + gainedLevels), 0, ::hojy::content::LevelMax);
    c->maxHp = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->maxHp + gain.maxHp), 0, ::hojy::content::MaxHpMax);
    c->hp = c->maxHp;
    c->hurt = 0;
    c->poisoned = 0;
    c->stamina = ::hojy::content::StaminaMax;
    c->maxMp = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->maxMp + gain.maxMp), 0, ::hojy::content::MaxMpMax);
    c->mp = c->maxMp;
    c->attack = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->attack + gain.stat), 0, ::hojy::content::AttackMax);
    c->speed = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->speed + gain.stat), 0, ::hojy::content::SpeedMax);
    c->defence = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->defence + gain.stat), 0, ::hojy::content::DefenceMax);
#define GrowProficiency(N, M) \
    if (c->N > 20) { \
        c->N = std::clamp<std::int16_t>( \
            static_cast<std::int16_t>(c->N + battle::originalRandom(random, 3)), \
            0, ::hojy::content::M); \
    }
    GrowProficiency(medic, MedicMax)
    GrowProficiency(poison, PoisonMax)
    GrowProficiency(depoison, DepoisonMax)
    GrowProficiency(fist, FistMax)
    GrowProficiency(sword, SwordMax)
    GrowProficiency(blade, BladeMax)
#undef GrowProficiency
    c->throwing = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(c->throwing + battle::originalRandom(random, 3)),
        0, ::hojy::content::ThrowingMax);
}

}
