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
#include "mem/action.hh"
#include "mem/savedata.hh"
#include "core/config.hh"
#include "util/conv.hh"

#include <SDL.h>

#include <stdexcept>

namespace hojy::scene {

Window *gWindow = nullptr;

static void medicMenu(Node *mainMenu);
static void medicTargetMenu(Node *mainMenu, int16_t charId);
static void depoisonMenu(Node *mainMenu);
static void depoisonTargetMenu(Node *mainMenu, int16_t charId);
static void statusMenu(Node *mainMenu);
static void showCharStatus(Node *mainMenu, std::int16_t charId);
static void systemMenu(Node *mainMenu);
static void selectSaveSlotMenu(Node *mainMenu, int x, int y, bool isSave);

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
    currTime_ = std::chrono::steady_clock::now();
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
    ++idx;
    if (playingMusic_ == idx) {
        return;
    }
    std::string filename;
    if (idx < 10) {
        filename = "GAME0" + std::to_string(idx) + ".XMI";
    } else {
        filename = "GAME" + std::to_string(idx) + ".XMI";
    }
    audio::gMixer.repeatPlay(0, new audio::ChannelMIDI(&audio::gMixer, core::config.musicFilePath(filename)));
    playingMusic_ = idx;
}

void Window::playAtkSound(int idx) {
    (void)this;
    std::string filename;
    if (idx < 10) {
        filename = "ATK0" + std::to_string(idx) + ".WAV";
    } else {
        filename = "ATK" + std::to_string(idx) + ".WAV";
    }
    audio::gMixer.play(1, new audio::ChannelWav(&audio::gMixer, core::config.soundFilePath(filename)));
}

void Window::playEffectSound(int idx) {
    (void)this;
    std::string filename;
    if (idx < 10) {
        filename = "E0" + std::to_string(idx) + ".WAV";
    } else {
        filename = "E" + std::to_string(idx) + ".WAV";
    }
    audio::gMixer.play(1, new audio::ChannelWav(&audio::gMixer, core::config.soundFilePath(filename)));
}

void Window::newGame() {
    map_ = subMap_;
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    dynamic_cast<SubMap*>(subMap_)->load(70);
    subMap_->setPosition(19, 20, false);
    dynamic_cast<SubMap*>(subMap_)->forceMainCharTexture(3445);
    map_->fadeIn([this] {
        map_->setPosition(19, 20);
        dynamic_cast<SubMap*>(subMap_)->forceMainCharTexture(3445);
    });
}

bool Window::loadGame(int slot) {
    if (!mem::gSaveData.load(slot)) { return false; }
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    auto &binfo = mem::gSaveData.baseInfo;
    if (binfo->subMap >= 0) {
        map_ = subMap_;
        dynamic_cast<SubMap *>(subMap_)->load(binfo->subMap);
        subMap_->setPosition(binfo->subX, binfo->subY, false);
        subMap_->setDirection(Map::Direction(binfo->direction));
        map_->fadeIn([this]() {
            map_->setPosition(binfo->subX, binfo->subY);
        });
    } else {
        globalMap_->setDirection(Map::Direction(binfo->direction));
        map_ = globalMap_;
        map_->fadeIn(nullptr);
    }
    return true;
}

bool Window::saveGame(int slot) {
    auto &binfo = mem::gSaveData.baseInfo;
    binfo->mainX = globalMap_->currX();
    binfo->mainY = globalMap_->currY();
    binfo->subMap = map_->subMapId();
    if (binfo->subMap >= 0) {
        binfo->subX = map_->currX();
        binfo->subY = map_->currY();
    }
    binfo->direction = std::int16_t(map_->direction());
    return mem::gSaveData.save(slot);
}

void Window::forceQuit() {
    (void)this;
    static SDL_QuitEvent evt = {SDL_QUIT, SDL_GetTicks()};
    SDL_PushEvent(reinterpret_cast<SDL_Event*>(&evt));
}

void Window::exitToGlobalMap(int direction) {
    map_->fadeOut([this, direction]() {
        map_ = globalMap_;
        globalMap_->setDirection(Map::Direction(direction));
        globalMap_->fadeIn(nullptr);
    });
}

