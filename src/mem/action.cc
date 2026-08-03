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

#include "savedata.hh"
#include "strings.hh"
#include "battle/formulas.hh"
#include "battle/game_random.hh"
#include "data/factors.hh"
#include "util/random.hh"
#include <algorithm>
#include <cstring>

namespace hojy::mem {

namespace {

/* Shared adapter so the pure battle rules use the game random source. */
battle::RandomSource &battleRandom() {
    static battle::GameRandom random;
    return random;
}

}

const std::wstring &propToName(PropType type) {
    return GETTEXT(std::int16_t(type) + 1);
}

void addUpPropFromEquipToChar(CharacterData *info) {
    for (auto id: info->equip) {
        if (id < 0) { continue; }
        const auto *itemInfo = mem::gSaveData.itemInfo[id];
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

std::uint16_t getExpForLevelUp(std::int16_t level) {
    --level;
    if (level >= data::gFactors.expForLevelUp.size()) { return 0;}
    return data::gFactors.expForLevelUp[level];
}

std::uint16_t getExpForSkillLearn(std::int16_t itemId, std::int16_t level, std::int16_t potential) {
    const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return 0; }
    /*
     * `WAR-TRAIN` Z.DAT:0x3BAB9. The multiplier is the potential tier, which is
     * not clamped, and a book that teaches no skill costs twice the base instead
     * of scaling with a level.
     */
    const int tier = battle::potentialTier(potential);
    if (itemInfo->skillId < 0) {
        return std::uint16_t(std::clamp<int>(itemInfo->reqExp * tier * 2, 0, 65535));
    }
    if (level >= data::SkillLevelMaxDiv) { return 0; }
    return std::uint16_t(std::clamp<int>(itemInfo->reqExp * (level + 1) * tier, 0, 65535));
}

std::uint16_t getExpForMakeItem(std::int16_t itemId, std::int16_t potential) {
    const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo || itemInfo->reqExpForMakeItem <= 0) { return 0; }
    /* `WAR-CRAFT` Z.DAT:0x3C2E1: the same potential tier scales the requirement. */
    const int tier = battle::potentialTier(potential);
    return std::uint16_t(std::clamp<int>(itemInfo->reqExpForMakeItem * tier, 0, 65535));
}

bool leaveTeam(std::int16_t id) {
    if (id <= 0) { return false; }
    auto *charInfo = mem::gSaveData.charInfo[id];
    if (!charInfo) { return false; }
    for (int i = 0; i < data::TeamMemberCount; ++i) {
        if (mem::gSaveData.baseInfo->members[i] != id) { continue; }
        for (auto &eq: charInfo->equip) {
            if (eq >= 0) {
                auto *itemInfo = mem::gSaveData.itemInfo[eq];
                if (itemInfo) { itemInfo->user = -1; }
                eq = -1;
            }
        }
        if (charInfo->learningItem >= 0) {
            auto *itemInfo = mem::gSaveData.itemInfo[charInfo->learningItem];
            if (itemInfo) { itemInfo->user = -1; }
            charInfo->learningItem = -1;
        }
        if (i < data::TeamMemberCount - 1) {
            memmove(mem::gSaveData.baseInfo->members + i,
                    mem::gSaveData.baseInfo->members + i + 1,
                    sizeof(std::int16_t) * (data::TeamMemberCount - i - 1));
        }
        mem::gSaveData.baseInfo->members[data::TeamMemberCount - 1] = -1;
        return true;
    }
    return false;
}

bool skillFull(std::int16_t charId) {
    if (charId < 0) { return true; }
    const auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    for (auto id: charInfo->skillId) {
        if (id <= 0) { return false; }
    }
    return true;
}

bool equipItem(std::int16_t charId, std::int16_t itemId) {
    if (charId < 0) { return false; }
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return false; }
    auto *itemInfo = mem::gSaveData.itemInfo[itemId];
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
        auto *charInfo2 = mem::gSaveData.charInfo[itemInfo->user];
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
            auto *itemInfo2 = mem::gSaveData.itemInfo[charInfo->equip[itemInfo->equipType]];
            if (itemInfo2) { itemInfo2->user = -1; }
        }
        charInfo->equip[itemInfo->equipType] = itemId;
    } else {
        itemInfo->user = charId;
        if (charInfo->learningItem >= 0) {
            auto *itemInfo2 = mem::gSaveData.itemInfo[charInfo->learningItem];
            if (itemInfo2) { itemInfo2->user = -1; }
        }
        charInfo->learningItem = itemId;
    }
    return true;
}

