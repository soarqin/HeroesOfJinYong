#include "window.hh"

#include "texture.hh"

#include <SDL.h>

namespace hojy::scene {

struct WindowCtx {
    SDL_Window *win;
    SDL_Renderer *renderer;
};

Window::Window(int w, int h): ctx_(new WindowCtx) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    ctx_->win = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    ctx_->renderer = SDL_CreateRenderer(ctx_->win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC);
}

Window::~Window() {
    SDL_DestroyRenderer(ctx_->renderer);
    SDL_DestroyWindow(ctx_->win);
    delete ctx_;
}

void *Window::renderer() {
    return ctx_->renderer;
}

bool Window::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            return false;
        }
    }
    return true;
}

void Window::flush() {
    SDL_RenderPresent(ctx_->renderer);
}

void Window::render(const Texture *tex, int x, int y) {
    auto w = tex->width(), h = tex->height();
    SDL_Rect src {0, 0, w, h};
    SDL_Rect dst {x - tex->originX(), y - tex->originY(), w, h};
    SDL_RenderCopy(ctx_->renderer, static_cast<SDL_Texture*>(tex->data()), &src, &dst);
}

}
