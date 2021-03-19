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

#include "colorpalette.hh"
#include "globalmap.hh"
#include "submap.hh"
#include "warfield.hh"
#include "effect.hh"
#include "talkbox.hh"
#include "title.hh"
#include "dead.hh"
#include "endscreen.hh"
#include "menu.hh"
#include "charlistmenu.hh"
#include "itemview.hh"
#include "statusview.hh"

#include "audio/mixer.hh"
#include "audio/channelmidi.hh"
#include "audio/channelwav.hh"
#include "data/factors.hh"
#include "data/grpdata.hh"
#include "data/event.hh"
#include "mem/strings.hh"
#include "mem/savedata.hh"
#include "core/config.hh"
#include "util/conv.hh"

#include <SDL.h>
#include <fmt/format.h>
#include <stdexcept>

namespace hojy::scene {

Window *gWindow = nullptr;

static void medicMenu(Node *mainMenu);
static void medicTargetMenu(Node *mainMenu, int16_t charId);
static void depoisonMenu(Node *mainMenu);
static void depoisonTargetMenu(Node *mainMenu, int16_t charId);
static void showItems(Node *mainMenu);
static void statusMenu(Node *mainMenu);
static void showCharStatus(Node *parent, std::int16_t charId);
static void leaveTeamMenu(Node *mainMenu);
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

    gNormalPalette.load("MMAP");
    gEndPalette.load("ENDCOL");
    {
        std::array<std::uint32_t, 256> n {};
        n.fill(0xFFFFFFFFu);
        n[0] = 0;
        gMaskPalette.create(n);
    }

    globalTextureMgr_.setPalette(gNormalPalette);
    globalTextureMgr_.setRenderer(renderer_);
    headTextureMgr_.setPalette(gNormalPalette);
    headTextureMgr_.setRenderer(renderer_);
    data::GrpData::DataSet dset;
    renderer_->enableLinear(true);
    if (data::GrpData::loadData("HDGRP", dset)) {
        headTextureMgr_.loadFromRLE(dset);
    }
    renderer_->enableLinear(false);
    gEffect.load(renderer_, "EFT");

    globalMap_ = new GlobalMap(renderer_, 0, 0, w, h, core::config.scale());
    subMap_ = new SubMap(renderer_, 0, 0, w, h, core::config.scale());
    warfield_ = new Warfield(renderer_, 0, 0, w, h, core::config.scale());

    SDL_ShowWindow(win);
    audio::gMixer.init(3);
    audio::gMixer.pause(false);
    title();
}

Window::~Window() {
    closePopup();
    globalTextureMgr_.clear();
    headTextureMgr_.clear();
    gEffect.clear();
    delete talkBox_;
    delete globalMap_;
    delete subMap_;
    delete warfield_;
    delete renderer_;
    SDL_DestroyWindow(static_cast<SDL_Window*>(win_));
}