void Window::enterSubMap(std::int16_t subMapId, int direction) {
    map_->fadeOut([this, subMapId, direction]() {
        map_ = subMap_;
        subMap_->setDirection(Map::Direction(direction));
        const auto &smi = mem::gSaveData.subMapInfo[subMapId];
        dynamic_cast<SubMap *>(subMap_)->load(subMapId);
        auto x = smi->enterX, y = smi->enterY;
        subMap_->setPosition(x, y, false);
        map_->fadeIn([this, x, y] {
            map_->setPosition(x, y);
        });
    });
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
        menu->setHandler([this](int index) {
            switch (index) {
            case 0:
                medicMenu(mainMenu_);
                break;
            case 1:
                depoisonMenu(mainMenu_);
                break;
            case 2:
                break;
            case 3:
                statusMenu(mainMenu_);
                break;
            case 4:
                break;
            case 5:
                systemMenu(mainMenu_);
                break;
            }
        }, [this]() {
            closePopup();
        });
        dynamic_cast<MenuTextList*>(mainMenu_)->popup({L"醫療", L"解毒", L"物品", L"狀態", L"離隊", L"系統"});
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
        msgBox = new MessageBox(popup_, 0, 0, width_, height_ * 4 / 5);
    } else {
        msgBox = new MessageBox(renderer_, 0, 0, width_, height_ * 4 / 5);
        popup_ = msgBox;
        freeOnClose_ = true;
    }
    msgBox->popup(text, type);
}


static void medicMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *msgBox = new MessageBox(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    msgBox->popup({L"誰要使用醫術"}, MessageBox::Normal, MessageBox::TopLeft);
    msgBox->forceUpdate();
    y = y + msgBox->height() + 10;
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->setTitle(L"醫療能力");
    std::vector<std::wstring> names;
    std::vector<int16_t> charIdList;
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (charInfo->medic > 0) {
            names.emplace_back(util::big5Conv.toUnicode(charInfo->name) + L' ' + std::to_wstring(charInfo->medic));
            charIdList.emplace_back(id);
        }
    }
    subMenu->popup(names);
    subMenu->setHandler([charIdList, mainMenu](int index) {
        medicTargetMenu(mainMenu, charIdList[index]);
    }, [msgBox, subMenu]() {
        auto *box = msgBox;
        delete subMenu;
        delete box;
    });
}

static void medicTargetMenu(Node *mainMenu, int16_t charId) {
    auto x = mainMenu->x() + mainMenu->width() + 30;
    auto y = mainMenu->y() + 20;
    auto *msgBox = new MessageBox(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    msgBox->popup({L"要醫治誰"}, MessageBox::Normal, MessageBox::TopLeft);
    msgBox->forceUpdate();
    y = y + msgBox->height() + 10;
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->setTitle(L"生命點數");
    std::vector<std::wstring> names;
    std::vector<int16_t> charIdList;
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        names.emplace_back(util::big5Conv.toUnicode(charInfo->name) + L' '
                               + std::to_wstring(charInfo->hp) + L'/' + std::to_wstring(charInfo->maxHp));
        charIdList.emplace_back(id);
    }
    subMenu->popup(names);
    subMenu->setHandler([charIdList, charId](int index) {
        int res = mem::actMedic(mem::gSaveData.charInfo[charId],
                                mem::gSaveData.charInfo[charIdList[index]], 2);
        gWindow->closePopup();
        gWindow->popupMessageBox({L"恢復生命 " + std::to_wstring(res)}, MessageBox::PressToCloseTop);
    }, [msgBox, subMenu]() {
        auto *box = msgBox;
        delete subMenu;
        delete box;
    });
}

