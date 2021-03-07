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
        KeyYes,
        KeyNo,
    };
public:
    Node(Node *parent, int x, int y, int width, int height);
    Node(Renderer *renderer, int x, int y, int width, int height): parent_(nullptr), renderer_(renderer), x_(x), y_(y), width_(width), height_(height) {}
    Node(const Node&) = delete;
    virtual ~Node();
    void add(Node *child);
    void remove(Node *child);

    virtual void close() { removeAllChildren(); }
    virtual void render() = 0;
    virtual void handleKeyInput(Key key) {}

protected:
    void doRender();
    void doHandleKeyInput(Key key);
    void removeAllChildren();

protected:
    Node *parent_;
    Renderer *renderer_;

    int x_, y_, width_, height_;
    bool visible_ = true;

    std::vector<Node*> children_;
};

}
