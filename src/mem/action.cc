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
#include "data/factors.hh"
#include "util/random.hh"
#include <algorithm>

namespace hojy::mem {

const std::wstring &propToName(PropType type) {
    static const std::wstring names[] = {
        L"生命", L"生命最大值", L"中毒程度", L"體力", L"內力性質", L"內力", L"內力最大值",
        L"攻擊力", L"輕功", L"防御力", L"醫療能力", L"用毒能力", L"解毒能力", L"抗毒能力",
        L"拳掌功夫", L"御劍能力", L"耍刀技巧", L"特殊兵器", L"暗器技巧", L"武學常識", L"道德", L"左右互搏", L"功夫帶毒",
    };
    return names[int(type)];
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
    if (level >= 9) {
        return 0;
    }
    return mem::gSaveData.itemInfo[itemId]->reqExp * (level <= 0 ? 1 : level) * std::clamp<std::int16_t>(7 - potential / 15, 1, 5);
}

bool leaveTeam(std::int16_t id) {
    if (id <= 0) { return false; }
    auto *charInfo = mem::gSaveData.charInfo[id];
    if (!charInfo) { return false; }
    for (int i = 0; i < data::TeamMemberCount; ++i) {
        if (mem::gSaveData.baseInfo->members[i] != id) { continue; }
        if (i < data::TeamMemberCount - 1) {
            memmove(mem::gSaveData.baseInfo->members + i,
                    mem::gSaveData.baseInfo->members + i + 1,
                    sizeof(std::int16_t) * (data::TeamMemberCount - i - 1));
        }
        mem::gSaveData.baseInfo->members[data::TeamMemberCount - 1] = -1;
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
        return true;
    }
    return false;
}

bool skillFull(std::int16_t charId) {
    if (charId < 0) { return true; }
    const auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    for (auto id: charInfo->skillId) {
        if (id < 0) { return false; }
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

bool useItem(std::int16_t charId, std::int16_t itemId, std::map<PropType, std::int16_t> &changes) {
    if (charId < 0) { return false; }
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return false; }
    auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return false; }
    if (!canUseItem(charInfo, itemInfo)) { return false; }
    if (!applyItemChanges(charInfo, itemInfo, changes)) { return false; }
    gBag.remove(itemId, 1);
    return true;
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

bool canUseItem(const CharacterData *charInfo, const ItemData *itemInfo) {
    if (itemInfo->itemType == 1 || itemInfo->itemType == 2) {
        if (itemInfo->charOnly >= 0 && itemInfo->charOnly != charInfo->id) { return false; }
        if (itemInfo->reqMpType == 0 || itemInfo->reqMpType == 1) {
            if (charInfo->mpType < 2 && itemInfo->reqMpType != charInfo->mpType) { return false; }
        }
    }
    auto check = [](std::int16_t v, std::int16_t n)->bool {
        if (n < 0) {
            return v <= -n;
        }
        return v >= n;
    };
    std::int16_t reqMpType, reqMp, reqAttack, reqSpeed, reqPoison, reqMedic, reqDepoison;
    std::int16_t reqFist, reqSword, reqBlade, reqSpecial, reqHiddenWeapon, reqPotential;
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
            return data::gFactors.leaveTeamStartEvents + std::int16_t(i);
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
    int dmg = (atk - def * 3) * 2 / 3;
    if (dmg < 0) {
        dmg = atk / 10;
    }
    if (dmg > 0) {
        dmg += stamina / 15 + hurt / 20;
        if (distance > 1) {
            if (distance <= 10) {
                dmg = dmg * (100 - (distance - 1) * 3) / 100;
            } else {
                dmg = dmg * 2 / 3;
            }
        }
    } else {
        dmg = 1;
    }
    return dmg;
}

std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currMp) {
    auto mpUse = reqMp * (level / 2 + 1);
    if (mpUse > currMp) {
        return currMp / reqMp * 2;
    }
    return level;
}

bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledge1, std::int16_t knowledge2,
               int distance, int index, int level, std::int16_t stamina,
               std::int16_t &damage, std::int16_t &poisoned, bool &dead, bool &levelup) {
    if (!c1 || !c2) { return false; }
    index = std::clamp(index, 0, 9);
    auto skillId = c1->skillId[index];
    const auto *skill = mem::gSaveData.skillInfo[skillId];
    if (!skill) { return false; }
    if (c1->mp < skill->reqMp) { return false; }
    c1->mp = std::max(0, c1->mp - skill->reqMp);
    int atk = calcRealAttack(c1, knowledge1, skill, level);
    int def = calcRealDefense(c2, knowledge2);
    int dmg = (atk - def * 3) * 2 / 3 + int(util::gRandom(21) - util::gRandom(21));
    if (dmg < 0) {
        dmg = atk / 10 + int(util::gRandom(5) - util::gRandom(5));
    }
    if (dmg > 0) {
        dmg += c1->stamina / 15 + c2->hurt / 20;
        if (distance > 1) {
            if (distance <= 10) {
                dmg = dmg * (100 - (distance - 1) * 3) / 100;
            } else {
                dmg = dmg * 2 / 3;
            }
        }
    } else {
        dmg = 1;
    }
    damage = dmg;
    c2->hp = std::max(0, c2->hp - dmg);
    c1->skillLevel[index] += util::gRandom(1, 2);
    levelup = c1->skillLevel[index] / 100 != level;
    if (levelup) {
        c1->skillLevel[index] = c2->skillLevel[index] / 100 * 100;
    }
    c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    poisoned = 0;
    if (c2->hp > 0) {
        /* add poison */
        if (c2->antipoison < 90) {
            int poison = c1->poisonAmp + level * skill->addPoison - c2->antipoison;
            if (poison) {
                std::int16_t oldPs = c2->poisoned;
                c2->poisoned = std::clamp<std::int16_t>(c2->poisoned + poison / 15, 0, data::PoisonedMax);
                poisoned = oldPs - c2->poisoned;
            }
        }
        dead = false;
    } else {
        dead = true;
    }
    return true;
}

