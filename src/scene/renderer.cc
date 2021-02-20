#include "renderer.hh"

#include "texture.hh"

#include <SDL.h>

namespace hojy::scene {

Renderer::Renderer(void *win): renderer_(SDL_CreateRenderer(static_cast<SDL_Window*>(win), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC)) {
}

Renderer::~Renderer() {
    SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
}

void Renderer::setTargetTexture(Texture *tex) {
    SDL_SetRenderTarget(static_cast<SDL_Renderer*>(renderer_), tex ? static_cast<SDL_Texture*>(tex->data()) : nullptr);
}

void Renderer::enableBlendMode(bool r) {
    SDL_SetRenderDrawBlendMode(static_cast<SDL_Renderer*>(renderer_), r ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
}

void Renderer::setClipRect(int l, int r, int w, int h) {
    SDL_Rect rc {l, r, w, h};
    SDL_RenderSetClipRect(static_cast<SDL_Renderer*>(renderer_), &rc);
}

void Renderer::unsetClipRect() {
    SDL_RenderSetClipRect(static_cast<SDL_Renderer*>(renderer_), nullptr);
}

void Renderer::fill(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *ren = static_cast<SDL_Renderer*>(renderer_);
    SDL_SetRenderDrawColor(ren, r, g, b, a);
    SDL_RenderClear(ren);
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

void Renderer::present() {
    SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

}
