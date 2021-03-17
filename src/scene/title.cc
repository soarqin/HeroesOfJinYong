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

#include "colorpalette.hh"
#include "window.hh"
#include "menu.hh"
#include "mem/savedata.hh"
#include "mem/action.hh"
#include "mem/strings.hh"
#include "data/factors.hh"
#include "data/grpdata.hh"
#include "core/config.hh"
#include "util/random.hh"
#include "util/file.hh"
#include "util/conv.hh"
#include <cstring>

namespace hojy::scene {

Title::~Title() {
    delete big_;
}

void Title::init() {
    titleTextureMgr_.setPalette(gNormalPalette);
    titleTextureMgr_.setRenderer(renderer_);

    renderer_->enableLinear(true);
    big_ = gWindow->globalTextureMgr().loadFromRAW(util::File::getFileContent(core::config.dataFilePath("TITLE.BIG")), 320, 200);
    renderer_->enableLinear(false);

    std::vector<std::string> dset;
    if (data::GrpData::loadData("TITLE", dset)) {
        titleTextureMgr_.loadFromRLE(dset);
    }
    update();
}

void Title::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
        if (currSel_-- == 0) { currSel_ = 2; }
        update();
        break;
    case KeyDown:
        if (currSel_++ == 2) { currSel_ = 0; }
        update();
        break;
    case KeyOK: case KeySpace:
        switch (mode_) {
        case 0:
            switch (currSel_) {
            case 0:
                if (core::config.noNameInput()) {
                    mainCharName_ = core::config.defaultName();
                    mode_ = 3;
                    mem::gSaveData.newGame();
                    doRandomBaseInfo();
                } else {
                    mainCharName_.clear();
                    mode_ = 2;
                }
                update();
                break;
            case 1:
                currSel_ = 0;
                mode_ = 1;
                update();
                break;
            case 2:
                gWindow->closePopup();
                gWindow->forceQuit();
                break;
            }
            break;
        case 1: {
            int sel = int(currSel_) + 1;
            if (gWindow->loadGame(sel)) {
                gWindow->closePopup();
            } else {
                auto *msgBox = new MessageBox(this, 0, height_ / 2, width_, height_ / 2);
                msgBox->popup({GETTEXT(69)}, MessageBox::PressToCloseThis);
            }
            break;
        }
        case 2:
            if (key == KeyOK) {
                mode_ = 3;
                mem::gSaveData.newGame();
                doRandomBaseInfo();
                update();
            }
            break;
        }
        break;
    case KeyCancel:
        switch (mode_) {
        case 1:
        case 2:
            currSel_ = 0;
            mode_ = 0;
            update();
            break;
        }
        break;
    case KeyBackspace:
        switch (mode_) {
        case 2:
            if (!mainCharName_.empty()) {
                mainCharName_.pop_back();
                update();
            }
            break;
        }
        break;
    default:
        break;
    }
}

void Title::handleTextInput(const std::wstring &str) {
    if (mode_ != 2) { return; }
    bool dirty = false;
    for (auto &ch: str) {
        if (mainCharName_.length() < 8 && ch != L' ') {
            mainCharName_ += ch;
            dirty = true;
        }
    }
    if (dirty) {
        update();
    }
}

