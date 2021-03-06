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

#include "title.hh"

#include "window.hh"
#include "image.hh"
#include "data/grpdata.hh"
#include "data/colorpalette.hh"
#include "core/config.hh"

#include <SDL.h>

namespace hojy::scene {

Title::~Title() {
    for (auto *img: images_) {
        delete img;
    }
}

void Title::init() {
    std::vector<std::string> dset;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    auto *bigImg = new Image(renderer_, x_, y_, width_, height_);
    bigImg->loadRAW(core::config.dataFilePath("TITLE.BIG"), 320, 200);
    images_[9] = bigImg;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    titleTextureMgr_.setPalette(data::gNormalPalette);
    titleTextureMgr_.setRenderer(renderer_);
    if (data::GrpData::loadData("TITLE", dset)) {
        titleTextureMgr_.loadFromRLE(dset);
    }
    const auto *img0 = titleTextureMgr_[0];
    int x = x_ + width_ / 2 - img0->width() + img0->originX();
    int y = y_ + height_ * 3 / 4;
    std::pair<int, int> offsetY[9] = {
        {y, 120}, {y, 40}, {y + 40, 40}, {y + 80, 40},
        {y, 120}, {y, 40}, {y + 40, 40}, {y + 80, 40}, {y, 40}
    };
    for (int i = 0; i < 9; ++i) {
        auto *img = new Image(renderer_, x, offsetY[i].first, 200, offsetY[i].second);
        img->setTexture(titleTextureMgr_[i]);
        images_[i] = img;
    }
}

void Title::render() {
    renderer_->fill(0, 0, 0, 0);
    images_[9]->render();
    if (selSave_) {
        images_[4]->render();
        images_[5 + currSel_]->render();
    } else {
        images_[0]->render();
        images_[1 + currSel_]->render();
    }
}

void Title::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
        if (currSel_-- == 0) { currSel_ = 2; }
        break;
    case KeyDown:
        if (currSel_++ == 2) { currSel_ = 0; }
        break;
    case KeyOK:
        if (!selSave_) {
            switch (currSel_) {
            case 0:
                gWindow->closePopup();
                gWindow->newGame();
                break;
            case 1:
                currSel_ = 0;
                selSave_ = true;
                break;
            case 2:
                gWindow->closePopup();
                gWindow->forceQuit();
                break;
            default:
                break;
            }
        } else {
            int sel = currSel_;
            gWindow->closePopup();
            gWindow->loadGame(sel);
        }
        break;
    case KeyCancel:
        if (selSave_) {
            currSel_ = 0;
            selSave_ = false;
        }
        break;
    default:
        break;
    }
}

}
