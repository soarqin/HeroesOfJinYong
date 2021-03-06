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

namespace hojy::scene {

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
    using NodeWithCache::NodeWithCache;

    inline void setCloseHandler(const std::function<void()> &closeHandler) {
        closeHandler_ = closeHandler;
    }
    inline void setYesNoHandler(const std::function<void()> &yesHandler, const std::function<void()> &noHandler) {
        yesHandler_ = yesHandler;
        noHandler_ = noHandler;
    }
    void popup(const std::vector<std::wstring> &text, Type type = Normal, Align align = Center);
    void handleKeyInput(Key key) override;

protected:
    void makeCache() override;

protected:
    std::vector<std::wstring> text_;
    Node *menu_ = nullptr;
    Type type_ = Normal;
    Align align_ = Center;
    std::function<void()> closeHandler_, yesHandler_, noHandler_;
};

}
