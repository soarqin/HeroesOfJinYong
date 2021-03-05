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

#include "node.hh"

#include <vector>
#include <string>
#include <cstdint>

namespace hojy::scene {

class TalkBox: public Node {
public:
    using Node::Node;

    void popup(const std::wstring &text, std::int16_t headId, std::int16_t position);

    void render() override;
    void handleKeyInput(Key key) override;

private:
    std::vector<std::wstring> text_;
    const Texture *headTex_ = nullptr;
    int index_ = 0, dispLines_ = 0, rowHeight_ = 0;
    int headX_ = 0, headY_ = 0, headW_ = 0, headH_ = 0;
    int textX_ = 0, textY_ = 0, textW_ = 0, textH_ = 0;
};

}
