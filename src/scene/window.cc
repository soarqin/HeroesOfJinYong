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
#include "title.hh"
#include "menu.hh"

#include "audio/mixer.hh"
#include "audio/channelmidi.hh"
#include "audio/channelwav.hh"
#include "data/colorpalette.hh"
#include "data/grpdata.hh"
#include "mem/savedata.hh"
#include "core/config.hh"
#include "util/conv.hh"

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
    auto *win = SDL_CreateWindow("Heroes of Jin Yong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    win_ = win;
    gWindow = this;

    renderer_ = new Renderer(win_, w, h);
    renderer_->enableLinear(false);

    SDL_ShowWindow(win);

    audio::gMixer.init(2);
    playMusic(16);
    audio::gMixer.pause(false);

    globalTextureMgr_.setPalette(data::gNormalPalette);
    globalTextureMgr_.setRenderer(renderer_);
    headTextureMgr_.setPalette(data::gNormalPalette);
    headTextureMgr_.setRenderer(renderer_);
    data::GrpData::DataSet dset;
    renderer_->enableLinear(true);
    if (data::GrpData::loadData("HDGRP", dset)) {
        headTextureMgr_.loadFromRLE(dset);
    }
    renderer_->enableLinear(false);

    globalMap_ = new GlobalMap(renderer_, 0, 0, w, h, core::config.scale());
    subMap_ = new SubMap(renderer_, 0, 0, w, h, core::config.scale());

    auto *title = new Title(renderer_, 0, 0, w, h);
    title->init();
    popup_ = title;
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
        case SDL_TEXTINPUT: {
            auto *node = popup_ ? popup_ : map_;
            node->doTextInput(util::Utf8Conv::toUnicode(e.text.text));
            break;
        }
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
            case SDL_SCANCODE_RETURN:
                node->doHandleKeyInput(Node::KeyOK);
                break;
            case SDL_SCANCODE_ESCAPE: case SDL_SCANCODE_DELETE:
                node->doHandleKeyInput(Node::KeyCancel);
                break;
            case SDL_SCANCODE_SPACE:
                node->doHandleKeyInput(Node::KeySpace);
                break;
            case SDL_SCANCODE_BACKSPACE:
                node->doHandleKeyInput(Node::KeyBackspace);
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

void Window::render() {
    if (map_) {
        map_->doRender();
    }
    if (popup_) {
        popup_->doRender();
    }
}

void Window::flush() {
    renderer_->present();
}

void Window::playMusic(int idx) {
    (void)this;
    std::string filename;
    ++idx;
    if (idx < 10) {
        filename = "data/GAME0" + std::to_string(idx) + ".XMI";
    } else {
        filename = "data/GAME" + std::to_string(idx) + ".XMI";
    }
    audio::gMixer.repeatPlay(0, new audio::ChannelMIDI(&audio::gMixer, filename));
}

void Window::playAtkSound(int idx) {
    (void)this;
    std::string filename;
    if (idx < 10) {
        filename = "data/ATK0" + std::to_string(idx) + ".WAV";
    } else {
        filename = "data/ATK" + std::to_string(idx) + ".WAV";
    }
    audio::gMixer.play(1, new audio::ChannelWav(&audio::gMixer, filename));
}

void Window::playEffectSound(int idx) {
    (void)this;
    std::string filename;
    if (idx < 10) {
        filename = "data/E0" + std::to_string(idx) + ".WAV";
    } else {
        filename = "data/E" + std::to_string(idx) + ".WAV";
    }
    audio::gMixer.play(1, new audio::ChannelWav(&audio::gMixer, filename));
}

void Window::newGame() {
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    dynamic_cast<SubMap*>(subMap_)->load(70, 19, 20);
    dynamic_cast<SubMap*>(subMap_)->forceMainCharTexture(3445);
    map_ = subMap_;
}

void Window::loadGame(int slot) {
    mem::gSaveData.load(slot);
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    auto &binfo = mem::gSaveData.baseInfo;
    if (binfo->subMap >= 0) {
        dynamic_cast<SubMap *>(subMap_)->load(binfo->subMap, binfo->subX, binfo->subY);
        map_ = subMap_;
    } else {
        map_ = globalMap_;
    }
}

void Window::forceQuit() {
    static SDL_QuitEvent evt = {SDL_QUIT, SDL_GetTicks()};
    SDL_PushEvent(reinterpret_cast<SDL_Event*>(&evt));
}

void Window::exitToGlobalMap(int direction) {
    globalMap_->setDirection(Map::Direction(direction));
    map_ = globalMap_;
}

void Window::enterSubMap(std::int16_t subMapId, int direction) {
    subMap_->setDirection(Map::Direction(direction));
    const auto &smi = mem::gSaveData.subMapInfo[subMapId];
    dynamic_cast<SubMap*>(subMap_)->load(subMapId, smi->enterX, smi->enterY);
    map_ = subMap_;
}

void Window::closePopup() {
    if (!popup_) { return; }
    if (freeOnClose_) {
        delete popup_;
    } else {
        popup_->close();
    }
    popup_ = nullptr;
}

void Window::endPopup(bool close, bool result) {
    if (close) {
        closePopup();
    }
    if (map_) {
        map_->continueEvents(result);
    }
}

void Window::showMainMenu(bool inSubMap) {
    if (popup_) {
        return;
    }
    if (mainMenu_ == nullptr) {
        auto *menu = new MenuTextList(renderer_, 40, 40, width_ - 80, height_ - 80);
        mainMenu_ = menu;
        menu->setHandler([](int index) {
        }, [this]() {
            closePopup();
        });
    }
    popup_ = mainMenu_;
    freeOnClose_ = false;
    if (inSubMap) {
        dynamic_cast<MenuTextList*>(mainMenu_)->popup({L"醫療", L"解毒", L"物品", L"狀態"});
    } else {
        dynamic_cast<MenuTextList*>(mainMenu_)->popup({L"醫療", L"解毒", L"物品", L"狀態", L"離隊", L"系統"});
    }
}

void Window::runTalk(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    if (popup_) {
        map_->continueEvents(false);
        return;
    }
    if (!talkBox_) {
        talkBox_ = new TalkBox(renderer_, 50, 50, width_ - 100, height_ - 100);
    }
    dynamic_cast<TalkBox*>(talkBox_)->popup(text, headId, position);
    popup_ = talkBox_;
    freeOnClose_ = false;
}

void Window::popupMessageBox(const std::vector<std::wstring> &text, MessageBox::Type type) {
    MessageBox *msgBox;
    if (popup_) {
        msgBox = new MessageBox(popup_, 50, 50, width_ - 100, height_ - 100);
    } else {
        msgBox = new MessageBox(renderer_, 50, 50, width_ - 100, height_ - 100);
        popup_ = msgBox;
        freeOnClose_ = true;
    }
    msgBox->popup(text, type);
}

}