bool Window::processEvents() {
    static const std::map<SDL_Scancode, Node::Key> inputMap = {
        { SDL_SCANCODE_UP, Node::KeyUp },
        { SDL_SCANCODE_KP_8, Node::KeyUp },
        { SDL_SCANCODE_DOWN, Node::KeyDown },
        { SDL_SCANCODE_KP_2, Node::KeyDown },
        { SDL_SCANCODE_LEFT, Node::KeyLeft },
        { SDL_SCANCODE_KP_4, Node::KeyLeft },
        { SDL_SCANCODE_RIGHT, Node::KeyRight },
        { SDL_SCANCODE_KP_6, Node::KeyRight },
        { SDL_SCANCODE_RETURN, Node::KeyOK },
        { SDL_SCANCODE_KP_ENTER, Node::KeyOK },
        { SDL_SCANCODE_ESCAPE, Node::KeyCancel },
        { SDL_SCANCODE_DELETE, Node::KeyCancel },
        { SDL_SCANCODE_KP_PERIOD, Node::KeyCancel },
        { SDL_SCANCODE_SPACE, Node::KeySpace },
        { SDL_SCANCODE_BACKSPACE, Node::KeyBackspace },
    };
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_TEXTINPUT: {
            auto *node = popup_ ? popup_ : map_;
            node->doTextInput(util::Utf8Conv::toUnicode(e.text.text));
            break;
        }
        case SDL_KEYDOWN: {
            auto ite = inputMap.find(e.key.keysym.scancode);
            if (ite != inputMap.end()) {
                auto *node = popup_ ? popup_ : map_;
                node->doHandleKeyInput(ite->second);
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
    audio::gMixer.repeatPlay(0, new audio::ChannelMIDI(&audio::gMixer, core::config.musicFilePath(fmt::format("GAME{:02}.XMI", idx))));
    playingMusic_ = idx;
}

void Window::playAtkSound(int idx) {
    (void)this;
    if (idx >= 24) {
        playEffectSound(idx - 24);
        return;
    }
    audio::gMixer.play(1, new audio::ChannelWav(&audio::gMixer, core::config.soundFilePath(fmt::format("ATK{:02}.WAV", idx))));
}

void Window::playEffectSound(int idx) {
    (void)this;
    audio::gMixer.play(2, new audio::ChannelWav(&audio::gMixer, core::config.soundFilePath(fmt::format("E{:02}.WAV", idx))));
}

void Window::title() {
    playMusic(16);
    auto *title = new Title(renderer_, 0, 0, width_, height_);
    title->init();
    freeOnClose_ = true;
    popup_ = title;
}

void Window::endscreen() {
    subMap_->cleanupEvents();
    map_ = nullptr;
    auto *endScreen = new EndScreen(renderer_, 0, 0, width_, height_);
    endScreen->init();
    freeOnClose_ = true;
    popup_ = endScreen;
}

void Window::newGame() {
    mem::gStrings.saveDataLoaded();
    map_ = subMap_;
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    dynamic_cast<SubMap*>(subMap_)->load(data::gFactors.initSubMapId);
    subMap_->setPosition(data::gFactors.initSubMapX, data::gFactors.initSubMapY, false);
    dynamic_cast<SubMap*>(subMap_)->forceMainCharTexture(data::gFactors.initMainCharTex / 2);
    map_->fadeIn([this] {
        dynamic_cast<SubMap*>(subMap_)->setPosition(data::gFactors.initSubMapX, data::gFactors.initSubMapY);
        dynamic_cast<SubMap*>(subMap_)->forceMainCharTexture(data::gFactors.initMainCharTex / 2);
        map_->resetFrame();
    });
}

bool Window::loadGame(int slot) {
    if (!mem::gSaveData.load(slot)) { return false; }
    mem::gStrings.saveDataLoaded();
    globalMap_->setPosition(mem::gSaveData.baseInfo->mainX, mem::gSaveData.baseInfo->mainY);
    auto &binfo = mem::gSaveData.baseInfo;
    if (binfo->subMap > 0) {
        map_ = subMap_;
        dynamic_cast<SubMap *>(subMap_)->load(binfo->subMap - 1);
        subMap_->setPosition(binfo->subX, binfo->subY, false);
        subMap_->setDirection(Map::Direction(binfo->direction));
        map_->fadeIn([this]() {
            dynamic_cast<SubMap*>(subMap_)->setPosition(binfo->subX, binfo->subY);
            map_->resetFrame();
        });
    } else {
        globalMap_->setDirection(Map::Direction(binfo->direction));
        map_ = globalMap_;
        map_->resetFrame();
        map_->fadeIn([this]() {
            map_->resetFrame();
        });
    }
    return true;
}

bool Window::saveGame(int slot) {
    auto &binfo = mem::gSaveData.baseInfo;
    binfo->mainX = globalMap_->currX();
    binfo->mainY = globalMap_->currY();
    binfo->subMap = map_->subMapId() + 1;
    if (binfo->subMap > 0) {
        binfo->subX = dynamic_cast<SubMap*>(subMap_)->currX();
        binfo->subY = dynamic_cast<SubMap*>(subMap_)->currY();
    }
    binfo->direction = std::int16_t(dynamic_cast<MapWithEvent*>(map_)->direction());
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
        map_->resetFrame();
        dynamic_cast<MapWithEvent*>(map_)->setDirection(Map::Direction(direction));
        map_->fadeIn([this]() {
            map_->resetFrame();
        });
    });
}

