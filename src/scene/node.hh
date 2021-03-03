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

#include "renderer.hh"

#include <vector>
#include <cstdint>

namespace hojy::scene {

class Node {
    friend class Window;
public:
    enum Key {
        KeyUp,
        KeyDown,
        KeyLeft,
        KeyRight,
        KeyOK,
        KeyCancel,
    };
public:
    Node(Node *parent, std::uint32_t width, std::uint32_t height): parent_(parent), renderer_(parent->renderer_), width_(width), height_(height){}
    Node(Renderer *renderer, std::uint32_t width, std::uint32_t height): parent_(nullptr), renderer_(renderer), width_(width), height_(height) {}
    Node(const Node&) = delete;
    void add(Node *child);
    void remove(Node *child);

    virtual void render() = 0;
    virtual void handleKeyInput(Key key) {}

protected:
    void doRender();

protected:
    Node *parent_;
    Renderer *renderer_;

    std::uint32_t width_, height_;
    bool visible_ = true;

    std::vector<Node*> children_;
};

}
