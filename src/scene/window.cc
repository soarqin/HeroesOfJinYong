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
#include "talkbox.hh"

#include "data/colorpalette.hh"
#include "data/grpdata.hh"
#include "mem/savedata.hh"

#include <SDL.h>

#include <stdexcept>

namespace hojy::scene {

Window *gWindow = nullptr;

Window::Window(int w, int h): width_(w), height_(h) {
    if (gWindow) {
        throw std::runtime_error("Duplicate window creation");
    }
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    auto *win = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    win_ = win;
    renderer_ = new Renderer(win_, w, h);
    globalMap_ = new GlobalMap(renderer_, 0, 0, w, h, 2.f);
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    subMap_ = new SubMap(renderer_, 0, 0, w, h, 2.f);
    subMap_->load(70);
    map_ = subMap_;
    gHeadTextureMgr.setPalette(data::gNormalPalette);
    gHeadTextureMgr.setRenderer(renderer_);
    data::GrpData::DataSet dset;
    if (data::GrpData::loadData("HDGRP", dset)) {
        gHeadTextureMgr.loadFromRLE(dset);
    }
    gWindow = this;
}

Window::~Window() {
    delete talkBox_;
    delete globalMap_;
    delete subMap_;
    delete renderer_;
    SDL_DestroyWindow(static_cast<SDL_Window*>(win_));
}

bool Window::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_KEYDOWN: {
            auto *node = popup_ ? popup_ : map_;
            switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_UP:
                node->doHandleKeyInput(Node::KeyUp);
                break;
            case SDL_SCANCODE_RIGHT:
                node->doHandleKeyInput(Node::KeyRight);
                break;
            case SDL_SCANCODE_LEFT:
                node->doHandleKeyInput(Node::KeyLeft);
                break;
            case SDL_SCANCODE_DOWN:
                node->doHandleKeyInput(Node::KeyDown);
                break;
            case SDL_SCANCODE_RETURN: case SDL_SCANCODE_SPACE:
                node->doHandleKeyInput(Node::KeyOK);
                break;
            case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_DELETE: case SDL_SCANCODE_BACKSPACE:
                node->doHandleKeyInput(Node::KeyCancel);
                break;
            default:
                break;
            }
            break;
        }
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
    if (map_) {
        map_->doRender();
    }
    if (popup_) {
        popup_->doRender();
    }
}

void Window::exitToGlobalMap(int direction) {
    globalMap_->setDirection(Map::Direction(direction));
    map_ = globalMap_;
}

void Window::enterSubMap(std::int16_t subMapId, int direction) {
    subMap_->load(subMapId);
    subMap_->setDirection(Map::Direction(direction));
    subMap_->setDefaultPosition();
    map_ = subMap_;
}

void Window::closePopup() {
    if (!popup_) { return; }
    popup_ = nullptr;
    map_->continueEvents();
}

void Window::runTalk(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    if (!talkBox_) {
        talkBox_ = new TalkBox(renderer_, 50, 50, width_ - 100, height_ - 100);
    }
    dynamic_cast<TalkBox*>(talkBox_)->popup(text, headId, position);
    popup_ = talkBox_;
}

}
