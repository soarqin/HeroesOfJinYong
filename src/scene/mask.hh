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

#include <chrono>
#include <functional>

namespace hojy::scene {

class Mask: public Node {
public:
    enum Type {
        FadeIn,
        FadeOut,
    };
    Mask(Node *parent, Type type, int interval);

    void render() override;

private:
    Type type_;
    int interval_;
    std::chrono::steady_clock::time_point start_;
};

}
