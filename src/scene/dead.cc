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

#include "window_command.hh"
#include "window.hh"
#include "texture.hh"
#include "colorpalette.hh"
#include "world/strings.hh"
#include "core/config.hh"
#include "util/file.hh"
#include <fmt/xchar.h>
#include <ctime>

namespace hojy::scene {

Dead::~Dead() {
    delete big_;
}

bool Dead::init() {
    renderer_->enableLinear(true);
    auto *candidate = Texture::loadFromRAW(renderer_, util::File::getFileContent(core::config.dataFilePath("DEAD.BIG")), 320, 200, gNormalPalette);
    renderer_->enableLinear(false);
    if (!candidate) { return false; }
    delete big_;
    big_ = candidate;
    requestPresentationRefresh();
    return true;
}

bool Dead::prepareTextResources() {
    auto *ttf = renderer_->ttf();
    const auto fontSize = ttf->fontSize() * 3 / 2;
    nameText_ = GETCHARNAME(0);
    auto now = time(nullptr);
    tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localTime = *localtime(&now);
#endif
    dateText_ = fmt::format(L"{}/{:>2}/{:>2}", localTime.tm_year + 1900,
                            localTime.tm_mon + 1, localTime.tm_mday);
    messageText_[0] = GETTEXT(111);
    messageText_[1] = GETTEXT(112);
    messageText_[2] = GETTEXT(113);
    bool ready = ttf->prepareText(nameText_, fontSize);
    ready = ttf->prepareText(dateText_, fontSize) && ready;
    for (const auto &message: messageText_) {
        ready = ttf->prepareText(message, fontSize) && ready;
    }
    return ready;
}

void Dead::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void Dead::applyInputLogic() {
    const auto key = pendingInput_;
    pendingInput_ = KeyNone;
    switch (key) {
    case KeySpace: case KeyOK: case KeyCancel:
        requestPresentationCleanup();
        postSceneCommand(this, [](SceneCommandContext &context) { context.title(); });
        break;
    default:
        break;
    }
}

void Dead::makeCache() {
    if (!big_) { return; }
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
    ttf->renderPrepared(nameText_, x + 100 * w / 320, y + 48 * h / 200, false, fsize);
    ttf->setColor(176, 4, 8);
    ttf->renderPrepared(dateText_, x + 190 * w / 320, y + 10 * h / 200, false, fsize);
    ttf->renderPrepared(messageText_[0], x + 185 * w / 320, y + 30 * h / 200, false, fsize);
    ttf->renderPrepared(messageText_[1], x + 185 * w / 320, y + 50 * h / 200, false, fsize);
    ttf->renderPrepared(messageText_[2], x + 185 * w / 320, y + 70 * h / 200, false, fsize);
    cacheEnd();
}

}