static void depoisonMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *msgBox = new MessageBox(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    msgBox->popup({L"誰要幫人解毒"}, MessageBox::Normal, MessageBox::TopLeft);
    msgBox->forceUpdate();
    y = y + msgBox->height() + 10;
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->setTitle(L"解毒能力");
    std::vector<std::wstring> names;
    std::vector<int16_t> charIdList;
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (charInfo->depoison > 0) {
            names.emplace_back(util::big5Conv.toUnicode(charInfo->name) + L' ' + std::to_wstring(charInfo->depoison));
            charIdList.emplace_back(id);
        }
    }
    subMenu->popup(names);
    subMenu->setHandler([charIdList, mainMenu](int index) {
        depoisonTargetMenu(mainMenu, charIdList[index]);
    }, [msgBox, subMenu]() {
        auto *box = msgBox;
        delete subMenu;
        delete box;
    });
}

static void depoisonTargetMenu(Node *mainMenu, int16_t charId) {
    auto x = mainMenu->x() + mainMenu->width() + 30;
    auto y = mainMenu->y() + 20;
    auto *msgBox = new MessageBox(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    msgBox->popup({L"替誰解毒"}, MessageBox::Normal, MessageBox::TopLeft);
    msgBox->forceUpdate();
    y = y + msgBox->height() + 10;
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->setTitle(L"中毒程度");
    std::vector<std::wstring> names;
    std::vector<int16_t> charIdList;
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        names.emplace_back(util::big5Conv.toUnicode(charInfo->name) + L' ' + std::to_wstring(charInfo->poisoned));
        charIdList.emplace_back(id);
    }
    subMenu->popup(names);
    subMenu->setHandler([charIdList, charId](int index) {
        int res = mem::actDepoison(mem::gSaveData.charInfo[charId],
                                   mem::gSaveData.charInfo[charIdList[index]], 0);
        gWindow->closePopup();
        gWindow->popupMessageBox({L"幫助解毒 " + std::to_wstring(res)}, MessageBox::PressToCloseTop);
    }, [msgBox, subMenu]() {
        auto *box = msgBox;
        delete subMenu;
        delete box;
    });
}

static void statusMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *msgBox = new MessageBox(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    msgBox->popup({L"要查閱誰的狀態"}, MessageBox::Normal, MessageBox::TopLeft);
    msgBox->forceUpdate();
    y = y + msgBox->height() + 10;
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    std::vector<std::wstring> names;
    std::vector<int16_t> charIdList;
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        names.emplace_back(util::big5Conv.toUnicode(charInfo->name));
        charIdList.emplace_back(id);
    }
    subMenu->popup(names);
    subMenu->setHandler([charIdList, mainMenu](int index) {
        showCharStatus(mainMenu, charIdList[index]);
    }, [msgBox, subMenu]() {
        auto *box = msgBox;
        delete subMenu;
        delete box;
    });
}

static void showCharStatus(Node *mainMenu, std::int16_t charId) {

}

static void systemMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->popup({L"讀檔", L"存檔", L"離開"});
    subMenu->forceUpdate();
    x += subMenu->width() + 10;
    subMenu->setHandler([mainMenu, x, y](int index) {
        switch (index) {
        case 0:
            selectSaveSlotMenu(mainMenu, x, y, false);
            break;
        case 1:
            selectSaveSlotMenu(mainMenu, x, y, true);
            break;
        case 2: {
            auto *yesNo = new MenuYesNo(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
            yesNo->setHandler([]() { gWindow->forceQuit(); },
                              [yesNo]() { delete yesNo; });
            yesNo->popupWithYesNo();
            break;
        }
        }
    }, [subMenu]() {
        delete subMenu;
    });
}

static void selectSaveSlotMenu(Node *mainMenu, int x, int y, bool isSave) {
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->popup({L"一", L"二", L"三"});
    subMenu->setHandler([subMenu, isSave](int index) {
        if (isSave) {
            gWindow->saveGame(index + 1);
            gWindow->popupMessageBox({L"存檔完成"}, MessageBox::PressToCloseTop);
        } else {
            if (gWindow->loadGame(index + 1)) {
                gWindow->closePopup();
            } else {
                gWindow->popupMessageBox({L"讀檔失敗"}, MessageBox::PressToCloseTop);
            }
        }
    }, [subMenu]() {
        delete subMenu;
    });
}

}
