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

#include "item_slots.hh"

namespace hojy::world::state {

bool consumeNpcItemAt(CharacterData *charInfo, int slot,
                      std::int16_t expectedItemId) {
    if (!charInfo || slot < 0 || slot >= ::hojy::content::CarryItemCount
        || charInfo->itemCount[slot] <= 0
        || (expectedItemId >= 0 && charInfo->item[slot] != expectedItemId)) {
        return false;
    }
    if (--charInfo->itemCount[slot] <= 0) {
        compactCarryItemSlots(*charInfo, slot);
    }
    return true;
}

bool consumeNpcItem(CharacterData *charInfo, std::int16_t itemId) {
    if (!charInfo) { return false; }
    for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
        if (charInfo->item[i] != itemId || charInfo->itemCount[i] <= 0) { continue; }
        return consumeNpcItemAt(charInfo, i, itemId);
    }
    return false;
}

std::int16_t findNpcItemSlot(const CharacterData *charInfo,
                             std::int16_t itemId) {
    if (!charInfo || itemId < 0) { return -1; }
    for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
        if (charInfo->item[i] == itemId && charInfo->itemCount[i] > 0) {
            return static_cast<std::int16_t>(i);
        }
    }
    return -1;
}

}
