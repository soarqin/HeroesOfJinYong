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

#include <cstdint>

namespace hojy::scene {

class Texture;

class Renderer final {
    friend class Texture;

public:
    explicit Renderer(void *win);
    Renderer(const Renderer&) = delete;
    ~Renderer();

    void setTargetTexture(Texture *tex);
    void enableBlendMode(bool r);
    void setClipRect(int l, int r, int w, int h);
    void unsetClipRect();
    void fill(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void renderTexture(const Texture *tex, int x, int y, bool ignoreOrigin = false);
    void renderTexture(const Texture *tex, int x, int y, int w, int h, bool ignoreOrigin = false);

    void present();

private:
    void *renderer_ = nullptr;
};

}
