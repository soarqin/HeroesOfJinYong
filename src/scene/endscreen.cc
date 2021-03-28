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

#include "endscreen.hh"

#include "colorpalette.hh"
#include "window.hh"
#include "data/grpdata.hh"

namespace hojy::scene {

enum {
    OrigWidth = 320,
    OrigHeight = 200,
};

void EndScreen::init() {
    data::GrpData::DataSet dset;
    if (data::GrpData::loadData("ENDWORD.IDX", "ENDWORD.GRP", dset)) {
        wordTexMgr_.setRenderer(renderer_);
        wordTexMgr_.setPalette(gEndPalette);
        wordTexMgr_.loadFromRLE(dset);
    }
    dset.clear();
    if (data::GrpData::loadData("KEND.IDX", "KEND.GRP", dset)) {
        imgTexMgr_.setRenderer(renderer_);
        imgTexMgr_.setPalette(gEndPalette);
        imgTexMgr_.loadFromRAW(dset, OrigWidth, OrigHeight);
    }
    stage_ = 0, frame_ = 0;
    frameTotal_ = 60;
}

void EndScreen::handleKeyInput(Node::Key key) {
    if (key == KeyOK || key == KeySpace || key == KeyCancel) {
        switch (stage_) {
        case 2:
            if (frame_ + 1 < frameTotal_) { break; }
            stage_ = 3;
            frame_ = 0;
            frameTotal_ = 60000;
            y_ = height_;
            update();
            break;
        case 3:
            if (frame_ + 1 < frameTotal_) { break; }
            gWindow->forceQuit();
            break;
        default:
            break;
        }
    }
}

void EndScreen::render() {
    if (cacheDirty_) {
        makeCache();
        cacheDirty_ = false;
    }
    renderer_->clear(0, 0, 0, 255);
    renderer_->renderTexture(cache_, x_, y_, w_, h_, 0, 0, tw_, th_, true);

    switch (stage_) {
    case 0:
        if (frame_ < frameTotal_) {
            ++frame_;
        } else {
            stage_ = 1; frame_ = 0;
            frameTotal_ = 600;
            y_ = height_;
            update();
            break;
        }
        break;
    case 1:
        if (frame_ < frameTotal_) {
            ++frame_;
            y_ -= 2;
        } else {
            stage_ = 2; frame_ = 0;
            frameTotal_ = imgTexMgr_.size();
            update();
            break;
        }
        break;
    case 2:
        if (frame_ + 1 < frameTotal_) {
            ++ frame_;
            update();
        }
        break;
    case 3:
        if (frame_ < frameTotal_) {
            ++frame_;
            y_ -= 2;
        }
        break;
    default:
        break;
    }
}

void EndScreen::makeCache() {
    if (!cache_) {
        renderer_->enableLinear();
        cache_ = Texture::createAsTarget(renderer_, 512, 4096);
        cache_->enableBlendMode(true);
        renderer_->enableLinear(false);
    }
    cacheBegin();
    int w = width_, h = width_ * OrigHeight / OrigWidth;
    if (h > height_) {
        h = height_;
        w = height_ * OrigWidth / OrigHeight;
    }
    int x = (width_ - w) / 2;
    int y = (height_ - h) / 2;
    renderer_->clear(0, 0, 0, 255);
    switch (stage_) {
    case 0: {
        const auto *tex = wordTexMgr_[0];
        tw_ = tex->width(), th_ = tex->height();
        w_ = tw_ * w / OrigWidth, h_ = th_ * h / OrigHeight;
        x_ = x + (w - w_) / 2;
        y_ = y + (h - h_) / 2;
        renderer_->renderTexture(tex, 0, 0);
        break;
    }
    case 1: {
        int cy = 0; tw_ = 0;
        for (int i = 1; i <= 2; ++i) {
            const auto *tex = wordTexMgr_[i];
            tw_ = std::max(tw_, tex->width());
            renderer_->renderTexture(tex, 0, cy);
            cy += 15 + tex->height();
        }
        th_ = cy;
        w_ = tw_ * w / OrigWidth;
        h_ = th_ * h / OrigHeight;
        x_ = x + (w - w_) / 2;
        break;
    }
    case 2: {
        const auto *tex = imgTexMgr_[frame_];
        renderer_->renderTexture(tex, 0, 0);
        x_ = x;
        y_ = y;
        w_ = w;
        h_ = h;
        tw_ = OrigWidth;
        th_ = OrigHeight;
        break;
    }
    case 3: {
        int cy = 0;
        int lh = wordTexMgr_[22]->height();
        tw_ = 0;
        for (int i = 3; i <= 22; ++i) {
            const auto *tex = wordTexMgr_[i];
            tw_ = std::max(tw_, tex->width() + (i == 22 ? 10 : 20));
            renderer_->renderTexture(tex, i == 22 ? 10 : 20, cy);
            cy += (i >= 20 ? 100 : 15) + tex->height();
        }
        cy -= 85;
        th_ = cy;
        w_ = tw_ * w / OrigWidth;
        h_ = th_ * h / OrigHeight;
        x_ = x + (w - w_) / 2;
        frameTotal_ = (h_ + (height_ - lh * h / OrigHeight) / 2) / 2;
        break;
    }
    }
    cacheEnd();
}

}
