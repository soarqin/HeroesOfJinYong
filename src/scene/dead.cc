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

#include "dead.hh"

#include "window.hh"
#include "menu.hh"
#include "mem/strings.hh"
#include "core/config.hh"
#include "util/file.hh"
#include <fmt/format.h>
#include <ctime>

namespace hojy::scene {

Dead::~Dead() {
    delete big_;
}

void Dead::init() {
    renderer_->enableLinear(true);
    big_ = gWindow->globalTextureMgr().loadFromRAW(util::File::getFileContent(core::config.dataFilePath("DEAD.BIG")), 320, 200);
    renderer_->enableLinear(false);
}

void Dead::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeySpace: case KeyOK: case KeyCancel:
        delete this;
        gWindow->title();
        break;
    default:
        break;
    }
}

void Dead::makeCache() {
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
    auto *ttf = renderer_->ttf();
    auto fsize = ttf->fontSize() * 3 / 2;
    ttf->setColor(68, 68, 68);
    ttf->render(GETCHARNAME(0), x + 100 * w / 320, y + 48 * h / 200, false, fsize);
    ttf->setColor(176, 4, 8);
    auto clock = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(clock);
    tm ltm = *localtime(&t);
    ttf->render(fmt::format(L"{}/{:>2}/{:>2}", ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday), x + 190 * w / 320, y + 10 * h / 200, false, fsize);
    ttf->render(GETTEXT(111), x + 185 * w / 320, y + 30 * h / 200, false, fsize);
    ttf->render(GETTEXT(112), x + 185 * w / 320, y + 50 * h / 200, false, fsize);
    ttf->render(GETTEXT(113), x + 185 * w / 320, y + 70 * h / 200, false, fsize);
    cacheEnd();
}

}
