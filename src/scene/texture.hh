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
#include <cstdint>

namespace hojy::scene {

class Renderer;
class ColorPalette;

class Texture {
    friend class TextureMgr;

public:
    [[nodiscard]] static Texture *createAsTarget(Renderer *renderer, int w, int h);
    [[nodiscard]] static Texture *create(Renderer *renderer, std::int16_t w, std::int16_t h);

public:
    Texture() = default;
    virtual ~Texture();
    Texture(const Texture &tex) = delete;
    Texture(Texture &&other) noexcept;
    Texture& operator=(Texture &&other) noexcept;

    [[nodiscard]] void *data() const { return data_; }
    [[nodiscard]] virtual std::int16_t x() const { return 0; }
    [[nodiscard]] virtual std::int16_t y() const { return 0; }
    [[nodiscard]] std::int16_t width() const { return width_; }
    [[nodiscard]] std::int16_t height() const { return height_; }
    [[nodiscard]] std::int16_t originX() const { return originX_; }
    [[nodiscard]] std::int16_t originY() const { return originY_; }

    void enableBlendMode(bool r);
    void setBlendColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    std::uint32_t *lock(int &pitch);
    std::uint32_t *lock(int &pitch, int x, int y, int w, int h);
    void unlock();

    static Texture *loadFromRLE(Renderer *renderer, const std::string &data, const ColorPalette &palette);
    static Texture *loadFromRAW(Renderer *renderer, const std::string &data, int width, int height, const ColorPalette &palette);
    static void renderRLE(const std::string &data, const std::uint32_t *colors, std::uint32_t *pixels, int pitch, int height, int x, int y, bool ignoreOrigin = false);
    static void renderRLEBlending(const std::string &data, const std::uint32_t *colors, std::uint32_t *pixels, int pitch, int height, int x, int y, bool ignoreOrigin = false);
    static std::uint32_t calcRLEAvgColor(const std::string &data, const std::uint32_t *colors);

protected:
    void *data_ = nullptr;
    std::int16_t width_ = 0, height_ = 0, originX_ = 0, originY_ = 0;
};

class TextureSlice final: public Texture {
public:
    TextureSlice(Texture *tex, std::int16_t x, std::int16_t y, std::int16_t w, std::int16_t h, std::int16_t ox = 0, std::int16_t oy = 0);
    ~TextureSlice() override;
    [[nodiscard]] std::int16_t x() const override { return x_; }
    [[nodiscard]] std::int16_t y() const override { return y_; }

private:
    std::int16_t x_ = 0, y_ = 0;
};

class RectPacker;

class TextureMgr final {
public:
    TextureMgr();
    ~TextureMgr();
    inline void setRenderer(Renderer *renderer) { renderer_ = renderer; }
    void setPalette(const ColorPalette &col);
    Texture *loadFromRLE(const std::string &data, std::int16_t index);
    void loadFromRLE(const std::vector<std::string> &data);
    Texture *loadFromRAW(const std::string &data, int width, int height, std::int16_t index);
    void loadFromRAW(const std::vector<std::string> &data, int width, int height);
    const Texture *operator[](std::int32_t id) const;
    const Texture *last() const;
    std::int32_t idMax() const { return textureIdMax_; }
    void clear();

private:
    std::unordered_map<std::int32_t, Texture*> textures_;
    std::vector<Texture*> textureContainers_;
    RectPacker *rectPacker_ = nullptr;
    std::int32_t textureIdMax_ = 0;
    Renderer *renderer_ = nullptr;
    const ColorPalette *palette_ = nullptr;
};

}
