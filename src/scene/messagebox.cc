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

void MessageBox::popup(const std::vector<std::wstring> &text, Type type, Align align) {
    if (!layoutReady_) {
        frameX_ = x_;
        frameY_ = y_;
        frameWidth_ = width_;
        frameHeight_ = height_;
    }
    x_ = frameX_;
    y_ = frameY_;
    width_ = frameWidth_;
    height_ = frameHeight_;
    text_ = text;
    type_ = type;
    align_ = align;
    layoutReady_ = false;
    if (type_ != YesNo && menu_) {
        menu_->requestDelete();
        menu_ = nullptr;
    }
    setDirty();
}

void MessageBox::update() {
    if (!layoutReady_) {
        layoutText();
    }
    if (type_ == YesNo) {
        ensureYesNoMenu();
    }
    NodeWithCache::update();
}

void MessageBox::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        switch (type_) {
        case PressToCloseThis: {
            auto fn = std::move(closeHandler_);
            requestDelete();
            if (fn) { fn(); }
            break;
        }
        case PressToCloseParent: {
            auto fn = std::move(closeHandler_);
            if (parent_) { parent_->requestDelete(); }
            if (fn) { fn(); }
            break;
        }
        case PressToCloseTop: {
            auto fn = std::move(closeHandler_);
            gWindow->endPopup(true);
            if (fn) { fn(); }
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

void MessageBox::layoutText() {
    auto *ttf = renderer_->ttf();
    int rowHeight = ttf->fontSize() + TextLineSpacing;
    auto windowBorder = core::config.windowBorder();

    lines_.clear();
    const int widthMax = std::max(0, frameWidth_ - windowBorder * 2);
    int textW = 0, textH;
    for (auto &l: text_) {
        size_t w = 0;
        size_t len = l.length();
        size_t idx = 0;
        for (size_t i = 0; i < len; ++i) {
            auto ch = l[i];
            std::uint8_t width;
            std::int8_t y0, y1;
            ttf->charDimension(ch, width, y0, y1);
            w += width;
            if (w > widthMax) {
                textW = std::max(textW, int(w - width));
                lines_.emplace_back(l.substr(idx, i - idx));
                idx = i;
                w = width;
            }
        }
        if (idx < len) {
            textW = std::max(textW, int(w));
            lines_.emplace_back(l.substr(idx));
        }
    }
    textW += windowBorder * 2;
    textH = rowHeight * int(lines_.size()) + windowBorder * 2 - TextLineSpacing;
    textWidth_ = textW;
    textHeight_ = textH;
    x_ = frameX_;
    y_ = frameY_;
    width_ = frameWidth_;
    height_ = frameHeight_;
    if (align_ == Center) {
        x_ += (frameWidth_ - textW) / 2;
        y_ += (frameHeight_ - textH) / 2;
    }
    width_ = textW;
    height_ = textH;
    layoutReady_ = true;
}

void MessageBox::ensureYesNoMenu() {
    if (menu_ != nullptr || type_ != YesNo) { return; }
    auto *m = new MenuYesNo(this, x_ + textWidth_ + 5, y_,
                            gWindow->width() - (x_ + textWidth_ + 5),
                            gWindow->height() - y_);
    m->enableHorizonal(true);
    m->popupWithYesNo();
    m->setHandler([this]{
        if (yesHandler_) {
            yesHandler_();
        } else {
            gWindow->endPopup(true, true);
        }
    }, [this] {
        if (noHandler_) {
            noHandler_();
        } else {
            gWindow->endPopup(true, false);
        }
    });
    menu_ = m;
}

void MessageBox::makeCache() {
    if (!layoutReady_) {
        layoutText();
    }
    auto *ttf = renderer_->ttf();
    int rowHeight = ttf->fontSize() + TextLineSpacing;
    auto windowBorder = core::config.windowBorder();

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    int x = windowBorder;
    int y = windowBorder;
    renderer_->fillRoundedRect(0, 0, textWidth_, textHeight_, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, textWidth_, textHeight_, windowBorder, 224, 224, 224, 255);
    ttf->setColor(236, 200, 40);
    for (auto &l: lines_) {
        ttf->render(l, x, y, true);
        y += rowHeight;
    }
    cacheEnd();
}

}
