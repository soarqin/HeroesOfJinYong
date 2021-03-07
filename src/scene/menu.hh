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
#include <functional>

namespace hojy::scene {

class Menu: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;

    void popup(const std::vector<std::wstring> &items, int defaultIndex = 0);

    void handleKeyInput(Key key) override;

protected:
    virtual void onOK() {}
    virtual void onCancel() {}

private:
    void makeCache() override;

protected:
    std::vector<std::wstring> items_;
    int currIndex_ = 0;
};


class MenuYesNo: public Menu {
public:
    using Menu::Menu;

    void popupWithYesNo();

    void setHandler(const std::function<void()> &yesHandler, const std::function<void()> &noHandler) {
        yesHandler_ = yesHandler;
        noHandler_ = noHandler;
    }

protected:
    void onOK() override;
    void onCancel() override;

private:
    std::function<void()> yesHandler_, noHandler_;
};

}
