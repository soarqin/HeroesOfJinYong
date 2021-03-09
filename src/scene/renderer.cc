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

#include "texture.hh"

#include <SDL2_gfxPrimitives.h>

namespace hojy::scene {

Renderer::Renderer(void *win, int w, int h):
    renderer_(SDL_CreateRenderer(static_cast<SDL_Window*>(win), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC)),
    ttf_(new TTF(renderer_)) {
    SDL_SetRenderDrawBlendMode(static_cast<SDL_Renderer*>(renderer_), SDL_BLENDMODE_BLEND);
    if (w * 3 > h * 4) {
        ttf_->init(h / 48 * 2);
    } else {
        ttf_->init(w * 3 / 4 / 48 * 2);
    }
    ttf_->add("mono.ttf");
    ttf_->add("cjk.ttf");
}

Renderer::~Renderer() {
    delete ttf_;
    SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
}

void Renderer::enableLinear(bool linear) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, linear ? "linear" : "nearest");
}

void Renderer::setTargetTexture(Texture *tex) {
    SDL_SetRenderTarget(static_cast<SDL_Renderer*>(renderer_), tex ? static_cast<SDL_Texture*>(tex->data()) : nullptr);
}

void Renderer::setClipRect(int x, int y, int w, int h) {
    SDL_Rect rc {x, y, w, h};
    SDL_RenderSetClipRect(static_cast<SDL_Renderer*>(renderer_), &rc);
}

void Renderer::unsetClipRect() {
    SDL_RenderSetClipRect(static_cast<SDL_Renderer*>(renderer_), nullptr);
}

void Renderer::clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_RenderClear(ren);
}

void Renderer::fillRect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_Rect rc {x, y, w, h};
    SDL_RenderFillRect(ren, &rc);
}

void Renderer::drawRect(int x, int y, int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_Rect rc {x, y, w, h};
    SDL_RenderDrawRect(ren, &rc);
}

void Renderer::fillRoundedRect(int x, int y, int w, int h, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    roundedBoxRGBA(static_cast<SDL_Renderer*>(renderer_), x, y, x + w - 1, y + h - 1, rad, r, g, b, a);
}

void Renderer::drawRoundedRect(int x, int y, int w, int h, int rad, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    roundedRectangleRGBA(static_cast<SDL_Renderer*>(renderer_), x, y, x + w - 1, y + h - 1, rad, r, g, b, a);
}

void Renderer::renderTexture(const Texture *tex, int x, int y, bool ignoreOrigin) {
    auto w = tex->width(), h = tex->height();
    SDL_Rect src {0, 0, w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {x, y, w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{x - tex->originX(), y - tex->originY(), w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::renderTexture(const Texture *tex, float x, float y, float scale, bool ignoreOrigin) {
    auto w = tex->width(), h = tex->height();
    SDL_Rect src {0, 0, w, h};
    if (ignoreOrigin) {
        SDL_FRect dst {x, y, float(w) * scale, float(h) * scale};
        SDL_RenderCopyF(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_FRect dst{x - float(tex->originX()) * scale, y - float(tex->originY()) * scale, float(w) * scale, float(h) * scale};
        SDL_RenderCopyF(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::renderTexture(const Texture *tex, int destx, int desty, int x, int y, int w, int h, bool ignoreOrigin) {
    SDL_Rect src {x, y, w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {destx, desty, w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{destx - tex->originX(), desty - tex->originY(), w, h};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::renderTexture(const Texture *tex, int destx, int desty, int destw, int desth, int x, int y, int w, int h, bool ignoreOrigin) {
    SDL_Rect src {x, y, w, h};
    if (ignoreOrigin) {
        SDL_Rect dst {destx, desty, destw, desth};
        SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
    } else {
        SDL_Rect dst{destx - tex->originX() * destw / w, desty - tex->originY() * desth / h, destw, desth};
        SDL_RenderCopy(static_cast<SDL_Renderer *>(renderer_), static_cast<SDL_Texture *>(tex->data()), &src, &dst);
    }
}

void Renderer::present() {
    SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

}