bool useItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes) {
    if (!charInfo) { return false; }
    auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return false; }
    if (!canUseItem(charInfo, itemInfo)) { return false; }
    if (!applyItemChanges(charInfo, itemInfo, changes)) { return false; }
    gBag.remove(itemId, 1);
    return true;
}

std::int16_t tryUseBagItem(CharacterData *charInfo, PropType type, std::int16_t value) {
    if (!charInfo) { return -1; }
    std::multimap<std::int16_t, std::int16_t> optionalItems;
    for (auto p: gBag.items()) {
        auto itemId = p.first;
        if (itemId < 0 || p.second <= 0) { continue; }
        const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
        if (!itemInfo) { continue; }
        if (itemInfo->itemType != 3) { continue; }
        switch (type) {
        case PropType::Hp:
            if (itemInfo->addHp <= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addHp - value), itemId);
            break;
        case PropType::Mp:
            if (itemInfo->addMp <= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addMp - value), itemId);
            break;
        case PropType::Stamina:
            if (itemInfo->addStamina <= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addStamina - value), itemId);
            break;
        case PropType::Poisoned:
            if (itemInfo->addPoisoned >= 0) { continue; }
            optionalItems.emplace(std::abs(-itemInfo->addPoisoned - value), itemId);
            break;
        default:
            break;
        }
    }
    if (optionalItems.empty()) {
        return -1;
    }
    if (optionalItems.size() > 1) {
        auto ite = optionalItems.begin();
        auto p1 = *ite;
        auto p2 = *(++ite);
        if (p1.first * 100 / p2.first >= 80) {
            return util::gRandom(2) ? p1.second : p2.second;
        }
    }
    return optionalItems.begin()->second;
}

bool useNpcItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes) {
    if (!charInfo) { return false; }
    auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return false; }
    if (!canUseItem(charInfo, itemInfo)) { return false; }
    for (int i = 0; i < data::CarryItemCount; ++i) {
        if (charInfo->item[i] != itemId || charInfo->itemCount[i] <= 0) { continue; }
        if (!applyItemChanges(charInfo, itemInfo, changes)) { return false; }
        return consumeNpcItem(charInfo, itemId);
    }
    return false;
}

std::int16_t tryUseNpcItem(CharacterData *charInfo, PropType type, std::int16_t value) {
    if (!charInfo) { return -1; }
    std::multimap<std::int16_t, std::int16_t> optionalItems;
    for (int i = 0; i < data::CarryItemCount; ++i) {
        auto itemId = charInfo->item[i];
        if (itemId < 0 || charInfo->itemCount[i] <= 0) { continue; }
        const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
        if (!itemInfo) { continue; }
        if (itemInfo->itemType != 3) { continue; }
        switch (type) {
        case PropType::Hp:
            if (itemInfo->addHp <= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addHp - value), itemId);
            break;
        case PropType::Mp:
            if (itemInfo->addMp <= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addHp - value), itemId);
            break;
        case PropType::Stamina:
            if (itemInfo->addStamina <= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addHp - value), itemId);
            break;
        case PropType::Poisoned:
            if (itemInfo->addPoisoned >= 0) { continue; }
            optionalItems.emplace(std::abs(itemInfo->addHp - value), itemId);
            break;
        default:
            break;
        }
    }
    if (optionalItems.empty()) {
        return -1;
    }
    if (optionalItems.size() > 1) {
        auto ite = optionalItems.begin();
        auto p1 = *ite;
        auto p2 = *(++ite);
        if (p1.first * 100 / p2.first >= 80) {
            return util::gRandom(2) ? p1.second : p2.second;
        }
    }
    return optionalItems.begin()->second;
}