void Title::makeCache() {
    cacheBegin();
    renderer_->clear(0, 0, 0, 255);

    int w = width_, h = width_ * big_->height() / big_->width();
    if (h > height_) {
        h = height_;
        w = height_ * big_->width() / big_->height();
    }
    int x = (width_ - w) / 2;
    int y = (height_ - h) / 2;
    renderer_->renderTexture(big_, x, y, w, h, 0, 0, big_->width(), big_->height(), false);
    switch (mode_) {
    case 0:
    case 1: {
        float scale = y == 0 ? float(height_) / 240.f : float(width_) / 320.f;
        const auto *img0 = titleTextureMgr_[0];
        float x0 = (float(width_) - scale * float(img0->width() + img0->originX())) / 2.f;
        float y0 = float(height_) - 20.f * scale * 3.5f;
        static const std::pair<float, float> offsetY[9] = {
            {y0, 20.f * scale * 3.f}, {y0, 20.f * scale}, {y0 + 20.f * scale, 20.f * scale}, {y0 + 20.f * scale * 2.f, 20.f * scale},
            {y0, 20.f * scale * 3.f}, {y0, 20.f * scale}, {y0 + 20.f * scale, 20.f * scale}, {y0 + 20.f * scale * 2.f, 20.f * scale}, {y0, 40}
        };
        if (mode_ == 0) {
            renderer_->renderTexture(titleTextureMgr_[0], x0, offsetY[0].first, scale);
            renderer_->renderTexture(titleTextureMgr_[1 + currSel_], x0, offsetY[1 + currSel_].first, scale);
        } else {
            renderer_->renderTexture(titleTextureMgr_[4], x0, offsetY[4].first, scale);
            renderer_->renderTexture(titleTextureMgr_[5 + currSel_], x0, offsetY[5 + currSel_].first, scale);
        }
        cacheEnd();
        break;
    }
    case 2: {
        auto *ttf = renderer_->ttf();
        y = height_ - (ttf->fontSize() + TextLineSpacing) * 5;
        ttf->setColor(236, 236, 236);
        ttf->setAltColor(2, 224, 180, 32);
        ttf->render(GETTEXT(41) + L'\2' + mainCharName_, width_ / 4, y, false);
        cacheEnd();
        break;
    }
    case 3: {
        auto ttf = renderer_->ttf();
        int lineheight = ttf->fontSize() + TextLineSpacing;
        y = height_ - lineheight * 6;
        int hh = lineheight - TextLineSpacing / 4;
        int colwidth = ttf->fontSize() * 21 / 4;
        x = (width_ - colwidth * 4 + 20) / 2;
        int ox = x, oy = y;
        auto askText = L'\2' + mainCharName_ + L"  \1" + GETTEXT(100);
        auto *data = mem::gSaveData.charInfo[0];
        ttf->setColor(236, 236, 236);
        ttf->setAltColor(2, 224, 180, 32);
        ttf->render(askText, x, y, false);
        y += lineheight * 2;
        drawProperty(GETTEXT(26), data->maxMp, 50, x, y, hh, data->mpType);
        drawProperty(GETTEXT(101), data->attack, 30, x + colwidth, y, hh);
        drawProperty(GETTEXT(9), data->speed, 30, x + colwidth * 2, y, hh);
        drawProperty(GETTEXT(102), data->defence, 30, x + colwidth * 3, y, hh);
        y += lineheight;
        drawProperty(GETTEXT(25), data->maxHp, 50, x, y, hh);
        drawProperty(GETTEXT(103), data->medic, 30, x + colwidth, y, hh);
        drawProperty(GETTEXT(104), data->poison, 30, x + colwidth * 2, y, hh);
        drawProperty(GETTEXT(105), data->depoison, 30, x + colwidth * 3, y, hh);
        y += lineheight;
        drawProperty(GETTEXT(106), data->fist, 30, x, y, hh);
        drawProperty(GETTEXT(107), data->sword, 30, x + colwidth, y, hh);
        drawProperty(GETTEXT(108), data->blade, 30, x + colwidth * 2, y, hh);
        drawProperty(GETTEXT(109), data->special, 30, x + colwidth * 3, y, hh);
        if (core::config.showPotential()) {
            drawProperty(GETTEXT(29), data->potential, 100, x + colwidth * 4, y, hh);
        }
        cacheEnd();
        if (mode_ == 3 && menu_ == nullptr) {
            int mx = ox + ttf->stringWidth(askText) + SubWindowBorder + 10;
            int my = oy - SubWindowBorder;
            auto *menu = new MenuYesNo(this, mx, my, gWindow->width() - mx, gWindow->height() - y);
            menu->enableHorizonal(true);
            menu->popupWithYesNo();
            menu->setHandler([this] {
                auto big5Name = util::big5Conv.fromUnicode(mainCharName_);
                while (big5Name.length() > 8) {
                    mainCharName_.pop_back();
                    big5Name = util::big5Conv.fromUnicode(mainCharName_);
                }
                auto *charInfo = mem::gSaveData.charInfo[0];
                memset(charInfo->name, 0, 10);
                memcpy(charInfo->name, big5Name.data(), big5Name.length());
                auto *subMap = mem::gSaveData.subMapInfo[data::gFactors.initSubMapId];
                auto tailName = util::big5Conv.fromUnicode(GETTEXT(110));
                memset(subMap->name, 0, 10);
                memcpy(subMap->name, big5Name.data(), big5Name.length());
                memcpy(subMap->name + big5Name.length(), tailName.data(), tailName.length());
                fadeOut([] {
                    gWindow->closePopup();
                    gWindow->newGame();
                });
            }, [this] {
                doRandomBaseInfo();
                update();
            });
            menu_ = menu;
        }
    }
    }
}

void Title::doRandomBaseInfo() {
    (void)this;
    auto *data = mem::gSaveData.charInfo[0];
    data->maxHp = util::gRandom(25, 50);
    data->hp = data->maxHp;
    data->maxMp = util::gRandom(25, 50);
    data->mp = data->maxMp;
    data->mpType = util::gRandom(0, 1);
    data->hpAddOnLevelUp = util::gRandom(1, 10);
    data->attack = util::gRandom(25, 30);
    data->speed = util::gRandom(25, 30);
    data->defence = util::gRandom(25, 30);
    data->medic = util::gRandom(25, 30);
    data->poison = util::gRandom(25, 30);
    data->depoison = util::gRandom(25, 30);
    data->fist = util::gRandom(25, 30);
    data->sword = util::gRandom(25, 30);
    data->blade = util::gRandom(25, 30);
    data->special = util::gRandom(25, 30);
    data->throwing = util::gRandom(25, 30);
    data->potential = util::gRandom(1, 100);
}

void Title::drawProperty(const std::wstring &name, std::int16_t value, std::int16_t maxValue, int x, int y, int h, int mpType) {
    auto ttf = renderer_->ttf();
    bool shadow = false;
    auto dispString = name + L": " + std::to_wstring(value);
    if (value >= maxValue) {
        renderer_->fillRect(x, y, ttf->stringWidth(dispString) + 2, h, 216, 20, 24, 255);
        shadow = true;
    }
    if (mpType >= 0) {
        std::uint8_t r, g, b;
        std::tie(r, g, b) = mem::calcColorForMpType(mpType);
        ttf->setColor(r, g, b);
    } else {
        if (value >= maxValue) {
            ttf->setColor(252, 236, 132);
        } else {
            ttf->setColor(216, 20, 24);
        }
    }
    ttf->render(dispString, x, y, shadow);
}

}
