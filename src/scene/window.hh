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

#include "map.hh"
#include "renderer.hh"

namespace hojy::scene {

class Window final {
public:
    Window(int w, int h);
    ~Window();

    [[nodiscard]] Renderer *renderer() {
        return renderer_;
    }

    bool processEvents();
    void render();

    void flush();

private:
    void *win_ = nullptr;
    Renderer *renderer_ = nullptr;
    Map *map_ = nullptr;
    Node *topNode_ = nullptr;
};

}
