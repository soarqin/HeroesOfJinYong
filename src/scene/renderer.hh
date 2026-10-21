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

    [[nodiscard]] bool ready() const noexcept { return ready_; }

    void enableLinear(bool linear = true);
    bool setTargetTexture(Texture *tex);
    [[nodiscard]] Texture *targetTexture() const { return targetTexture_; }
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

    bool canRender(std::uint64_t now);
    void present();
    [[nodiscard]] inline TTF *ttf() { return ttf_; }
    [[nodiscard]] inline int fontSize() const noexcept { return fontSize_; }
    void setItemAtlas(const Texture *texture, int cellWidth, int cellHeight) noexcept {
        itemAtlas_ = texture;
        itemTexW_ = cellWidth;
        itemTexH_ = cellHeight;
    }
    [[nodiscard]] const Texture *itemAtlas() const noexcept { return itemAtlas_; }
    [[nodiscard]] int itemTexWidth() const noexcept { return itemTexW_; }
    [[nodiscard]] int itemTexHeight() const noexcept { return itemTexH_; }
    [[nodiscard]] inline float fps() const { return fps_; }
    [[nodiscard]] std::uint64_t nextRenderTime() const { return nextRenderTime_; }

private:
    float fps_ = 0.f;
    void *renderer_ = nullptr;
    TTF *ttf_ = nullptr;
    bool ready_ = false;
    Texture *targetTexture_ = nullptr;
    int fontSize_ = 16;
    const Texture *itemAtlas_ = nullptr;
    int itemTexW_ = 0, itemTexH_ = 0;

    int frameCount_ = 0;
    std::uint64_t nextCountTime_ = 0;
    std::uint64_t nextRenderTime_ = 0;
    std::uint64_t renderInterval_ = 0;
};

/** Restores the caller's render target even when preparation aborts. */
class RenderTargetGuard final {
public:
    RenderTargetGuard(Renderer *renderer, Texture *target): renderer_(renderer) {
        if (!renderer_) { return; }
        previous_ = renderer_->targetTexture();
        active_ = renderer_->setTargetTexture(target);
    }
    RenderTargetGuard(const RenderTargetGuard &) = delete;
    RenderTargetGuard &operator=(const RenderTargetGuard &) = delete;
    ~RenderTargetGuard() {
        if (active_ && renderer_) {
            restored_ = renderer_->setTargetTexture(previous_);
            if (!restored_ && renderer_->targetTexture() != previous_) {
                // A failed restoration must not leave a soon-to-be-destroyed
                // candidate bound as the active render target.
                restored_ = renderer_->setTargetTexture(nullptr);
            }
        }
    }

    [[nodiscard]] bool valid() const noexcept { return active_; }
    [[nodiscard]] bool restored() const noexcept { return restored_; }
    void release() noexcept { active_ = false; }

private:
    Renderer *renderer_ = nullptr;
    Texture *previous_ = nullptr;
    bool active_ = false;
    bool restored_ = true;
};

}