std::int16_t actPoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    if (c1->poison <= c2->antipoison) { return 0; }
    auto oldPs = c2->poisoned;
    c2->poisoned = std::clamp<std::int16_t>(c2->poisoned + (c1->poison - c2->antipoison) / 4, 0, data::PoisonedMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    return oldPs - c2->poisoned;
}

std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto oldHp = c2->hp;
    int heal;
    if (c2->hurt > c1->medic + 20) { return 0; }
    if (c2->hurt <= 25) {
        heal = c1->medic * 4 / 5;
    } else if (c2->hurt <= 50) {
        heal = c1->medic * 3 / 4;
    } else if (c2->hurt <= 75) {
        heal = c1->medic * 2 / 3;
    } else {
        heal = c1->medic / 2;
    }
    c2->hp = std::clamp<std::int16_t>(c2->hp + heal + util::gRandom(6), 0, c2->maxHp);
    c2->hurt = std::clamp<std::int16_t>(c2->hurt - c1->medic, 0, data::HurtMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    return c2->hp - oldHp;
}

std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto oldPs = c2->poisoned;
    c2->poisoned = std::clamp<std::int16_t>(c2->poisoned - c1->depoison / 3 + util::gRandom(6) - util::gRandom(6), 0, data::PoisonedMax);
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    return oldPs - c2->poisoned;
}

std::int16_t actThrow(CharacterData *c1, CharacterData *c2, std::int16_t itemId, std::int16_t stamina, bool &dead) {
    if (!c1 || !c2) { return 0; }
    const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (!itemInfo) { return 0; }

    int div;
    if (c2->hurt == 0) {
        div = 4;
    } else if (c2->hurt <= 33) {
        div = 3;
    } else if (c2->hurt <= 66) {
        div = 2;
    } else {
        div = 1;
    }
    auto oldHp = c2->hp;
    c2->hp = std::clamp<std::int16_t>(c2->hp - std::max<std::int16_t>(1, (-itemInfo->addHp / div + util::gRandom(6) + c1->throwing * 2) / 3), 0, c2->maxHp);
    if (c2->antipoison < 100) {
        auto ps = itemInfo->addPoisoned <= 0 ? (itemInfo->addPoisoned / 2 + util::gRandom(6) - util::gRandom(6))
            : ((itemInfo->addPoisoned - c2->throwing) / 2 - c2->antipoison) / 2;
        if (ps > 0) {
            c2->poisoned = std::clamp<std::int16_t>(c2->poisoned - ps, 0, data::PoisonedMax);
        }
    }
    if (stamina) {
        c1->stamina = std::clamp<std::int16_t>(c1->stamina - stamina, 0, data::StaminaMax);
    }
    dead = c2->hp <= 0;
    return c2->hp - oldHp;
}

std::int16_t actPoisonDamage(CharacterData *c) {
    if (!c->poisoned) { return 0; }
    auto oldHp = c->hp;
    c->hp = std::clamp<std::int16_t>(c->hp - c->poisoned / 10, 1, c->maxHp);
    return c->hp - oldHp;
}

}
