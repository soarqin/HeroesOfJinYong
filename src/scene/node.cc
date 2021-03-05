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

#include "node.hh"

#include <algorithm>

namespace hojy::scene {

Node::~Node() {
    for (auto *n: children_) {
        delete n;
    }
    children_.clear();
}

void Node::add(Node *child) {
    children_.push_back(child);
}

void Node::remove(Node *child) {
    children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
}

void Node::doRender() {
    render();
    for (auto *node : children_) {
        node->doRender();
    }
}

void Node::doHandleKeyInput(Node::Key key) {
    if (children_.empty()) {
        handleKeyInput(key);
        return;
    }
    children_.back()->handleKeyInput(key);
}

}
