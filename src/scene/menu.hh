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

    [[nodiscard]] int currIndex() const { return currIndex_; }
    inline void setTitle(const std::wstring &title) { title_ = title; }
    virtual void enableCheckBox(bool b, const std::function<bool(std::int16_t)> &onCheckBoxToggle) {
        checkbox_ = b;
        onCheckBoxToggle_ = b ? onCheckBoxToggle : nullptr;
    }
    inline void enableHorizonal(bool b) { horizonal_ = b; }
    void popup(const std::vector<std::wstring> &items, int defaultIndex = 0);
    void popup(const std::vector<std::wstring> &items, const std::vector<std::wstring> &values, int defaultIndex = 0);
    void checkItem(size_t index, bool check);
    [[nodiscard]] bool itemChecked(size_t index) const;

    void handleKeyInput(Key key) override;

protected:
    virtual void onOK() {}
    virtual void onCancel() {}

private:
    void makeCache() override;

protected:
    std::wstring title_;
    std::vector<std::wstring> items_;
    std::vector<std::wstring> values_;
    std::vector<bool> selected_;
    int currIndex_ = 0;
    bool checkbox_ = false;
    bool horizonal_ = false;
    std::function<bool(std::int16_t)> onCheckBoxToggle_;
};

class MenuTextList: public Menu {
public:
    using Menu::Menu;

    inline void setHandler(const std::function<void()> &okHandler,
                           const std::function<bool()> &cancelHandler = nullptr) {
        okHandler_ = okHandler;
        cancelHandler_ = cancelHandler;
    }

protected:
    void onOK() override;
    void onCancel() override;

protected:
    std::function<void()> okHandler_;
    std::function<bool()> cancelHandler_;
};

class MenuYesNo: public Menu {
public:
    using Menu::Menu;

    void popupWithYesNo();

    void handleKeyInput(Key key) override;

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

class MenuOption: public Menu {
public:
    using Menu::Menu;

    void setValue(int index, const std::wstring &value);
    void setHandler(const std::function<void(int)> &handler) { handler_ = handler; }
    void handleKeyInput(Key key) override;

protected:
    std::function<void(int)> handler_;
};

}
