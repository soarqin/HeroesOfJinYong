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

#include "bag.hh"
#include "data/consts.hh"

#include <cstdint>

namespace hojy::mem {

struct BaseData {
    struct ItemInfo {
        std::int16_t id, count;
    };
    std::int16_t onShip, subMap, mainX, mainY, subX, subY, direction, shipX, shipY, shipX1, shipY1, encode;
    std::int16_t members[data::TeamMemberCount];
    ItemInfo items[data::BagItemCount];
};

using BaseInfo = SerializableStruct<BaseData>;

}
