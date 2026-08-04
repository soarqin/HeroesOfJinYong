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

#include "bag.hh"

#include "savedata.hh"

#include <algorithm>
#include <cstring>

namespace hojy::mem {

Bag gBag;

void Bag::syncFromSave() {
    items_.clear();
    orderedItems_.clear();
    for (auto &item : gSaveData.baseInfo->items) {
        if (item.count) {
            items_[item.id] += item.count;
            auto ordered = std::find_if(
                orderedItems_.begin(), orderedItems_.end(),
                [&item](const ItemEntry &entry) { return entry.first == item.id; });
            if (ordered == orderedItems_.end()) {
                orderedItems_.emplace_back(item.id, item.count);
            } else {
                ordered->second += item.count;
            }
        }
    }
    dirty_ = false;
}

void Bag::syncToSave() {
    if (!dirty_) {
        return;
    }
    dirty_ = false;
    size_t index = 0;
    for (const auto &p: orderedItems_) {
        auto &item = gSaveData.baseInfo->items[index++];
        item.id = p.first;
        item.count = p.second;
    }
    auto &items = gSaveData.baseInfo->items;
    for (; index < data::BagItemCount; ++index) {
        items[index] = {-1, 0};
    }
}

void Bag::add(std::int16_t id, std::int16_t count) {
    if (count == 0 || id < 0 || id >= data::BagItemCount) { return; }
    auto &cnt = items_[id];
    cnt += count;
    if (cnt <= 0) {
        items_.erase(id);
        orderedItems_.erase(
            std::remove_if(orderedItems_.begin(), orderedItems_.end(),
                           [id](const ItemEntry &item) { return item.first == id; }),
            orderedItems_.end());
    } else {
        auto ordered = std::find_if(
            orderedItems_.begin(), orderedItems_.end(),
            [id](const ItemEntry &item) { return item.first == id; });
        if (ordered == orderedItems_.end()) {
            orderedItems_.emplace_back(id, cnt);
        } else {
            ordered->second = cnt;
        }
        const auto *itemInfo = gSaveData.itemInfo[id];
        if (itemInfo) {
            switch (itemInfo->itemType) {
            case 1:
            case 2:
                if (cnt > 1) {
                    cnt = 1;
                    auto ordered = std::find_if(
                        orderedItems_.begin(), orderedItems_.end(),
                        [id](const ItemEntry &item) { return item.first == id; });
                    if (ordered != orderedItems_.end()) {
                        ordered->second = cnt;
                    }
                }
            default: break;
            }
        }
    }
    dirty_ = true;
}

bool Bag::remove(std::int16_t id, std::int16_t count) {
    if (id < 0 || id >= data::BagItemCount) { return false; }
    if (count <= 0) { return true; }
    auto ite = items_.find(id);
    if (ite == items_.end()) { return false; }
    auto &cnt = items_[id];
    if (cnt < count) {
        return false;
    }
    cnt -= count;
    if (cnt <= 0) {
        items_.erase(id);
        orderedItems_.erase(
            std::remove_if(orderedItems_.begin(), orderedItems_.end(),
                           [id](const ItemEntry &item) { return item.first == id; }),
            orderedItems_.end());
    } else {
        auto ordered = std::find_if(
            orderedItems_.begin(), orderedItems_.end(),
            [id](const ItemEntry &item) { return item.first == id; });
        if (ordered != orderedItems_.end()) {
            ordered->second = cnt;
        }
    }
    dirty_ = true;
    return true;
}

}
