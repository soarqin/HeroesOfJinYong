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

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace hojy::scene {

class Renderer;
class ColorPalette;

class Texture final {
    friend class TextureMgr;

public:
    [[nodiscard]] static Texture *createAsTarget(Renderer *renderer, int w, int h);
    [[nodiscard]] static Texture *create(Renderer *renderer, std::int16_t w, std::int16_t h);

public:
    Texture() = default;
    ~Texture();
    Texture(const Texture &tex) = delete;
    Texture(Texture &&other) noexcept;
    Texture& operator=(Texture &&other) noexcept;

    [[nodiscard]] void *data() const { return data_; }
    [[nodiscard]] std::int16_t width() const { return width_; }
    [[nodiscard]] std::int16_t height() const { return height_; }
    [[nodiscard]] std::int16_t originX() const { return originX_; }
    [[nodiscard]] std::int16_t originY() const { return originY_; }

    void enableBlendMode(bool r);
    void setBlendColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);

    static Texture *loadFromRLE(Renderer *renderer, const std::string &data, const ColorPalette &palette);
    static Texture *loadFromRAW(Renderer *renderer, const std::string &data, int width, int height, const ColorPalette &palette);

private:
    void *data_ = nullptr;
    std::int16_t width_ = 0, height_ = 0, originX_ = 0, originY_ = 0;
};

class TextureMgr final {
public:
    inline void setRenderer(Renderer *renderer) { renderer_ = renderer; }
    void setPalette(const ColorPalette &col);
    bool loadFromRLE(const std::vector<std::string> &data);
    bool mergeFromRLE(const std::vector<std::string> &data);
    bool loadFromRAW(const std::vector<std::string> &data, int width, int height);
    const Texture *operator[](std::int32_t id) const;
    const Texture *last() const;
    std::int32_t max() const { return textureIdMax_; }
    void clear() { textures_.clear(); }

private:
    std::unordered_map<std::int32_t, std::unique_ptr<Texture>> textures_;
    std::int32_t textureIdMax_ = 0;
    Renderer *renderer_ = nullptr;
    const ColorPalette *palette_ = nullptr;
};

}
