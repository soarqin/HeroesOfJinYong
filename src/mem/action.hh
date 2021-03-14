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
    Fist, Sword, Blade, Special, HiddenWeapon, Knowledge, Integrity, DoubleAttack, PoisonAmp,
};
const std::wstring &propToName(PropType type);
void addUpPropFromEquipToChar(CharacterData *info);
std::uint16_t getExpForLevelUp(std::int16_t level);
std::uint16_t getExpForSkillLearn(std::int16_t itemId, std::int16_t level, std::int16_t potential);
bool leaveTeam(std::int16_t id);
bool equipItem(std::int16_t charId, std::int16_t itemId);
bool useItem(std::int16_t charId, std::int16_t itemId, std::map<PropType, std::int16_t> &changes);
bool applyItemChanges(CharacterData *charInfo, const ItemData *itemInfo, std::map<PropType, std::int16_t> &changes);
bool canUseItem(const CharacterData *charInfo, const ItemData *itemInfo);
std::int16_t getLeaveEventId(std::int16_t id);
std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> calcColorForMpType(std::int16_t type);
bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledge1, std::int16_t knowledge2,
               int distance, int index, int level, std::int16_t stamina,
               std::int16_t &damage, std::int16_t &poisoned, bool &dead, bool &levelup);
std::int16_t actPoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actPoisonDamage(CharacterData *c);

}
