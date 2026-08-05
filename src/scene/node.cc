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
    if (lifetimeState_) {
        lifetimeState_->owner = nullptr;
        lifetimeState_.reset();
    }
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
    child->commandSink_ = commandSink_;
    child->phaseTime_ = phaseTime_;
    children_.push_back(child);
}

void Node::setCommandSink(CommandSink sink) {
    commandSink_ = std::move(sink);
    for (auto *child: children_) {
        if (child) { child->setCommandSink(commandSink_); }
    }
}

void Node::postCommand(std::unique_ptr<SceneCommand> command) const {
    if (commandSink_ && command) { commandSink_(std::move(command)); }
}

void Node::postCommand(std::function<void(SceneCommandContext &)> command) const {
    if (command) {
        postCommand(std::make_unique<FunctionSceneCommand>(std::move(command)));
    }
}

Node::LifetimeHandle Node::lifetimeHandle() {
    if (!lifetimeState_) {
        lifetimeState_ = std::make_shared<LifetimeState>();
        lifetimeState_->owner = this;
    }
    return lifetimeState_;
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
                // The fixed-logic phase may only record that the fade has
                // completed.  The presentation tree owns the actual fade
                // node cleanup and performs it from doPrepareRender().
                fadeCleanupRequested_ = true;
            }
            if (fn) {
                // Fade continuations can replace scenes, create widgets, or
                // emit audio.  Queue them behind the fixed-logic barrier so
                // the update traversal never executes presentation work
                // inline.
                postCommand([fn = std::move(fn)](SceneCommandContext &) mutable {
                    fn();
                });
            }
        }
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::doPrepareRender() {
    if (presentationCleanupRequested_) {
        presentationCleanupRequested_ = false;
        requestDelete();
        return;
    }
    if (fadeCleanupRequested_) {
        fadeCleanupRequested_ = false;
        if (fadeNode_) {
            fadeNode_->requestDelete();
            fadeNode_ = nullptr;
        }
        fadePostAction_ = nullptr;
        runFadePostAction_ = false;
    }
    prepareRender();
    const auto children = children_;
    for (auto *node : children) {
        if (node && node->parent_ == this && !node->deleteRequested_) {
            node->doPrepareRender();
        }
    }
}

void Node::dispatchInputLogic() {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        auto *consumer = root->lastInputConsumer_;
        root->lastInputConsumer_ = nullptr;
        if (!consumer || consumer->rootNode() != root) {
            consumer = this;
        }
        consumer->applyInputLogic();
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::doRender() const {
    render();
    const auto children = children_;
    for (auto *node : children) {
        if (node && node->parent_ == this && !node->deleteRequested_) {
            node->doRender();
        }
    }
}

void Node::consume(const KeyIntent &intent) {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        if (!acceptsInput()) {
            --root->dispatchDepth_;
            return;
        }
        Node *consumer = nullptr;
        for (auto ite = children_.rbegin(); ite != children_.rend(); ++ite) {
            auto *child = *ite;
            if (child && child->parent_ == this && child->acceptsInput()) {
                consumer = child;
                break;
            }
        }
        if (!consumer) {
            root->lastInputConsumer_ = this;
            consumeKeyIntent(intent.key());
        } else {
            consumer->consume(intent);
        }
    } catch (...) {
        --root->dispatchDepth_;
        throw;
    }
    --root->dispatchDepth_;
}

void Node::consume(const TextIntent &intent) {
    auto *root = rootNode();
    ++root->dispatchDepth_;
    try {
        if (!acceptsInput()) {
            --root->dispatchDepth_;
            return;
        }
        Node *consumer = nullptr;
        for (auto ite = children_.rbegin(); ite != children_.rend(); ++ite) {
            auto *child = *ite;
            if (child && child->parent_ == this && child->acceptsInput()) {
                consumer = child;
                break;
            }
        }
        if (!consumer) {
            root->lastInputConsumer_ = this;
            consumeTextIntent(intent.text());
        } else {
            consumer->consume(intent);
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

    // A render-preparation cleanup may destroy a subtree that was marked for
    // deferred deletion during the preceding logic phase.  Remove every
    // pointer in that subtree from the root-owned pending list before
    // destruction; otherwise the next logic barrier could dereference a
    // stale pointer after the presentation tree has been rebuilt.
    const auto purgePendingReferences = [root](auto &&self, Node *node) -> void {
        if (!node) { return; }
        const auto pendingEnd = std::remove(
            root->pendingDeletes_.begin(), root->pendingDeletes_.end(), node);
        root->pendingDeletes_.erase(pendingEnd, root->pendingDeletes_.end());
        if (root->lastInputConsumer_ == node) {
            root->lastInputConsumer_ = nullptr;
        }
        const auto descendants = node->children_;
        for (auto *child : descendants) {
            self(self, child);
        }
    };
    for (auto *n: children_) {
        purgePendingReferences(purgePendingReferences, n);
        if (!n) { continue; }
        n->parent_ = nullptr;
        n->deleteRequested_ = false;
        delete n;
    }
    children_.clear();
}

}
