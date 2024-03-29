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

#include "renderer.hh"

#include "window.hh"
#include "core/config.hh"
#include <SDL2_gfxPrimitives.h>

namespace hojy::scene {

Renderer::Renderer(void *win, int w, int h):
    renderer_(SDL_CreateRenderer(static_cast<SDL_Window*>(win), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE)),
    ttf_(new TTF(this)) {
    if (core::config.limitFPS() > 0) {
        renderInterval_ = 1000 * 1000;
        renderInterval_ /= core::config.limitFPS();
    } else {
        renderInterval_ = 0;
    }
    SDL_SetRenderDrawBlendMode(static_cast<SDL_Renderer*>(renderer_), SDL_BLENDMODE_BLEND);
    int fontSize;
    if (w * 3 > h * 4) {
        fontSize = h / 48 * 2;
    } else {
        fontSize = w * 3 / 4 / 48 * 2;
    }
    ttf_->init(fontSize);
    for (const auto &f: core::config.fonts()) {
        ttf_->add(f);
    }
}

Renderer::~Renderer() {
    delete ttf_;
    SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
}

void Renderer::enableLinear(bool linear) {
    (void)this;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, linear ? "linear" : "nearest");
}

void Renderer::setTargetTexture(Texture *tex) {
    SDL_SetRenderTarget(static_cast<SDL_Renderer*>(renderer_), tex ? static_cast<SDL_Texture*>(tex->data()) : nullptr);
}

void Renderer::clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_RenderClear(ren);
}

void Renderer::fillRect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    boxRGBA(ren, x, y, x + w - 1, y + h - 1, r, g, b, a);
}

void Renderer::drawRect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    rectangleRGBA(ren, x, y, x + w - 1, y + h - 1, r, g, b, a);
}

void Renderer::fillRoundedRect(int x, int y, int w, int h, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    roundedBoxRGBA(static_cast<SDL_Renderer*>(renderer_), x, y, x + w - 1, y + h - 1, rad, r, g, b, a);
}

void Renderer::drawRoundedRect(int x, int y, int w, int h, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    roundedRectangleRGBA(static_cast<SDL_Renderer*>(renderer_), x, y, x + w - 1, y + h - 1, rad, r, g, b, a);
}

void Renderer::drawCircle(int x, int y, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    circleRGBA(static_cast<SDL_Renderer*>(renderer_), x, y, rad, r, g, b, a);
}

void Renderer::fillCircle(int x, int y, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    filledCircleRGBA(static_cast<SDL_Renderer*>(renderer_), x, y, rad, r, g, b, a);
}

void Renderer::renderTexture(const Texture *tex, int x, int y, bool ignoreOrigin) {
    auto w = tex->width(), h = tex->height();
    SDL_Rect src {tex->x(), tex->y(), w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {x, y, w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{x - tex->originX(), y - tex->originY(), w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::renderTexture(const Texture *tex, int x, int y, std::pair<int, int> scale, bool ignoreOrigin) {
    auto w = tex->width(), h = tex->height();
    SDL_Rect src {tex->x(), tex->y(), w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {x, y, w * scale.first / scale.second, h * scale.first / scale.second};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{x - tex->originX() * scale.first / scale.second, y - tex->originY() * scale.first / scale.second,
                     w * scale.first / scale.second, h * scale.first / scale.second};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::renderTexture(const Texture *tex, int destx, int desty, int x, int y, int w, int h, bool ignoreOrigin) {
    SDL_Rect src {tex->x() + x, tex->y() + y, w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {destx, desty, w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{destx - tex->originX(), desty - tex->originY(), w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::renderTexture(const Texture *tex, int destx, int desty, int destw, int desth, int x, int y, int w, int h, bool ignoreOrigin) {
    SDL_Rect src {tex->x() + x, tex->y() + y, w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {destx, desty, destw, desth};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{destx - tex->originX() * destw / w, desty - tex->originY() * desth / h, destw, desth};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

bool Renderer::canRender() {
    auto now = gWindow->currTime();
    if (renderInterval_) {
        if (nextRenderTime_ > now) {
            return false;
        }
        nextRenderTime_ += renderInterval_;
        if (nextRenderTime_ < now) { nextRenderTime_ = now + renderInterval_; }
    }
    if (nextCountTime_ <= now) {
        fps_ = float(frameCount_) / (1.f + float(now - nextCountTime_) / 1000000.f);
        nextCountTime_ = now + 1000 * 1000;
        frameCount_ = 0;
    }
    return true;
}

void Renderer::present() {
    SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
    ++frameCount_;
}

}
