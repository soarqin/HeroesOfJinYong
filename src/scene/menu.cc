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

#include "menu.hh"

#include "window.hh"

namespace hojy::scene {

void Menu::popup(const std::vector<std::wstring> &items, int defaultIndex, bool horizonal) {
    items_ = items;
    currIndex_ = defaultIndex;
    horizonal_ = horizonal;
    update();
}

void Menu::handleKeyInput(Key key) {
    switch (key) {
    case KeyUp:
        if (horizonal_) {
            if (currIndex_ == 0) { break; }
            currIndex_ = 0;
        } else {
            if (--currIndex_ < 0) { currIndex_ = int(items_.size()) - 1; }
        }
        update();
        break;
    case KeyDown:
        if (horizonal_) {
            if (currIndex_ == int(items_.size()) - 1) { break; }
            currIndex_ = int(items_.size()) - 1;
        } else {
            if (++currIndex_ >= items_.size()) { currIndex_ = 0; }
        }
        update();
        break;
    case KeyLeft:
        if (horizonal_) {
            if (--currIndex_ < 0) { currIndex_ = int(items_.size()) - 1; }
        } else {
            if (currIndex_ == 0) { break; }
            currIndex_ = 0;
        }
        update();
        break;
    case KeyRight:
        if (horizonal_) {
            if (++currIndex_ >= items_.size()) { currIndex_ = 0; }
        } else {
            if (currIndex_ == int(items_.size()) - 1) { break; }
            currIndex_ = int(items_.size()) - 1;
        }
        update();
        break;
    case KeyOK: case KeySpace:
        onOK();
        break;
    case KeyCancel:
        onCancel();
        break;
    default:
        break;
    }
}

void Menu::makeCache() {
    auto *ttf = renderer_->ttf();
    int x = 0, y = 0, h = 0, w = 0;
/*
    int itemsTW = 0;
*/
    auto lines = int(items_.size());
    auto rowHeight = ttf->fontSize() + TextLineSpacing;
    std::vector<int> itemsOff;
    if (horizonal_) {
        for (auto &s: items_) {
            itemsOff.emplace_back(w);
            w += ttf->stringWidth(s) + SubWindowBorder;
        }
        w += SubWindowBorder;
        h = rowHeight * (title_.empty() ? 1 : 2) + SubWindowBorder * 2 - TextLineSpacing;
    } else {
        auto totalLines = lines;
        if (!title_.empty()) {
            ++totalLines;
            w/* = itemsTW*/ = ttf->stringWidth(title_);
        }
/*
        itemsOff.reserve(items_.size());
*/
        for (auto &s: items_) {
            auto sw = ttf->stringWidth(s);
/*
            itemsOff.emplace_back(sw);
*/
            w = std::max(w, sw);
        }
/*
        int nw = w;
*/
        w += SubWindowBorder * 2;
        h = rowHeight * totalLines + SubWindowBorder * 2 - TextLineSpacing;
/* TODO: support centered menu?
    x = (width_ - w) / 2;
    y = (height_ - h) / 2;
*/
    }
    width_ = w;
    height_ = h;

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(x, y, w, h, RoundedRectRad, 64, 64, 64, 208);
    renderer_->drawRoundedRect(x, y, w, h, RoundedRectRad, 224, 224, 224, 255);
    x += SubWindowBorder; y += SubWindowBorder;
    if (!title_.empty()) {
        ttf->setColor(236, 200, 40);
        ttf->render(title_, x/* + (nw - itemsTW) / 2*/, y, true);
        y += rowHeight;
    }
    if (horizonal_) {
        for (int i = 0; i < lines; ++i) {
            if (i == currIndex_) {
                ttf->setColor(236, 236, 236);
            } else {
                ttf->setColor(252, 148, 16);
            }
            ttf->render(items_[i], x + itemsOff[i], y, true);
        }
    } else {
        for (int i = 0; i < lines; ++i, y += rowHeight) {
            if (i == currIndex_) {
                ttf->setColor(236, 236, 236);
            } else {
                ttf->setColor(252, 148, 16);
            }
            ttf->render(items_[i], x/* + (nw - itemsOff[i]) / 2*/, y, true);
        }
    }
    cacheEnd();
}

void MenuTextList::onOK() {
    if (currIndex_ < 0) { return; }
    okHandler_(currIndex_);
}

void MenuTextList::onCancel() {
    cancelHandler_();
}

void MenuYesNo::popupWithYesNo(bool horizonal) {
    popup({L"是", L"否"}, -1, horizonal);
}

void MenuYesNo::onOK() {
    if (currIndex_ < 0) { return; }
    if (currIndex_ == 0) {
        yesHandler_();
    } else {
        noHandler_();
    }
}

void MenuYesNo::onCancel() {
    noHandler_();
}

}
