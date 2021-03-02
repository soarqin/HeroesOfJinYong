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

#include "Renderer.hh"

#include <vector>
#include <unordered_map>
#include <string>

#include <cstdint>

namespace hojy::scene {

class Texture final {
    friend class TextureMgr;

public:
    [[nodiscard]] static Texture *createAsTarget(Renderer *renderer, int w, int h);

public:
    [[nodiscard]] void *data() const { return data_; }
    [[nodiscard]] std::int32_t width() const { return width_; }
    [[nodiscard]] std::int32_t height() const { return height_; }
    [[nodiscard]] std::int32_t originX() const { return originX_; }
    [[nodiscard]] std::int32_t originY() const { return originY_; }

    void enableBlendMode(bool r);

private:
    Texture() = default;
    bool loadFromRLE(Renderer *renderer, const std::string &data, void *palette);

private:
    void *data_ = nullptr;
    std::int32_t width_ = 0, height_ = 0, originX_ = 0, originY_ = 0;
};

class TextureMgr final {
public:
    inline void setRenderer(Renderer *renderer) { renderer_ = renderer; }
    void setPalette(const std::uint32_t *colors, std::size_t size);
    bool loadFromRLE(std::int32_t id, const std::string &data);
    const Texture &operator[](std::int32_t id) const;

private:
    std::unordered_map<std::int32_t, Texture> textures_;
    Renderer *renderer_ = nullptr;
    void *palette_ = nullptr;
};

extern TextureMgr mapTextureMgr;

}
