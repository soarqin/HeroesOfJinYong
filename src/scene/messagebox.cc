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

namespace hojy::scene {

void MessageBox::popup(const std::vector<std::wstring> &text, Type type, Align align) {
    text_ = text;
    type_ = type;
    align_ = align;
    update();
}

void MessageBox::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        switch (type_) {
        case PressToCloseThis:
            delete this;
            break;
        case PressToCloseTop:
            gWindow->endPopup(true);
            break;
        }
        break;
    default:
        break;
    }
}

void MessageBox::makeCache() {
    auto *ttf = renderer_->ttf();
    int rowHeight = ttf->fontSize() + TextLineSpacing;

    std::vector<std::wstring> lines;
    size_t widthMax = width_ - SubWindowBorder * 2;
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
    textH = rowHeight * int(lines.size()) + SubWindowBorder * 2 - TextLineSpacing;
    if (align_ == Center) {
        x_ += (width_ - textW) / 2;
        y_ += (height_ - textH) / 2;
    }
    width_ = textW;
    height_ = textH;

    NodeWithCache::makeCache();
    renderer_->setTargetTexture(cache_);
    renderer_->fill(0, 0, 0, 0);
    int x = SubWindowBorder;
    int y = SubWindowBorder;
    renderer_->fillRoundedRect(0, 0, textW, textH, RoundedRectRad, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, textW, textH, RoundedRectRad, 224, 224, 224, 255);
    ttf->setColor(236, 200, 40);
    for (auto &l: lines) {
        ttf->render(l, x, y, true);
        y += rowHeight;
    }
    renderer_->setTargetTexture(nullptr);
    text_.clear();

    switch (type_) {
    case YesNo:
        if (menu_ == nullptr) {
            auto mx = x_ + textW + 5, my = y_;
            auto *m = new MenuYesNo(this, mx, my, gWindow->width() - mx, gWindow->height() - my);
            m->popupWithYesNo(true);
            m->setHandler([]{
                gWindow->endPopup(true, true);
            }, [] {
                gWindow->endPopup(true, false);
            });
            menu_ = m;
        }
        break;
    }
}

}
