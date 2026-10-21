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

#include "nodewithcache.hh"
#include "logic/talk_layout.hh"

#include <memory>
#include <utility>

namespace hojy::scene {

struct MessageBoxResult final {
    bool accepted = false;
};

class MessageBoxResultSink {
public:
    virtual ~MessageBoxResultSink() = default;
    virtual void submit(MessageBoxResult result) = 0;
};

template<typename Function>
class MessageBoxResultSinkAdapter final: public MessageBoxResultSink {
public:
    explicit MessageBoxResultSinkAdapter(Function function):
        function_(std::move(function)) {}

    void submit(MessageBoxResult result) override {
        function_(std::move(result));
    }

private:
    Function function_;
};

template<typename Function>
std::shared_ptr<MessageBoxResultSink> makeMessageBoxResultSink(
        Function function) {
    return std::make_shared<MessageBoxResultSinkAdapter<Function>>(
        std::move(function));
}

class MessageBox: public NodeWithCache {
public:
    enum Type {
        Normal,
        PressToCloseThis,
        PressToCloseParent,
        PressToCloseTop,
        YesNo,
    };
    enum Align {
        Center,
        TopLeft,
    };

public:
    MessageBox(Node *parent, int x, int y, int width, int height):
        NodeWithCache(parent, x, y, width, height) {}
    MessageBox(Renderer *renderer, int x, int y, int width, int height):
        NodeWithCache(renderer, x, y, width, height) {}

    void setResultSink(std::shared_ptr<MessageBoxResultSink> sink) {
        resultSink_ = std::move(sink);
    }
    void executeYesNoSelection(bool yes);
    void popup(const std::vector<std::wstring> &text, Type type = Normal, Align align = Center);
    void update() override;
    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;
    void prepareRender() override;

protected:
    bool prepareTextResources() override;
    void ensureLayout() override;
    void makeCache() override;
    bool buildLayoutSnapshot();
    void ensureYesNoMenu();
    void submitResult(MessageBoxResult result);

protected:
    std::vector<std::wstring> text_;
    Node *menu_ = nullptr;
    Type type_ = Normal;
    Align align_ = Center;
    std::shared_ptr<MessageBoxResultSink> resultSink_;
    bool layoutReady_ = false;
    // Popup replacement records stale choice-menu cleanup for the
    // presentation preparation phase; fixed logic never mutates the node tree.
    bool presentationMenuCleanupRequested_ = false;
    bool presentationParentCleanupRequested_ = false;
    bool frameInitialized_ = false;
    int frameX_ = 0, frameY_ = 0, frameWidth_ = 0, frameHeight_ = 0;
    logic::TextBlockLayout layout_;
    Key pendingInput_ = KeyNone;
};

}
