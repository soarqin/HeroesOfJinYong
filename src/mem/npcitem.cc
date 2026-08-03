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

#include <algorithm>

namespace hojy::mem {

bool consumeNpcItem(CharacterData *charInfo, std::int16_t itemId) {
    if (!charInfo) { return false; }
    for (int i = 0; i < data::CarryItemCount; ++i) {
        if (charInfo->item[i] != itemId || charInfo->itemCount[i] <= 0) { continue; }
        if (--charInfo->itemCount[i] == 0) {
            std::move(charInfo->item + i + 1,
                      charInfo->item + data::CarryItemCount,
                      charInfo->item + i);
            std::move(charInfo->itemCount + i + 1,
                      charInfo->itemCount + data::CarryItemCount,
                      charInfo->itemCount + i);
            charInfo->item[data::CarryItemCount - 1] = -1;
            charInfo->itemCount[data::CarryItemCount - 1] = 0;
        }
        return true;
    }
    return false;
}

}
