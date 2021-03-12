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

#pragma once

#include "renderer.hh"
#include "texture.hh"
#include "mapwithevent.hh"
#include "messagebox.hh"

#include <chrono>
#include <string>
#include <cstdint>

namespace hojy::scene {

enum {
    SubWindowBorder = 10,
    RoundedRectRad = 10,
};

class Window final {
public:
    Window(int w, int h);
    ~Window();

    [[nodiscard]] Renderer *renderer() {
        return renderer_;
    }

    [[nodiscard]] inline int width() const { return width_; }
    [[nodiscard]] inline int height() const { return height_; }

    [[nodiscard]] std::chrono::steady_clock::time_point currTime() { return currTime_; }

    [[nodiscard]] inline const Texture *headTexture(std::int16_t id) const { return headTextureMgr_[id]; }
    [[nodiscard]] TextureMgr &globalTextureMgr() { return globalTextureMgr_; }
    [[nodiscard]] const TextureMgr &mapTextureMgr() const { return map_->textureMgr(); }

    [[nodiscard]] MapWithEvent *globalMap() const { return globalMap_; }

    bool processEvents();
    void render();
    void flush();

    void playMusic(int idx);
    void playAtkSound(int idx);
    void playEffectSound(int idx);

    void newGame();
    bool loadGame(int slot);
    bool saveGame(int slot);
    void forceQuit();
    void exitToGlobalMap(int direction);
    void enterSubMap(std::int16_t subMapId, int direction);
    void enterWar(std::int16_t warId, bool getExpOnLose);
    void useQuestItem(std::int16_t itemId);
    void forceEvent(std::int16_t eventId);

    void closePopup();
    void endPopup(bool close = false, bool result = true);

    void showMainMenu(bool inSubMap);
    void runTalk(const std::wstring &text, std::int16_t headId, std::int16_t position);
    void popupMessageBox(const std::vector<std::wstring> &text, MessageBox::Type type = MessageBox::Normal);

private:
    int width_, height_;
    void *win_ = nullptr;
    Renderer *renderer_ = nullptr;
    Map *map_ = nullptr;
    Node *popup_ = nullptr;
    Node *mainMenu_ = nullptr;
    bool freeOnClose_ = false;

    MapWithEvent *globalMap_ = nullptr;
    MapWithEvent *subMap_ = nullptr;
    Map *warfield_ = nullptr;
    Node *talkBox_ = nullptr;
    TextureMgr globalTextureMgr_, headTextureMgr_;

    std::chrono::steady_clock::time_point currTime_;
    int playingMusic_ = -1;
};

extern Window *gWindow;

}
