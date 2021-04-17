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

#include "extendednode.hh"

#include "colorpalette.hh"
#include "window.hh"
#include "core/config.hh"

namespace hojy::scene {

void ExtendedNode::setTimeToClose(std::uint32_t millisec) {
    if (millisec <= 0) {
        closeType_ = 0;
        closeDeadline_ = 0;
    } else {
        closeType_ = 0;
        closeDeadline_ = gWindow->currTime() + millisec * 1000ULL;
    }
}

void ExtendedNode::setWaitForKeyPress() {
    closeType_ = 1;
}

void ExtendedNode::addBox(int x0, int y0, int x1, int y1) {
    boxlist_.emplace_back(std::make_tuple(x0, y0, x1 - x0 + 1, y1 - y0 + 1));
    setDirty();
}

void ExtendedNode::addText(int x, int y, const std::wstring &text, int c0, int c1) {
    textlist_.emplace_back(std::make_tuple(x, y, text, c0, c1));
    setDirty();
}

void ExtendedNode::addTexture(int x, int y, const Texture *tex, std::pair<int, int> scale) {
    if (!tex) { return; }
    texturelist_.emplace_back(std::make_tuple(x, y, tex, scale));
    setDirty();
}

void ExtendedNode::checkTimeout() {
    if (closeType_ == 0 && gWindow->currTime() >= closeDeadline_) {
        if (handler_) { handler_(); }
        delete this;
    }
}

void ExtendedNode::handleKeyInput(Node::Key key) {
    if (closeType_ != 0) { return; }
    keyPressed_ = key;
    if (handler_) { handler_(); }
    delete this;
}

void ExtendedNode::makeCache() {
    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    auto windowBorder = core::config.windowBorder();
    for (auto &p: boxlist_) {
        int x0, y0, w, h;
        std::tie(x0, y0, w, h) = p;
        renderer_->drawRoundedRect(x0, y0, w, h, windowBorder, 224, 224, 224, 255);
    }
    auto *ttf = renderer_->ttf();
    for (auto &p: textlist_) {
        auto c = std::get<3>(p);
        if (c >= 0 && c < gNormalPalette.size()) {
            std::uint32_t color = gNormalPalette.colors()[c];
            auto *colorptr = reinterpret_cast<const uint8_t*>(&color);
            ttf->setColor(colorptr[0], colorptr[1], colorptr[2]);
        }
        ttf->render(std::get<2>(p), std::get<0>(p), std::get<1>(p), true);
    }
    for (auto &p: texturelist_) {
        int x, y; const Texture *tex; std::pair<int, int> scale;
        std::tie(x, y, tex, scale) = p;
        renderer_->renderTexture(tex, x, y, scale);
    }
    cacheEnd();
}

}
