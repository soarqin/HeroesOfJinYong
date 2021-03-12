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

struct SkillData {
    std::int16_t id;
    char name[10];
    std::int16_t padding[5];
    std::int16_t soundId;
    std::int16_t skillType;    //1Fist 2Sowrd 3Blade 4Special
    std::int16_t effectId;
    std::int16_t damageType;        //0Normal 1Drain MP
    std::int16_t attackAreaType;    //0single 1line 2cross 3area
    std::int16_t reqMp, addPoison;
    std::int16_t attack[data::SkillCheckCount], selDistance[data::SkillCheckCount], attackDistance[data::SkillCheckCount], addMp[data::SkillCheckCount], drainMp[data::SkillCheckCount];
};

using SkillInfo = SerializableStructVec<SkillData>;

}