void Window::enterSubMap(std::int16_t subMapId, int direction) {
    bool switching = map_->subMapId() >= 0;
    map_->fadeOut([this, subMapId, direction, switching]() {
        if (!switching) {
            map_ = subMap_;
            subMap_->setDirection(Map::Direction(direction));
        }
        const auto *smi = mem::gSaveData.subMapInfo[subMapId];
        dynamic_cast<SubMap *>(map_)->load(subMapId);
        std::int16_t x, y;
        if (switching && smi->subMapEnterX) {
            x = smi->subMapEnterX;
            y = smi->subMapEnterY;
        } else {
            x = smi->enterX;
            y = smi->enterY;
        }
        dynamic_cast<MapWithEvent*>(map_)->setPosition(x, y, false);
        auto *tips = new MessageBox(map_, 0, 0, width_, height_ * 4 / 5);
        tips->popup({GETSUBMAPNAME(subMapId)}, MessageBox::Normal);
        map_->fadeIn([this, tips, x, y] {
            delete tips;
            dynamic_cast<MapWithEvent*>(map_)->setPosition(x, y);
            map_->resetFrame();
        });
    });
}

void Window::enterWar(std::int16_t warId, bool getExpOnLose, bool deadOnLose) {
    auto *wf = dynamic_cast<Warfield*>(warfield_);
    wf->setGetExpOnLose(getExpOnLose);
    wf->setDeadOnLose(deadOnLose);
    wf->load(warId);
    std::set<std::int16_t> defaultChars;
    if (wf->getDefaultChars(defaultChars)) {
        auto *clm = new CharListMenu(renderer_, 0, 0, gWindow->width(), gWindow->height());
        clm->enableCheckBox(true, [defaultChars](std::int16_t charId)->bool {
            return defaultChars.find(charId) == defaultChars.end();
        });
        clm->initWithTeamMembers({GETTEXT(70)}, {CharListMenu::LEVEL}, [this, clm, wf](std::int16_t) {
            wf->putChars(clm->getSelectedCharIds());
            map_ = warfield_;
            closePopup();
        }, []()->bool { return false; });
        for (size_t i = 0; i < clm->charCount(); ++i) {
            if (defaultChars.find(clm->charId(i)) != defaultChars.end()) {
                clm->checkItem(i, true);
            }
        }
        clm->makeCenter(gWindow->width(), gWindow->height() * 4 / 5);
        popup_ = clm;
        freeOnClose_ = true;
    } else {
        wf->putChars({});
        map_ = warfield_;
    }
}

void Window::endWar(bool won, bool instantDie) {
    if (instantDie) { playerDie(); return; }
    map_ = subMap_;
    subMap_->continueEvents(won);
    auto *subMapInfo = mem::gSaveData.subMapInfo[subMap_->subMapId()];
    if (subMapInfo) {
        auto music = subMapInfo->enterMusic;
        if (music < 0) {
            music = subMapInfo->exitMusic;
        }
        if (music >= 0) {
            gWindow->playMusic(music);
        }
    }
}

void Window::playerDie() {
    subMap_->cleanupEvents();
    map_ = nullptr;
    auto *dead = new Dead(renderer_, 0, 0, width_, height_);
    dead->init();
    freeOnClose_ = true;
    popup_ = dead;
}

void Window::useQuestItem(std::int16_t itemId) {
    auto *mapev = dynamic_cast<MapWithEvent*>(map_);
    if (mapev) mapev->onUseItem(itemId);
}

void Window::forceEvent(std::int16_t eventId) {
    auto *mapev = dynamic_cast<MapWithEvent*>(map_);
    if (mapev) mapev->runEvent(eventId);
    else if (subMap_) subMap_->runEvent(eventId);
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
    auto *mapev = dynamic_cast<MapWithEvent*>(map_);
    if (mapev) mapev->continueEvents(result);
}

