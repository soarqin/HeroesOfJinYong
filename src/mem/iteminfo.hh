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

struct ItemData {
    std::int16_t id;
    char name[20], name2[20];
    char desc[30];
    std::int16_t skillId, throwingEffectId;
    std::int16_t user;      /* current equipped user */
    std::int16_t equipType; /* 0Weapon 1Armor */
    std::int16_t showDesc;
    std::int16_t itemType;  /* 0Special 1Equip 2Skill 3Heal 4Attack */
    std::int16_t padding[3];
    std::int16_t addHp, addMaxHp, addPoisoned, addStamina, changeMpType, addMp, addMaxMp;
    std::int16_t addAttack, addSpeed, addDefence, addMedic, addPoison, addDepoison, addAntipoison;
    std::int16_t addFist, addSword, addBlade, addSpecial, addThrowing, addKnowledge, addIntegrity, addDoubleAttack, addPoisonAmp;
    std::int16_t charOnly;  /* for equip and skill book: only this char can equip/learn it */
    std::int16_t reqMpType, reqMp, reqAttack, reqSpeed, reqPoison, reqMedic, reqDepoison;
    std::int16_t reqFist, reqSword, reqBlade, reqSpecial, reqThrowing, reqPotential;
    std::int16_t reqExp, reqExpForMakeItem, reqMaterial;
    std::int16_t makeItem[data::MakeItemCount], makeItemCount[data::MakeItemCount];
};

using ItemInfo = SerializableStructVec<ItemData>;

}
