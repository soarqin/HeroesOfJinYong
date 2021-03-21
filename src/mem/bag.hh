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

#include "data/consts.hh"
#include <map>
#include <cstdint>

namespace hojy::mem {

class Bag {
public:
    void syncFromSave();
    void syncToSave();
    void add(std::int16_t id, std::int16_t count);
    bool remove(std::int16_t id, std::int16_t count);
    [[nodiscard]] const std::map<std::int16_t, std::int16_t> &items() const { return items_; }
    [[nodiscard]] inline std::int16_t operator[](std::int16_t id) const {
        auto ite = items_.find(id);
        if (ite == items_.end()) return 0;
        return ite->second;
    }

private:
    std::map<std::int16_t, std::int16_t> items_;
    bool dirty_ = false;
};

extern Bag gBag;

}
