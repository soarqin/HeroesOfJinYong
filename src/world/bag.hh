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

#include "content/constants.hh"
#include <map>
#include <cstdint>
#include <utility>
#include <vector>

namespace hojy::world::state {

struct BaseData;

class Bag {
public:
    using ItemEntry = std::pair<std::int16_t, std::int16_t>;

    Bag() = default;
    Bag(const Bag &) = default;
    Bag &operator=(const Bag &) = default;
    Bag(Bag &&) noexcept = default;
    Bag &operator=(Bag &&) noexcept = default;

    void swap(Bag &other) noexcept;

    [[nodiscard]] bool syncFrom(const BaseData &base) noexcept;
    void syncFromSave();
    void syncTo(BaseData &base) const;
    void syncToSave();
    void add(std::int16_t id, std::int16_t count);
    bool remove(std::int16_t id, std::int16_t count);
    [[nodiscard]] const std::map<std::int16_t, std::int16_t> &items() const { return items_; }
    // Battle AI follows the DOS save-slot order rather than sorted item IDs.
    [[nodiscard]] const std::vector<ItemEntry> &orderedItems() const { return orderedItems_; }
    [[nodiscard]] inline std::int16_t operator[](std::int16_t id) const {
        auto ite = items_.find(id);
        if (ite == items_.end()) return 0;
        return ite->second;
    }

private:
    std::map<std::int16_t, std::int16_t> items_;
    std::vector<ItemEntry> orderedItems_;
    bool dirty_ = false;
};

extern Bag gBag;

}