void Window::showMainMenu(bool inSubMap) {
    if (popup_) {
        return;
    }
    if (mainMenu_ == nullptr) {
        auto *menu = new MenuTextList(renderer_, 40, 40, width_ - 80, height_ - 80);
        mainMenu_ = menu;
        menu->setHandler([this]() {
            switch (dynamic_cast<Menu*>(mainMenu_)->currIndex()) {
            case 0:
                medicMenu(mainMenu_);
                break;
            case 1:
                depoisonMenu(mainMenu_);
                break;
            case 2:
                showItems(mainMenu_);
                break;
            case 3:
                statusMenu(mainMenu_);
                break;
            case 4:
                leaveTeamMenu(mainMenu_);
                break;
            case 5:
                systemMenu(mainMenu_);
                break;
            default:
                break;
            }
        }, [this]()->bool {
            closePopup();
            return false;
        });
    }
    popup_ = mainMenu_;
    freeOnClose_ = false;
    /*
    if (inSubMap) {
        dynamic_cast<MenuTextList*>(mainMenu_)->popup({GETTEXT(47), GETTEXT(48), GETTEXT(49), GETTEXT(50)});
    } else {
     */
        dynamic_cast<MenuTextList*>(mainMenu_)->popup({GETTEXT(47), GETTEXT(48), GETTEXT(49), GETTEXT(50), GETTEXT(51), GETTEXT(52)});
    /* } */
}

void Window::runTalk(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    if (popup_) {
        auto *mapev = dynamic_cast<MapWithEvent*>(map_);
        if (mapev) mapev->continueEvents(false);
        return;
    }
    if (!talkBox_) {
        talkBox_ = new TalkBox(renderer_, 50, 50, width_ - 100, height_ - 100);
    }
    dynamic_cast<TalkBox*>(talkBox_)->popup(text, headId, position);
    popup_ = talkBox_;
    freeOnClose_ = false;
}

bool Window::runShop(std::int16_t id) {
    auto *shopInfo = mem::gSaveData.shopInfo[id];
    if (!shopInfo) {
        return false;
    }
    auto *subMenu = new MenuTextList(popup_, 0, 0, gWindow->width(), gWindow->height());
    std::vector<std::wstring> items;
    std::vector<std::wstring> prices;
    std::vector<int> indices;
    for (int i = 0; i < data::ShopItemCount; ++i) {
        if (shopInfo->id[i] <= 0 || shopInfo->total[i] <= 0) { continue; }
        items.emplace_back(GETITEMNAME(shopInfo->id[i]));
        prices.emplace_back(std::to_wstring(shopInfo->price[i]));
        indices.emplace_back(i);
    }
    subMenu->popup(items, prices);
    subMenu->makeCenter(gWindow->width(), gWindow->height());
    subMenu->setHandler([subMenu, shopInfo, indices]() {
        int index = subMenu->currIndex();
        if (index < 0 || index >= indices.size()) { return; }
        index = indices[index];
        auto price = shopInfo->price[index];
        if (!mem::gBag.remove(data::ItemIDMoney, price)) {
            gWindow->closePopup();
            gWindow->runTalk(data::gEvent.talk(0xB9F), 0x6F, 0);
            return;
        }
        mem::gBag.add(shopInfo->id[index], 1);
        if (shopInfo->total[index] < 1000) {
            --shopInfo->total[index];
        }
        gWindow->closePopup();
        gWindow->runTalk(data::gEvent.talk(0xBA0), 0x6F, 0);
    }, [this]() {
        subMap_->continueEvents(false);
        return false;
    });
    return true;
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
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(53)}, {CharListMenu::MEDIC},
                              [mainMenu](std::int16_t charId) {
                                  medicTargetMenu(mainMenu, charId);
                              }, nullptr, [](CharListMenu::ValueType, std::int16_t value)->bool {
                                  return value > 0;
                              });
}

