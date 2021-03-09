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

#include "mask.hh"

#include <algorithm>

namespace hojy::scene {

Node::Node(Node *parent, int x, int y, int width, int height) : parent_(parent), renderer_(parent->renderer_), x_(x), y_(y), width_(width), height_(height) {
    if (parent_) { parent_->add(this); }
}

Node::~Node() {
    if (parent_) { parent_->remove(this); }
    removeAllChildren();
}

void Node::add(Node *child) {
    children_.push_back(child);
}

void Node::remove(Node *child) {
    auto ite = std::remove(children_.begin(), children_.end(), child);
    if (ite == children_.end()) { return; }
    child->parent_ = nullptr;
    children_.erase(ite, children_.end());
}

void Node::fadeEnd() {
    runFadePostAction_ = true;
}

void Node::fadeIn(const std::function<void()> &postAction) {
    fadePostAction_ = postAction;
    fadeNode_ = new Mask(this, Mask::FadeIn, 3);
}

void Node::fadeOut(const std::function<void()> &postAction) {
    fadePostAction_ = postAction;
    fadeNode_ = new Mask(this, Mask::FadeOut, 3);
}

void Node::doRender() {
    render();
    for (auto *node : children_) {
        node->doRender();
    }
    if (runFadePostAction_) {
        runFadePostAction_ = false;
        auto fn = std::move(fadePostAction_);
        delete fadeNode_;
        fadeNode_ = nullptr;
        if (fn) { fn(); }
    }
}

void Node::doHandleKeyInput(Node::Key key) {
    if (children_.empty()) {
        handleKeyInput(key);
        return;
    }
    children_.back()->doHandleKeyInput(key);
}

void Node::doTextInput(const std::wstring &str) {
    if (children_.empty()) {
        handleTextInput(str);
        return;
    }
    children_.back()->doTextInput(str);
}

void Node::removeAllChildren() {
    for (auto *n: children_) {
        n->parent_ = nullptr;
        delete n;
    }
    children_.clear();
}

}
