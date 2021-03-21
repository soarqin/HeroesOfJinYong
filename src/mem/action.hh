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

#pragma once

#include "character.hh"
#include "iteminfo.hh"
#include "skillinfo.hh"

#include <string>
#include <tuple>
#include <map>
#include <cstdint>

namespace hojy::mem {

enum class PropType {
    Hp = 0, MaxHp, Poisoned, Stamina, MpType, Mp, MaxMp,
    Attack, Speed, Defence, Medic, Poison, Depoison, Antipoison,
    Fist, Sword, Blade, Special, Throwing, Knowledge, Integrity, DoubleAttack, PoisonAmp,
};
const std::wstring &propToName(PropType type);
void addUpPropFromEquipToChar(CharacterData *info);
std::uint16_t getExpForLevelUp(std::int16_t level);
std::uint16_t getExpForSkillLearn(std::int16_t itemId, std::int16_t level, std::int16_t potential);
bool leaveTeam(std::int16_t id);
bool skillFull(std::int16_t charId);
bool equipItem(std::int16_t charId, std::int16_t itemId);
bool useItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes);
std::int16_t tryUseBagItem(CharacterData *charInfo, PropType type, std::int16_t value);
bool useNpcItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes);
std::int16_t tryUseNpcItem(CharacterData *charInfo, PropType type, std::int16_t value);
bool applyItemChanges(CharacterData *charInfo, const ItemData *itemInfo, std::map<PropType, std::int16_t> &changes);
bool canUseItem(const CharacterData *charInfo, const ItemData *itemInfo);
std::int16_t getLeaveEventId(std::int16_t id);
std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> calcColorForMpType(std::int16_t type);
std::int16_t calcRealAttack(const CharacterData *c, std::int16_t knowledge, const SkillData *skill, std::int16_t level);
std::int16_t calcRealDefense(const CharacterData *c, std::int16_t knowledge);
std::int16_t calcPredictDamage(std::int16_t atk, std::int16_t def, std::int16_t stamina, std::int16_t hurt, std::int16_t distance);
std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currMp);
bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledge1, std::int16_t knowledge2,
               int distance, int index, int level, std::int16_t &damage, std::int16_t &poisoned, bool &dead);
void postDamage(CharacterData *c, int index, std::int16_t stamina, bool &levelup);
std::int16_t actPoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actThrow(CharacterData *c1, CharacterData *c2, std::int16_t itemId, std::int16_t stamina, bool &dead);
std::int16_t actPoisonDamage(CharacterData *c);
void actRest(CharacterData *c);
void actLevelup(CharacterData *c);

}
