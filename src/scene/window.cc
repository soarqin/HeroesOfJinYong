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

#include "window.hh"

#include "globalmap.hh"
#include "submap.hh"

#include <SDL.h>

namespace hojy::scene {

Window::Window(int w, int h) {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    auto *win = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    win_ = win;
    renderer_ = new Renderer(win_);
    map_ = new SubMap(renderer_, w, h, 1.5f, 70);
    topNode_ = map_;
    ttf_ = new TTF(renderer_->get());
    ttf_->init(16);
    ttf_->add("mono.ttf");
    ttf_->add("cjk.ttf");
}

Window::~Window() {
    delete ttf_;
    delete map_;
    delete renderer_;
    SDL_DestroyWindow(static_cast<SDL_Window*>(win_));
}

bool Window::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_KEYDOWN:
            switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_UP:
                topNode_->handleKeyInput(Node::KeyUp);
                break;
            case SDL_SCANCODE_RIGHT:
                topNode_->handleKeyInput(Node::KeyRight);
                break;
            case SDL_SCANCODE_LEFT:
                topNode_->handleKeyInput(Node::KeyLeft);
                break;
            case SDL_SCANCODE_DOWN:
                topNode_->handleKeyInput(Node::KeyDown);
                break;
            case SDL_SCANCODE_RETURN: case SDL_SCANCODE_SPACE:
                topNode_->handleKeyInput(Node::KeyOK);
                break;
            case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_DELETE: case SDL_SCANCODE_BACKSPACE:
                topNode_->handleKeyInput(Node::KeyCancel);
                break;
            default:
                break;
            }
            break;
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
    ttf_->render(L"ABC测试", 5, 5, 100);
}

}