bool applyItemChanges(CharacterData *charInfo, const ItemData *itemInfo, std::map<PropType, std::int16_t> &changes) {
#define ChangeProp(N, M) \
    if (itemInfo->add##M != 0) { \
        auto oldVal = charInfo->N; \
        charInfo->N = std::clamp<std::int16_t>(charInfo->N + itemInfo->add##M, 0, data::M##Max); \
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
    /*
     * `WAR-TRAIN` Z.DAT:0x3BC6C. A completed skill book grants a fixed set of
     * nineteen properties, in this order. It deliberately leaves the consumable
     * fields alone: hp, mp, poisoned and stamina are never touched here.
     */
    charInfo->maxHp = std::int16_t(charInfo->maxHp + itemInfo->addMaxHp);
    if (charInfo->maxHp > data::MaxHpMax) { charInfo->maxHp = data::MaxHpMax; }
    /* Only the switch to the third mp type is honoured (Z.DAT:0x3BC94). */
    if (itemInfo->changeMpType == 2) { charInfo->mpType = 2; }
    charInfo->maxMp = std::int16_t(charInfo->maxMp + itemInfo->addMaxMp);
    if (charInfo->maxMp > data::MaxMpMax) { charInfo->maxMp = data::MaxMpMax; }
#define BookStat(N, M) \
    charInfo->N = std::int16_t(battle::applyBookStat(charInfo->N, itemInfo->add##M, data::M##Max))
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
    /* Assigned, not added, and only while the character has none (Z.DAT:0x3C0B5). */
    if (charInfo->doubleAttack == 0) { charInfo->doubleAttack = itemInfo->addDoubleAttack; }
    charInfo->poisonAmp = std::int16_t(battle::applyBookStat(charInfo->poisonAmp,
                                                            itemInfo->addPoisonAmp,
                                                            data::PoisonAmpMax));
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
    for (size_t i = 0; i < data::gFactors.leaveTeamChars.size(); ++i) {
        if (data::gFactors.leaveTeamChars[i] == id) {
            return data::gFactors.leaveTeamStartEvents + std::int16_t(i) * 2;
        }
    }
    return -1;
}

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> calcColorForMpType(std::int16_t type) {
    switch (type) {
    case 0:
        return std::make_tuple(208, 152, 208);
    case 1:
        return std::make_tuple(236, 200, 40);
    default:
        break;
    }
    return std::make_tuple(252, 252, 252);
}

std::int16_t calcRealAttack(const CharacterData *c, std::int16_t knowledge, const SkillData *skill, std::int16_t level) {
    int atk = c->attack;
    int eqatk = 0;
    for (auto &eq: c->equip) {
        if (eq < 0) { continue; }
        const auto *itemInfo = mem::gSaveData.itemInfo[eq];
        if (!itemInfo) { continue; }
        atk -= itemInfo->addAttack;
        eqatk += itemInfo->addAttack;
    }
    auto &swBindings = data::gFactors.skillWeaponsBindings;
    for (size_t i = 0; i < swBindings.size(); i+=3) {
        if (swBindings[i + 1] == skill->id && swBindings[i] == c->equip[0]) {
            eqatk += swBindings[i + 2];
            break;
        }
    }
    return (atk * 3 + skill->damage[level]) / 2 + eqatk + knowledge * 2;
}

std::int16_t calcRealDefense(const CharacterData *c, std::int16_t knowledge) {
    return c->defence + knowledge * 2;
}

std::int16_t calcPredictDamage(std::int16_t atk, std::int16_t def, std::int16_t stamina, std::int16_t hurt, std::int16_t distance) {
    return std::int16_t(battle::predictDamage(battle::DamageInput{atk, def, stamina, hurt, distance}));
}

std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currMp) {
    return std::int16_t(battle::resolveSkillLevel(reqMp, level, currMp));
}

std::int16_t calcSkillMpCost(const SkillData *skill, std::int16_t level) {
    if (!skill) { return 0; }
    return std::int16_t(battle::skillMpCost(skill->reqMp, level));
}

bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledgeSelf, std::int16_t knowledgeOther,
               int distance, int index, int level, std::int16_t &damage, std::int16_t &poisoned,
               std::int16_t &exp, bool &dead) {
    if (!c1 || !c2) { return false; }
    index = std::clamp(index, 0, data::LearnSkillCount - 1);
    auto skillId = c1->skillId[index];
    const auto *skill = mem::gSaveData.skillInfo[skillId];
    if (!skill) { return false; }
    exp = 0;
    poisoned = 0;
    if (skill->damageType > 0) {
        /*
         * `NUM-DRAIN-MP` Z.DAT:0x395EC. The attacker gains `addMp` and grows
         * `maxMp` independently of what the target actually loses; the raw
         * stored skill level is used instead of the mp-limited one.
         */
        auto rawLevel = std::clamp<std::int16_t>(c1->skillLevel[index] / 100, 0, data::SkillLevelMaxDiv);
        auto addMp = skill->addMp[rawLevel];
        std::int16_t selfDelta = std::int16_t(battle::originalRandom(battleRandom(), 3)
                                              - battle::originalRandom(battleRandom(), 3));
        c1->mp = std::int16_t(c1->mp + addMp);
        c1->maxMp = std::clamp<std::int16_t>(c1->maxMp + battle::originalRandom(battleRandom(), addMp / 2),
                                             0, data::MaxMpMax);
        c1->mp = std::int16_t(c1->mp + selfDelta);
        if (c1->mp > c1->maxMp) { c1->mp = c1->maxMp; }
        std::int16_t targetDelta = std::int16_t(battle::originalRandom(battleRandom(), 3)
                                                - battle::originalRandom(battleRandom(), 3));
        auto oldMp = c2->mp;
        c2->mp = std::int16_t(c2->mp - skill->drainMp[rawLevel] - targetDelta);
        if (c2->mp <= 0) { c2->mp = 0; }
        damage = std::int16_t(oldMp - c2->mp);
        dead = c2->hp <= 0;
        return true;
    }
    int atk = calcRealAttack(c1, knowledgeSelf, skill, level);
    int def = calcRealDefense(c2, knowledgeOther);
    int dmg = battle::calcDamage(battle::DamageInput{atk, def, c1->stamina, c2->hurt, distance},
                                 battleRandom());
    damage = std::int16_t(dmg);
    /* `NUM-EXP-HIT` Z.DAT:0x39493 */
    exp = std::int16_t(dmg / 5);
    auto newHp = std::int16_t(c2->hp - dmg);
    if (newHp < 0) {
        newHp = 0;
        /* `NUM-EXP-KILL` Z.DAT:0x394DF: only granted on strict overkill. */
        exp = std::int16_t(exp + c2->level * 10);
    }
    c2->hp = newHp;
    /* `NUM-HURT-ONHIT` Z.DAT:0x394EE */
    c2->hurt = std::int16_t(c2->hurt + dmg / 10);
    if (c2->hurt > data::HurtMax) { c2->hurt = data::HurtMax; }
    auto poisonAdd = battle::poisonOnHit(c1->poisonAmp, c1->skillLevel[index], skill->addPoison,
                                         c2->antipoison);
    if (poisonAdd > 0) {
        auto oldPs = c2->poisoned;
        c2->poisoned = std::clamp<std::int16_t>(c2->poisoned + poisonAdd, 0, data::PoisonedMax);
        poisoned = std::int16_t(c2->poisoned - oldPs);
    }
    dead = c2->hp <= 0;
    return true;
}

void postDamage(CharacterData *c, int index, int level, std::int16_t stamina, bool &levelup) {
    index = std::clamp(index, 0, data::LearnSkillCount - 1);
    /*
     * `NUM-SKILL-GROWTH` Z.DAT:0x38380. The original caps the stored value at
     * 999 and keeps the progress inside the new level instead of rounding it
     * down to a multiple of 100.
     */
    int oldlevel = c->skillLevel[index] / 100;
    c->skillLevel[index] = std::int16_t(c->skillLevel[index] + util::gRandom(1, 2));
    if (c->skillLevel[index] > data::SkillLevelStoreMax) {
        c->skillLevel[index] = data::SkillLevelStoreMax;
    }
    levelup = c->skillLevel[index] / 100 != oldlevel;
    /* `NUM-SKILL-MPCOST` Z.DAT:0x384ED: charged once per attack, not per target. */
    const auto *skill = mem::gSaveData.skillInfo[c->skillId[index]];
    if (skill) {
        c->mp = std::int16_t(c->mp - battle::skillMpCost(skill->reqMp, level));
        if (c->mp < 0) { c->mp = 0; }
    }
    if (stamina) {
        c->stamina = std::int16_t(c->stamina - stamina);
        if (c->stamina < 0) { c->stamina = 0; }
    }
}

std::int16_t actPoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto amount = std::int16_t(battle::poisonAmount(c1->poison, c2->antipoison));
    auto oldPs = c2->poisoned;
    c2->poisoned = std::clamp<std::int16_t>(c2->poisoned + amount, 0, data::PoisonedMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    /* The original reports the poison actually added as a positive value. */
    return std::int16_t(c2->poisoned - oldPs);
}

std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto heal = std::int16_t(battle::medicHeal(c1->medic, c2->hurt, battleRandom()));
    auto hurtCut = std::int16_t(std::max<std::int16_t>(0, c1->medic));
    /* The random draw above happens before this rejection in the original. */
    if (c2->hurt > c1->medic + 20) { heal = 0; hurtCut = 0; }
    auto oldHp = c2->hp;
    c2->hp = std::clamp<std::int16_t>(c2->hp + heal, 0, c2->maxHp);
    c2->hurt = std::clamp<std::int16_t>(c2->hurt - hurtCut, 0, data::HurtMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    return std::int16_t(c2->hp - oldHp);
}

std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto amount = std::int16_t(battle::depoisonAmount(c1->depoison, c2->poisoned, battleRandom()));
    auto oldPs = c2->poisoned;
    c2->poisoned = std::clamp<std::int16_t>(c2->poisoned - amount, 0, data::PoisonedMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    return std::int16_t(oldPs - c2->poisoned);
}

std::int16_t actThrow(CharacterData *c1, CharacterData *c2, std::int16_t itemId, std::int16_t stamina, bool &dead) {
    if (!c1 || !c2) { return 0; }
    const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return 0; }

    auto oldHp = c2->hp;
    /* Negative for damaging items, matching the raw item value. */
    auto delta = battle::throwDamage(itemInfo->addHp, c2->hurt, c1->throwing, battleRandom());
    c2->hurt = std::int16_t(c2->hurt - delta / 4);
    if (c2->hurt > data::HurtMax) { c2->hurt = data::HurtMax; }
    if (c2->hurt < 0) { c2->hurt = 0; }
    c2->hp = std::int16_t(c2->hp + delta);
    if (c2->hp > c2->maxHp) { c2->hp = c2->maxHp; }
    if (c2->hp < 0) { c2->hp = 0; }
    auto poisonDelta = battle::throwPoison(itemInfo->addPoisoned, c1->throwing, c2->antipoison,
                                           battleRandom());
    c2->poisoned = std::clamp<std::int16_t>(c2->poisoned + poisonDelta, 0, data::PoisonedMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    dead = c2->hp <= 0;
    return std::int16_t(c2->hp - oldHp);
}

std::int16_t actRoundEndDrain(CharacterData *c, bool inactive) {
    if (!c) { return 0; }
    /*
     * `NUM-ROUND-DRAIN` Z.DAT:0x3C563. A non-zero hurt value bypasses every
     * other guard in the original, including the inactive and hp checks; the
     * final `hp < 0 -> hp = 1` clamp then revives already dead characters that
     * carry a hurt value. That resurrection is a control-flow defect, so this
     * implementation keeps the dead out of the drain entirely.
     */
    if (c->hp <= 0) { return 0; }
    if (c->hurt <= 0) {
        if (c->poisoned <= 0 || c->stamina <= 0 || inactive) { return 0; }
    }
    auto oldHp = c->hp;
    c->hp = std::int16_t(c->hp - battle::roundEndDrain(c->hurt, c->poisoned));
    if (c->stamina < 0) { c->stamina = 1; }
    if (c->hp < 0) { c->hp = 1; }
    return std::int16_t(c->hp - oldHp);
}

void actRest(CharacterData *c, bool moved) {
    auto gain = battle::restGain(c->stamina, moved, battleRandom());
    c->stamina = std::clamp<std::int16_t>(c->stamina + gain.stamina, 0, data::StaminaMax);
    if (gain.hp) {
        c->hp = std::int16_t(c->hp + gain.hp);
        if (c->hp > c->maxHp) { c->hp = c->maxHp; }
    }
    if (gain.mp) {
        c->mp = std::int16_t(c->mp + gain.mp);
        if (c->mp > c->maxMp) { c->mp = c->maxMp; }
    }
}

void actLevelup(CharacterData *c, int gainedLevels) {
    if (!c || gainedLevels <= 0) { return; }
    /*
     * `NUM-LEVELUP` Z.DAT:0x3B6BE. Every level gained is applied in one step and
     * scales the growth, so the caller must not loop. Only proficiencies above 20
     * improve, except throwing which always does, and `special` never grows.
     */
    auto factor = battle::levelUpFactor(c->potential, battleRandom());
    auto gain = battle::levelUpGain(gainedLevels, c->hpAddOnLevelUp, factor, battleRandom());
    c->level = std::clamp<std::int16_t>(c->level + gainedLevels, 0, data::LevelMax);
    c->maxHp = std::clamp<std::int16_t>(c->maxHp + gain.maxHp, 0, data::MaxHpMax);
    c->hp = c->maxHp;
    c->hurt = 0;
    c->poisoned = 0;
    c->stamina = data::StaminaMax;
    c->maxMp = std::clamp<std::int16_t>(c->maxMp + gain.maxMp, 0, data::MaxMpMax);
    c->mp = c->maxMp;
    c->attack = std::clamp<std::int16_t>(c->attack + gain.stat, 0, data::AttackMax);
    c->speed = std::clamp<std::int16_t>(c->speed + gain.stat, 0, data::SpeedMax);
    c->defence = std::clamp<std::int16_t>(c->defence + gain.stat, 0, data::DefenceMax);
#define GrowProficiency(N, M) \
    if (c->N > 20) { \
        c->N = std::clamp<std::int16_t>(c->N + battle::originalRandom(battleRandom(), 3), 0, data::M); \
    }
    GrowProficiency(medic, MedicMax)
    GrowProficiency(poison, PoisonMax)
    GrowProficiency(depoison, DepoisonMax)
    GrowProficiency(fist, FistMax)
    GrowProficiency(sword, SwordMax)
    GrowProficiency(blade, BladeMax)
#undef GrowProficiency
    c->throwing = std::clamp<std::int16_t>(c->throwing + battle::originalRandom(battleRandom(), 3),
                                          0, data::ThrowingMax);
}

}
