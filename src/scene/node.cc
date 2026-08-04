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

Node::Node(Node *parent, int x, int y, int width, int height): x_(x), y_(y), width_(width), height_(height) {
    if (parent) { parent->add(this); }
}

Node::~Node() {
    if (parent_) { parent_->remove(this); }
    removeAllChildren();
}

Node *Node::rootNode() {
    auto *root = this;
    while (root->parent_) { root = root->parent_; }
    return root;
}

const Node *Node::rootNode() const {
    auto *root = this;
    while (root->parent_) { root = root->parent_; }
    return root;
}

void Node::requestDelete() {
    if (deleteRequested_) { return; }
    deleteRequested_ = true;
    auto *root = rootNode();
    root->pendingDeletes_.push_back(this);
}

bool Node::consumeDeleteRequest() {
    if (!deleteRequested_) { return false; }
    deleteRequested_ = false;
    return true;
}

void Node::applyDeferredDeletes() {
    auto *root = rootNode();
    if (root != this) {
        root->applyDeferredDeletes();
        return;
    }
    if (dispatchDepth_ != 0 || pendingDeletes_.empty()) { return; }

    auto pending = std::move(pendingDeletes_);
    pendingDeletes_.clear();
    std::vector<Node*> candidates;
    candidates.reserve(pending.size());
    for (auto *node : pending) {
        if (!node || node == this) { continue; }
        bool covered = false;
        for (auto *ancestor : pending) {
            if (!ancestor || ancestor == node || ancestor == this) { continue; }
            for (auto *parent = node->parent_; parent; parent = parent->parent_) {
                if (parent == ancestor) {
                    covered = true;
                    break;
                }
            }
            if (covered) { break; }
        }
        if (!covered) { candidates.push_back(node); }
    }
    for (auto *node : candidates) {
        delete node;
    }
}

void Node::add(Node *child) {
    if (!child || child == this) { return; }
    child->parent_ = this;
    child->renderer_ = renderer_;
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

void Node::makeCenter(int w, int h, int x, int y) {
    x_ = x + (w - width_) / 2;
    y_ = y + (h - height_) / 2;
}

void Node::doUpdate() {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        const bool runFadePostAction = runFadePostAction_;
        if (runFadePostAction) { runFadePostAction_ = false; }
        update();
        const auto children = children_;
        for (auto *node : children) {
            if (node && node->parent_ == this && !node->deleteRequested_) {
                node->doUpdate();
            }
        }
        if (runFadePostAction) {
            auto fn = std::move(fadePostAction_);
            if (fadeNode_) {
                fadeNode_->requestDelete();
                fadeNode_ = nullptr;
            }
            if (fn) { fn(); }
        }
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::doRender() {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        render();
        const auto children = children_;
        for (auto *node : children) {
            if (node && node->parent_ == this && !node->deleteRequested_) {
                node->doRender();
            }
        }
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::doHandleKeyInput(Node::Key key) {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        if (children_.empty()) {
            handleKeyInput(key);
        } else {
            auto *child = children_.back();
            if (child && child->parent_ == this && !child->deleteRequested_) {
                child->doHandleKeyInput(key);
            }
        }
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::doTextInput(const std::wstring &str) {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        if (children_.empty()) {
            handleTextInput(str);
        } else {
            auto *child = children_.back();
            if (child && child->parent_ == this && !child->deleteRequested_) {
                child->doTextInput(str);
            }
        }
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::removeAllChildren() {
    auto *root = rootNode();
    if (root->dispatchDepth_ != 0) {
        const auto children = children_;
        for (auto *n : children) {
            if (n) { n->requestDelete(); }
        }
        return;
    }
    for (auto *n: children_) {
        n->parent_ = nullptr;
        delete n;
    }
    children_.clear();
}

}
