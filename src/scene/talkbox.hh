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
#include <string>
#include <cstdint>

namespace hojy::scene {

class TalkBox: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;

    void popup(const std::wstring &text, std::int16_t headId, std::int16_t position);

    void handleKeyInput(Key key) override;

private:
    void makeCache() override;

private:
    std::vector<std::wstring> text_;
    const Texture *headTex_ = nullptr;
    std::pair<int, int> headScale_ = {2, 1};
    std::int16_t position_ = 0;
    int index_ = 0, dispLines_ = 0;
};

}
