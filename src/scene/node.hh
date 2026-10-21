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
#include "logic/input.hh"
#include "logic/command.hh"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace hojy::scene {

class Node : public InputConsumer {
    friend class Window;
public:
    struct LifetimeState final {
        Node *owner = nullptr;
    };

    using LifetimeHandle = std::weak_ptr<LifetimeState>;
    using CommandSink = std::function<void(std::unique_ptr<SceneCommand>)>;
    using Key = InputKey;
    static constexpr Key KeyNone = Key::None;
    static constexpr Key KeyUp = Key::Up;
    static constexpr Key KeyDown = Key::Down;
    static constexpr Key KeyLeft = Key::Left;
    static constexpr Key KeyRight = Key::Right;
    static constexpr Key KeyOK = Key::Accept;
    static constexpr Key KeyCancel = Key::Cancel;
    static constexpr Key KeySpace = Key::Space;
    static constexpr Key KeyBackspace = Key::Backspace;
public:
    Node(Node *parent, int x, int y, int width, int height);
    Node(Renderer *renderer, int x, int y, int width, int height): parent_(nullptr), renderer_(renderer), x_(x), y_(y), width_(width), height_(height) {}
    Node(const Node&) = delete;
    virtual ~Node();
    void add(Node *child);
    void remove(Node *child);
    void setCommandSink(CommandSink sink);
    void postCommand(std::unique_ptr<SceneCommand> command) const;
    void postCommand(std::function<void(SceneCommandContext &)> command) const;
    [[nodiscard]] LifetimeHandle lifetimeHandle();

    void setInputEnabled(bool enabled) noexcept { inputEnabled_ = enabled; }
    [[nodiscard]] virtual bool acceptsInput() const noexcept {
        return inputEnabled_ && !deleteRequested_
            && !presentationCleanupRequested_;
    }

    [[nodiscard]] Node *parent() const { return parent_; }
    void requestDelete();
    // Fixed logic records presentation cleanup; the node tree is changed
    // only when doPrepareRender() owns the presentation phase.
    void requestPresentationCleanup() noexcept {
        presentationCleanupRequested_ = true;
    }
    [[nodiscard]] bool presentationCleanupRequested() const noexcept {
        return presentationCleanupRequested_;
    }
    [[nodiscard]] bool deleteRequested() const { return deleteRequested_; }
    // Used by an external owner (Window/Map) when this node is the root.
    bool consumeDeleteRequest();
    void applyDeferredDeletes();

    void dispatchUpdate() { doUpdate(); }
    void dispatchInputLogic();
    void dispatchPrepareRender() { doPrepareRender(); }
    void dispatchRender() const { doRender(); }

    void consume(const KeyIntent &intent) override;
    void consume(const TextIntent &intent) override;

    [[nodiscard]] inline int x() const { return x_; }
    [[nodiscard]] inline int y() const { return y_; }
    [[nodiscard]] inline int width() const { return width_; }
    [[nodiscard]] inline int height() const { return height_; }
    [[nodiscard]] int rootWidth() const { return rootNode()->width_; }
    [[nodiscard]] int rootHeight() const { return rootNode()->height_; }
    void setPhaseTime(std::uint64_t time) {
        phaseTime_ = time;
        for (auto *child: children_) { if (child) { child->setPhaseTime(time); } }
    }
    [[nodiscard]] std::uint64_t phaseTime() const noexcept { return phaseTime_; }
    inline void setPosition(int x, int y) { x_ = x; y_ = y; }

    void fadeIn(const std::function<void()> &postAction = nullptr);
    void fadeOut(const std::function<void()> &postAction = nullptr);
    void fadeEnd();
    // Logic may invalidate a pending fade, but the fade node is removed only
    // while the render-preparation phase owns the presentation tree.
    void requestFadeCleanup() noexcept { fadeCleanupRequested_ = true; }
    [[nodiscard]] bool fadeCleanupRequested() const noexcept {
        return fadeCleanupRequested_;
    }

    virtual void makeCenter(int w, int h, int x, int y);
    virtual void close() { removeAllChildren(); }
    virtual void update() {}
    virtual void applyInputLogic() {}
    virtual void prepareRender() {}
    virtual void render() const = 0;

protected:
    void doUpdate();
    void doPrepareRender();
    void doRender() const;
    virtual void consumeKeyIntent(InputKey key) {}
    virtual void consumeTextIntent(const std::wstring &str) {}
    void removeAllChildren();

protected:
    Node *parent_ = nullptr;
    Renderer *renderer_ = nullptr;

    int x_, y_, width_, height_;
    bool visible_ = true;

    std::vector<Node*> children_;

    Node *fadeNode_ = nullptr;
    std::function<void()> fadePostAction_;
    bool runFadePostAction_ = false;
    bool fadeCleanupRequested_ = false;
    bool presentationCleanupRequested_ = false;
    bool inputEnabled_ = true;

    bool deleteRequested_ = false;
    std::uint32_t dispatchDepth_ = 0;
    std::vector<Node*> pendingDeletes_;
    // Root-owned pointer to the leaf that consumed the current input intent.
    // It lets the fixed-logic flush target the same consumer even when that
    // consumer requests deletion while handling the intent.
    Node *lastInputConsumer_ = nullptr;
    CommandSink commandSink_;
    std::shared_ptr<LifetimeState> lifetimeState_;
    std::uint64_t phaseTime_ = 0;

    Node *rootNode();
    const Node *rootNode() const;
};

}
