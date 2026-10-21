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

#include "messagebox.hh"

#include "texture.hh"
#include "menu.hh"
#include "window.hh"
#include "core/config.hh"

namespace hojy::scene {

namespace {

class MessageBoxChoiceAction final: public MenuAction {
public:
    MessageBoxChoiceAction(MessageBox *owner, bool yes): owner_(owner), yes_(yes) {}

    void execute(MenuSelection) override {
        if (!owner_) { return; }
        owner_->executeYesNoSelection(yes_);
    }

private:
    MessageBox *owner_ = nullptr;
    bool yes_ = false;
};

}

void MessageBox::executeYesNoSelection(bool yes) {
    if (resultSink_) {
        submitResult({yes});
        return;
    }
    postCommand([yes](SceneCommandContext &context) {
        context.endPopup(true, yes);
    });
}

void MessageBox::submitResult(MessageBoxResult result) {
    auto sink = std::move(resultSink_);
    if (sink) { sink->submit(std::move(result)); }
}

void MessageBox::popup(const std::vector<std::wstring> &text, Type type, Align align) {
    if (!frameInitialized_) {
        frameX_ = x_;
        frameY_ = y_;
        frameWidth_ = width_;
        frameHeight_ = height_;
        frameInitialized_ = true;
    }
    x_ = frameX_;
    y_ = frameY_;
    width_ = frameWidth_;
    height_ = frameHeight_;
    text_ = text;
    type_ = type;
    align_ = align;
    if (menu_) { presentationMenuCleanupRequested_ = true; }
    layout_ = {};
    layoutReady_ = false;
    requestPresentationRefresh();
}

void MessageBox::update() {
    NodeWithCache::update();
}

void MessageBox::prepareRender() {
    if (presentationParentCleanupRequested_) {
        presentationParentCleanupRequested_ = false;
        if (parent_) { parent_->requestPresentationCleanup(); }
    }
    NodeWithCache::prepareRender();
}

bool MessageBox::prepareTextResources() {
    if (!renderer_ || !renderer_->ttf()) { return false; }
    auto *ttf = renderer_->ttf();
    bool ready = true;
    for (const auto &line: text_) { ready = ttf->prepareText(line) && ready; }
    return ttf->prepareText(L"*") && ready;
}

void MessageBox::ensureLayout() {
    if (presentationMenuCleanupRequested_) {
        if (menu_) {
            menu_->requestDelete();
            menu_ = nullptr;
        }
        presentationMenuCleanupRequested_ = false;
    }
    if (layoutReady_) { return; }
    if (!buildLayoutSnapshot()) {
        layout_.lines = text_;
        layout_.width = std::max(0, frameWidth_);
        layout_.height = std::max(0, frameHeight_);
        x_ = frameX_;
        y_ = frameY_;
        width_ = frameWidth_;
        height_ = frameHeight_;
    }
    layoutReady_ = true;
    if (type_ == YesNo) { ensureYesNoMenu(); }
}

void MessageBox::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void MessageBox::applyInputLogic() {
    const auto key = pendingInput_;
    pendingInput_ = KeyNone;
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        switch (type_) {
        case PressToCloseThis: {
            requestPresentationCleanup();
            submitResult({true});
            break;
        }
        case PressToCloseParent: {
            presentationParentCleanupRequested_ = true;
            submitResult({true});
            break;
        }
        case PressToCloseTop: {
            postCommand([](SceneCommandContext &context) {
                context.endPopup(true);
            });
            submitResult({true});
            break;
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
}

bool MessageBox::buildLayoutSnapshot() {
    if (!renderer_ || !renderer_->ttf()) { return false; }
    auto *ttf = renderer_->ttf();
    const auto rowHeight = renderer_->fontSize() + TextLineSpacing;
    const auto windowBorder = core::config.windowBorder();
    const auto maximumLineWidth = std::max(
        0, frameWidth_ - windowBorder * 2);
    const auto metricRequest = logic::collectTextMetricRequest(text_);
    logic::TextMetricsSnapshot metrics;
    for (const auto ch: metricRequest.characters) {
        int advance = 0;
        if (!ttf->measureCharAdvance(ch, advance)) { return false; }
        metrics.advances.emplace(ch, advance);
    }
    for (const auto &pair: metricRequest.pairs) {
        int adjustment = 0;
        if (ttf->measureKerningAdvance(
                pair.first, pair.second, adjustment)) {
            metrics.pairAdjustments.emplace(pair, adjustment);
        }
    }
    logic::TextBlockLayout candidate;
    if (!logic::buildTextBlockLayout(
            text_, maximumLineWidth, rowHeight, TextLineSpacing,
            windowBorder, metrics, candidate)) {
        return false;
    }
    x_ = frameX_;
    y_ = frameY_;
    width_ = frameWidth_;
    height_ = frameHeight_;
    if (align_ == Center) {
        x_ += (frameWidth_ - candidate.width) / 2;
        y_ += (frameHeight_ - candidate.height) / 2;
    }
    width_ = candidate.width;
    height_ = candidate.height;
    layout_ = std::move(candidate);
    return true;
}

void MessageBox::ensureYesNoMenu() {
    if (menu_ != nullptr || type_ != YesNo) { return; }
    const auto menuX = x_ + layout_.width + 5;
    auto *m = new MenuYesNo(this, menuX, y_,
                            frameX_ + frameWidth_ - menuX,
                            frameY_ + frameHeight_ - y_);
    m->enableHorizonal(true);
    m->popupWithYesNo();
    auto controller = std::make_shared<ActionMenuController>();
    controller->bind(0, std::make_unique<MessageBoxChoiceAction>(this, true));
    controller->bind(1, std::make_unique<MessageBoxChoiceAction>(this, false));
    controller->bindCancel(
        std::make_unique<MessageBoxChoiceAction>(this, false));
    m->setSelectionSink(std::move(controller));
    menu_ = m;
}

void MessageBox::makeCache() {
    auto *ttf = renderer_->ttf();
    int rowHeight = ttf->fontSize() + TextLineSpacing;
    auto windowBorder = core::config.windowBorder();

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    int x = windowBorder;
    int y = windowBorder;
    renderer_->fillRoundedRect(0, 0, layout_.width, layout_.height, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, layout_.width, layout_.height, windowBorder, 224, 224, 224, 255);
    ttf->setColor(236, 200, 40);
    for (const auto &l: layout_.lines) {
        ttf->renderPrepared(l, x, y, true);
        y += rowHeight;
    }
    cacheEnd();
}

}
