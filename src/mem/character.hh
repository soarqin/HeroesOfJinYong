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

#include "serializable.hh"

namespace hojy::mem {

enum {
    CharFrameCount = 15,
    LearnSkillCount = 10,
    CarryItemCount = 4,
};

#pragma pack(push, 1)
struct CharacterData {
    std::int16_t id;
    std::int16_t headId, incLife, padding;
    char name[10], nick[10];
    std::int16_t sex;
    std::int16_t level;
    std::uint16_t exp;
    std::int16_t hp, maxHp, hurt, poisoned, stamina;
    std::uint16_t expForMakeItem;
    std::int16_t equip0, equip1;
    std::int16_t frame[CharFrameCount];
    std::int16_t mpType, mp, maxMp;
    std::int16_t attack, speed, defence, medic, poison, depoison, antipoison, fist, sword, blade, special, hiddenWeapon;
    std::int16_t knowledge, integrity, poisonAmp, doubleAttack, fame, potential;
    std::int16_t trainingItem;
    std::uint16_t expForItem;
    std::int16_t skillId[LearnSkillCount], skillLevel[LearnSkillCount];
    std::int16_t item[CarryItemCount], itemCount[CarryItemCount];
} ATTR_PACKED;
#pragma pack(pop)

using Character = SerializableStructVec<CharacterData>;

}