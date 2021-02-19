#include "window.hh"

#include <SDL.h>

namespace hojy::scene {

Window::Window(int w, int h) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    auto *win = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    win_ = win;
    renderer_ = new Renderer(win_);
    map_ = new Map(renderer_);
}

Window::~Window() {
    delete map_;
    delete renderer_;
    SDL_DestroyWindow(static_cast<SDL_Window*>(win_));
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
    renderer_->present();
}

void Window::render() {
    map_->doRender();
}

}
