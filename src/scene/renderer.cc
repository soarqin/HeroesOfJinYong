#include "renderer.hh"

#include "texture.hh"

#include <SDL.h>

namespace hojy::scene {

Renderer::Renderer(void *win) {
    auto *renderer = SDL_CreateRenderer(static_cast<SDL_Window*>(win), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC);
    renderer_ = renderer;
}

Renderer::~Renderer() {
    SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
}

void Renderer::renderTexture(const Texture *tex, int x, int y) {
    auto w = tex->width(), h = tex->height();
    SDL_Rect src {0, 0, w, h};
    SDL_Rect dst {x - tex->originX(), y - tex->originY(), w, h};
    SDL_RenderCopy(static_cast<SDL_Renderer*>(renderer_), static_cast<SDL_Texture*>(tex->data()), &src, &dst);
}

void Renderer::present() {
    SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

}
