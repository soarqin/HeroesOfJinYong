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

#include "ttf.hh"

#include <cstdint>

namespace hojy::scene {

class Texture;

class Renderer final {
    friend class Texture;

public:
    explicit Renderer(void *win, int w, int h);
    Renderer(const Renderer&) = delete;
    ~Renderer();

    void enableLinear(bool linear = true);
    void setTargetTexture(Texture *tex);
    void clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void fillRect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void drawRect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void fillRoundedRect(int x, int y, int w, int h, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void drawRoundedRect(int x, int y, int w, int h, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void drawCircle(int x, int y, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void fillCircle(int x, int y, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void renderTexture(const Texture *tex, int x, int y, bool ignoreOrigin = false);
    void renderTexture(const Texture *tex, int x, int y, std::pair<int, int> scale, bool ignoreOrigin = false);
    void renderTexture(const Texture *tex, int destx, int desty, int x, int y, int w, int h, bool ignoreOrigin = false);
    void renderTexture(const Texture *tex, int destx, int desty, int destw, int desth, int x, int y, int w, int h, bool ignoreOrigin = false);

    bool canRender();
    void present();
    [[nodiscard]] inline TTF *ttf() { return ttf_; }
    [[nodiscard]] inline float fps() const { return fps_; }
    [[nodiscard]] std::uint64_t nextRenderTime() const { return nextRenderTime_; }

private:
    float fps_ = 0.f;
    void *renderer_ = nullptr;
    TTF *ttf_ = nullptr;

    int frameCount_ = 0;
    std::uint64_t nextCountTime_ = 0;
    std::uint64_t nextRenderTime_ = 0;
    std::uint64_t renderInterval_ = 0;
};

}
