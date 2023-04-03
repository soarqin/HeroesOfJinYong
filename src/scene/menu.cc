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
#include "mem/strings.hh"
#include "core/config.hh"
#include "util/consts.hh"

namespace hojy::scene {

void Menu::popup(const std::vector<std::wstring> &items, int defaultIndex) {
    items_ = items;
    currIndex_ = defaultIndex;
    if (checkbox_) {
        selected_.clear();
        selected_.resize(items_.size(), false);
        items_.emplace_back(GETTEXT(80));
    }
    setDirty();
}

void Menu::popup(const std::vector<std::wstring> &items, const std::vector<std::wstring> &values, int defaultIndex) {
    items_ = items;
    values_ = values;
    currIndex_ = defaultIndex;
    if (checkbox_) {
        selected_.clear();
        selected_.resize(items_.size(), false);
        items_.emplace_back(GETTEXT(80));
        values_.emplace_back(L"");
    }
    setDirty();
}

void Menu::checkItem(size_t index, bool check) {
    if (!checkbox_) { return; }
    if (index >= selected_.size()) { return; }
    selected_[index] = check;
}

bool Menu::itemChecked(size_t index) const {
    if (!checkbox_) { return false; }
    if (index >= selected_.size()) { return false; }
    return selected_[index];
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
        setDirty();
        break;
    case KeyDown:
        if (horizonal_) {
            if (currIndex_ == int(items_.size()) - 1) { break; }
            currIndex_ = int(items_.size()) - 1;
        } else {
            if (++currIndex_ >= items_.size()) { currIndex_ = 0; }
        }
        setDirty();
        break;
    case KeyLeft:
        if (horizonal_) {
            if (--currIndex_ < 0) { currIndex_ = int(items_.size()) - 1; }
        } else {
            if (currIndex_ == 0) { break; }
            currIndex_ = 0;
        }
        setDirty();
        break;
    case KeyRight:
        if (horizonal_) {
            if (++currIndex_ >= items_.size()) { currIndex_ = 0; }
        } else {
            if (currIndex_ == int(items_.size()) - 1) { break; }
            currIndex_ = int(items_.size()) - 1;
        }
        setDirty();
        break;
    case KeyOK: case KeySpace:
        if (checkbox_) {
            if (currIndex_ < 0) { break; }
            if (currIndex_ >= selected_.size()) {
                onOK();
                break;
            }
            if (!onCheckBoxToggle_ || onCheckBoxToggle_(currIndex_)) {
                selected_[currIndex_] = !selected_[currIndex_];
            }
            setDirty();
        } else {
            onOK();
        }
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
    auto windowBorder = core::config.windowBorder();
    int x = windowBorder, y = windowBorder, h, w = 0, wfill = 0, x2 = 0, w2 = 0;
    auto lines = int(items_.size());
    auto fontSize = ttf->fontSize();
    auto rowHeight = fontSize + TextLineSpacing;
    std::vector<std::pair<int, int>> itemsOff;
    bool drawValue = false;
    if (horizonal_) {
        for (auto &s: items_) {
            auto sw = ttf->stringWidth(s);
            itemsOff.emplace_back(std::make_pair(w, sw));
            w += sw + windowBorder;
        }
        w += windowBorder;
        h = rowHeight * (title_.empty() ? 1 : 2) + windowBorder * 2 - TextLineSpacing;
    } else {
        for (auto &s: items_) {
            auto sw = ttf->stringWidth(s);
            w = std::max(w, sw);
        }
        if (!values_.empty()) {
            drawValue = true;
            x2 = x + w + windowBorder;
            for (auto &s: values_) {
                auto sw = ttf->stringWidth(s);
                w2 = std::max(w2, sw);
            }
            w += w2 + windowBorder;
        }
        auto totalLines = lines;
        if (!title_.empty()) {
            ++totalLines;
            w = std::max(w, ttf->stringWidth(title_));
        }
        wfill = w;
        if (checkbox_) {
            auto checkBoxW = ttf->stringWidth(L"*");
            x += checkBoxW;
            x2 += checkBoxW;
            w += checkBoxW;
        }
        w += windowBorder * 2;
        h = rowHeight * totalLines + windowBorder * 2 - TextLineSpacing;
    }
    width_ = w;
    height_ = h;

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 224, 224, 224, 255);
    if (!title_.empty()) {
        ttf->setColor(236, 200, 40);
        ttf->render(title_, x, y, true);
        y += rowHeight;
    }
    if (horizonal_) {
        for (int i = 0; i < lines; ++i) {
            if (i == currIndex_) {
                ttf->setColor(236, 236, 236);
                renderer_->fillRoundedRect(x + itemsOff[i].first - 2, y - 2, itemsOff[i].second + 4, fontSize + 4, 2, 96, 96, 96, 192);
            } else {
                ttf->setColor(252, 148, 16);
            }
            ttf->render(items_[i], x + itemsOff[i].first, y, true);
        }
    } else {
        for (int i = 0; i < lines; ++i, y += rowHeight) {
            if (i == currIndex_) {
                ttf->setColor(236, 236, 236);
                renderer_->fillRoundedRect(x - 2, y - 2, wfill + 4, fontSize + 4, 2, 96, 96, 96, 192);
            } else {
                ttf->setColor(252, 148, 16);
            }
            if (checkbox_ && i < selected_.size() && selected_[i]) {
                ttf->render(L"*", windowBorder, y, true);
            }
            ttf->render(items_[i], x, y, true);
            if (drawValue) {
                ttf->render(values_[i], x2, y, true);
            }
        }
    }
    cacheEnd();
}

void MenuTextList::onOK() {
    if (currIndex_ < 0) { return; }
    if (okHandler_) { okHandler_(); }
}

void MenuTextList::onCancel() {
    if (!cancelHandler_ || cancelHandler_()) {
        delete this;
    }
}

void MenuYesNo::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
    case KeyLeft:
        if (currIndex_ != 0) {
            currIndex_ = 0;
            setDirty();
        }
        break;
    case KeyDown:
    case KeyRight:
        if (currIndex_ != 1) {
            currIndex_ = 1;
            setDirty();
        }
        break;
    default:
        Menu::handleKeyInput(key);
        break;
    }
}

void MenuYesNo::popupWithYesNo() {
    // Default selection should always be the first option in the pop-up menu
    popup({GETTEXT(78), GETTEXT(79)}, hojy::util::consts::DEFAULT_MENU_OPTION);
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

void MenuOption::setValue(int index, const std::wstring &value) {
    if (index < 0 || index >= values_.size()) { return; }
    values_[index] = value;
    setDirty();
}

void MenuOption::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
        if (!horizonal_) {
            break;
        }
        if (handler_) {
            handler_(1);
        }
        return;
    case KeyDown:
        if (!horizonal_) {
            break;
        }
        if (handler_) {
            handler_(2);
        }
        return;
    case KeyLeft:
        if (horizonal_) {
            break;
        }
        if (handler_) {
            handler_(1);
        }
        return;
    case KeyRight:
        if (horizonal_) {
            break;
        }
        if (handler_) {
            handler_(2);
        }
        return;
    case KeyOK: case KeySpace:
        if (handler_) {
            handler_(3);
        }
        return;
    case KeyCancel:
        if (handler_) {
            handler_(0);
        }
        delete this;
        return;
    default:
        break;
    }
    Menu::handleKeyInput(key);
}

}