static void medicTargetMenu(Node *mainMenu, int16_t charId) {
    auto x = mainMenu->x() + mainMenu->width() + 30;
    auto y = mainMenu->y() + 20;
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(54)}, {CharListMenu::HP},
                              [charId](std::int16_t toCharId) {
                                  int res = mem::actMedic(mem::gSaveData.charInfo[charId],
                                                          mem::gSaveData.charInfo[toCharId], 2);
                                  gWindow->closePopup();
                                  gWindow->popupMessageBox({GETTEXT(55) + L' ' + std::to_wstring(res)}, MessageBox::PressToCloseTop);
                              }, nullptr);
}

static void depoisonMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(56)}, {CharListMenu::DEPOISON},
                              [mainMenu](std::int16_t charId) {
                                  depoisonTargetMenu(mainMenu, charId);
                              }, nullptr, [](CharListMenu::ValueType, std::int16_t value)->bool {
                                  return value > 0;
                              });
}

static void depoisonTargetMenu(Node *mainMenu, int16_t charId) {
    auto x = mainMenu->x() + mainMenu->width() + 30;
    auto y = mainMenu->y() + 20;
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(57)}, {CharListMenu::HP},
                              [charId](std::int16_t toCharId) {
                                  int res = mem::actDepoison(mem::gSaveData.charInfo[charId],
                                                             mem::gSaveData.charInfo[toCharId], 2);
                                  gWindow->closePopup();
                                  gWindow->popupMessageBox({GETTEXT(58) + L' ' + std::to_wstring(res)}, MessageBox::PressToCloseTop);
                              }, nullptr);
}

static void showItems(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *iv = new ItemView(mainMenu, x, y, gWindow->width() - x - 40, gWindow->height() - y - 40);
    iv->show(false, [](std::int16_t itemId) {
        gWindow->useQuestItem(itemId);
    });
}

static void statusMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(59)}, {CharListMenu::LEVEL},
                              [mainMenu](std::int16_t charId) {
                                  showCharStatus(mainMenu, charId);
                              }, nullptr);
}

static void showCharStatus(Node *parent, std::int16_t charId) {
    auto x = parent->x() + parent->width() + SubWindowBorder;
    auto y = parent->y();
    auto *sv = new StatusView(parent, x, y, gWindow->width() - x, gWindow->height() - y);
    sv->show(charId);
}

static void leaveTeamMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(60)}, {CharListMenu::LEVEL},
                              [](std::int16_t charId) {
                                  if (charId == 0) {
                                      gWindow->popupMessageBox({GETTEXT(61)}, MessageBox::PressToCloseThis);
                                      return;
                                  }
                                  if (mem::leaveTeam(charId)) {
                                      auto eventId = mem::getLeaveEventId(charId);
                                      gWindow->closePopup();
                                      if (eventId >= 0) {
                                          gWindow->forceEvent(eventId);
                                      }
                                  }
                              }, nullptr);
}

static void systemMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + 10;
    auto y = mainMenu->y();
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->popup({GETTEXT(62), GETTEXT(63), GETTEXT(64)});
    subMenu->forceUpdate();
    x += subMenu->width() + 10;
    subMenu->setHandler([mainMenu, subMenu, x, y]() {
        switch (subMenu->currIndex()) {
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
        default:
            break;
        }
    }, nullptr);
}

static void selectSaveSlotMenu(Node *mainMenu, int x, int y, bool isSave) {
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->popup({GETTEXT(65), GETTEXT(66), GETTEXT(67)});
    subMenu->setHandler([subMenu, isSave]() {
        auto index = subMenu->currIndex();
        if (isSave) {
            gWindow->saveGame(index + 1);
            gWindow->popupMessageBox({GETTEXT(68)}, MessageBox::PressToCloseTop);
        } else {
            if (gWindow->loadGame(index + 1)) {
                gWindow->closePopup();
            } else {
                gWindow->popupMessageBox({GETTEXT(69)}, MessageBox::PressToCloseTop);
            }
        }
    }, nullptr);
}

}
