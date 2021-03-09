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
#include <algorithm>

namespace hojy::mem {

std::uint16_t getExpForLevelUp(std::int16_t level) {
    --level;
    if (level >= data::gFactors.expForLevelUp.size()) { return 0;}
    return data::gFactors.expForLevelUp[level];
}

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> calcColorForMpType(std::int16_t type) {
    switch (type) {
    case 0:
        return std::make_tuple(208, 152, 208);
        break;
    case 1:
        return std::make_tuple(236, 200, 40);
        break;
    default:
        break;
    }
    return std::make_tuple(252, 252, 252);
}

std::int16_t actMedic(CharacterData *c1, CharacterData *c2, int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto oldHp = c2->hp;
    c2->hp = std::clamp<int16_t>(c2->hp + c1->medic, 0, c2->maxHp);
    if (stamina) {
        c1->stamina = std::clamp<int16_t>(c1->stamina - stamina, 0, StaminaMax);
    }
    return c2->hp - oldHp;
}

std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, int16_t stamina) {
    if (!c1 || !c2) { return 0; }
    auto oldPs = c2->poisoned;
    c2->poisoned = std::clamp<int16_t>(c2->poisoned - c1->depoison / 3, 0, PoisonedMax);
    if (stamina) {
        c1->stamina = std::clamp<int16_t>(c1->stamina - stamina, 0, StaminaMax);
    }
    return oldPs - c2->poisoned;
}

}
