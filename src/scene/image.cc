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

#include "image.hh"

#include "texture.hh"
#include "window.hh"
#include "util/file.hh"

namespace hojy::scene {

Image::~Image() {
    if (freeOnClose_) {
        delete texture_;
    }
}

bool Image::loadRAW(const std::vector<std::string> &filename, int width, int height) {
    texture_ = gWindow->globalTextureMgr().loadFromRAW(util::File::getFileContent(filename), width, height);
    if (!texture_) { return false; }
    freeOnClose_ = true;
    calcDrawRect();
    return true;
}

void Image::setTexture(const Texture *texture) {
    texture_ = const_cast<Texture*>(texture);
    calcDrawRect();
    freeOnClose_ = false;
}

void Image::render() {
    if (texture_) {
        renderer_->renderTexture(texture_, destX_, destY_, destWidth_, destHeight_, 0, 0, texture_->width(), texture_->height());
    }
}

void Image::calcDrawRect() {
    int width = texture_->width();
    int height = texture_->height();
    if (keepAspectRatio_) {
        auto h = width_ * height / width;
        if (h > height_) {
            destWidth_ = height_ * width / height;
            destHeight_ = height_;
            switch (align_) {
            case 0:
                destX_ = x_;
                break;
            case 1:
                destX_ = x_ + (width_ - destWidth_) / 2;
                break;
            case 2:
                destX_ = x_ + width_ - destWidth_;
                break;
            }
            destY_ = y_;
        } else {
            destWidth_ = width_;
            destHeight_ = h;
            destX_ = x_;
            switch (align_) {
            case 0:
                destY_ = y_;
                break;
            case 1:
                destY_ = y_ + (height_ - destHeight_) / 2;
                break;
            case 2:
                destY_ = y_ + height_ - destHeight_;
                break;
            }
        }
    } else {
        destX_ = x_;
        destY_ = y_;
        destWidth_ = width_;
        destHeight_ = height_;
    }
}

}
