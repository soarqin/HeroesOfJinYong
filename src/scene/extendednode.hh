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
#include <chrono>
#include <vector>
#include <tuple>
#include <string>
#include <functional>

namespace hojy::scene {

class Texture;

class ExtendedNode: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;
    void setTimeToClose(int millisec);
    void setWaitForKeyPress();
    void addBox(int x0, int y0, int x1, int y1);
    void addText(int x, int y, const std::wstring &text, int c0, int c1);
    void addTexture(int x, int y, const Texture *tex, std::pair<int, int> scale);
    [[nodiscard]] inline Key keyPressed() const { return keyPressed_; }
    void setHandler(const std::function<void()> &func) { handler_ = func; }
    void checkTimeout();

    void handleKeyInput(Key key) override;

protected:
    void makeCache() override;

private:
    int closeType_ = -1;
    std::chrono::steady_clock::time_point closeDeadline_;
    std::vector<std::tuple<int, int, int, int>> boxlist_;
    std::vector<std::tuple<int, int, std::wstring, int, int>> textlist_;
    std::vector<std::tuple<int, int, const Texture*, std::pair<int, int>>> texturelist_;
    std::function<void()> handler_;
    Key keyPressed_ = KeyNone;
};

}
