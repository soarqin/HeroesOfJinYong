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

#include "nodewithcache.hh"

#include <vector>
#include <functional>
#include <cstdint>

namespace hojy::scene {

class ItemView: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;

    void show(bool inBattle, const std::function<void(std::int16_t)> &resultFunc);
    void handleKeyInput(Key key) override;

protected:
    void makeCache() override;

protected:
    std::vector<std::pair<std::int16_t, std::int16_t>> items_;
    bool inBattle_ = false;
    int cols_ = 0, rows_ = 0;
    int scale_ = 1, cellWidth_ = 0, cellHeight_ = 0;
    int currTop_ = 0, currSel_ = 0;
    std::function<void(std::int16_t)> resultFunc_;
};

}
