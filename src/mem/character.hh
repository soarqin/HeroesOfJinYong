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
#include "data/consts.hh"

namespace hojy::mem {

struct CharacterData {
    std::int16_t id;
    std::int16_t headId, hpAddOnLevelUp, padding;
    char name[10], nick[10];
    std::int16_t sex;
    std::int16_t level;
    std::uint16_t exp;
    std::int16_t hp, maxHp, hurt, poisoned, stamina;
    std::uint16_t expForMakeItem;
    std::int16_t equip0, equip1;
    std::int16_t frame[data::CharFrameCount];
    std::int16_t frameDelay[data::CharFrameCount];
    std::int16_t frameSoundDelay[data::CharFrameCount];
    std::int16_t mpType, mp, maxMp;
    std::int16_t attack, speed, defence, medic, poison, depoison, antipoison, fist, sword, blade, special, throwing;
    std::int16_t knowledge, integrity, poisonAmp, doubleAttack, reputation, potential;
    std::int16_t learningItem;
    std::uint16_t expForItem;
    std::int16_t skillId[data::LearnSkillCount], skillLevel[data::LearnSkillCount];
    std::int16_t item[data::CarryItemCount], itemCount[data::CarryItemCount];
};

using Character = SerializableStructVec<CharacterData>;

}
