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
#include "window.hh"

namespace hojy::scene {

MessageBox::~MessageBox() {
    delete cache_;
}

void MessageBox::popup(const std::vector<std::wstring> &text, MessageBox::Type type) {
    makeCache(text);
}

void MessageBox::close() {
    delete cache_;
    cache_ = nullptr;
    removeAllChildren();
}

void MessageBox::render() {
    renderer_->renderTexture(cache_, x_, y_, 0, 0, width_, height_, true);
}

void MessageBox::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeyCancel:
        gWindow->endPopup(true);
        break;
    default:
        break;
    }
}

void MessageBox::makeCache(const std::vector<std::wstring> &text) {
    if (!cache_) {
        cache_ = Texture::createAsTarget(renderer_, width_, height_);
        cache_->enableBlendMode(true);
    }
    auto *ttf = renderer_->ttf();
    int rowHeight = ttf->fontSize() + TextLineSpacing;

    std::vector<std::wstring> lines;
    size_t widthMax = width_ - SubWindowBorder * 2;
    int textW = 0, textH;
    for (auto &l: text) {
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
                lines.emplace_back(l.substr(idx, i - idx));
                idx = i;
                w = width;
            }
        }
        if (idx < len) {
            textW = std::max(textW, int(w));
            lines.emplace_back(l.substr(idx));
        }
    }
    textW += SubWindowBorder * 2;
    textH = rowHeight * int(lines.size()) + SubWindowBorder * 2 + TextLineSpacing;
    int textX = (width_ - textW) / 2;
    int textY = (height_ - textH) / 2;

    renderer_->setTargetTexture(cache_);
    renderer_->fill(0, 0, 0, 0);
    int x = SubWindowBorder + textX;
    int y = SubWindowBorder + textY;
    renderer_->fillRoundedRect(textX, textY, textW, textH, RoundedRectRad, 0, 0, 0, 160);
    ttf->setColor(236, 200, 40);
    for (auto &l: lines) {
        ttf->render(l, x, y, true);
        y += rowHeight;
    }
    renderer_->setTargetTexture(nullptr);
}

}
