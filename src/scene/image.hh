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

class Texture;

class Image : public Node {
public:
    using Node::Node;
    ~Image() override;

    bool loadRAW(const std::vector<std::string> &filename, int width, int height);
    void setTexture(const Texture *texture);

    void render() override;

protected:
    void calcDrawRect();

private:
    Texture *texture_ = nullptr;
    bool keepAspectRatio_ = true;
    bool freeOnClose_ = false;
    int align_ = 0;
    int destX_ = 0, destY_ = 0, destWidth_ = 0, destHeight_ = 0;
};

}
