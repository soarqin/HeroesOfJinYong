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

#include "nodewithcache.hh"

#include "texture.hh"

namespace hojy::scene {

NodeWithCache::~NodeWithCache() {
    delete cache_;
}

void NodeWithCache::makeCenter(int w, int h, int x, int y) {
    if (cacheDirty_) {
        makeCache();
        cacheDirty_ = false;
    }
    Node::makeCenter(w, h, x, y);
}

void NodeWithCache::close() {
    delete cache_;
    cache_ = nullptr;
    Node::close();
}

void NodeWithCache::update() {
    if (cacheDirty_) {
        makeCache();
        cacheDirty_ = false;
    }
}

void NodeWithCache::render() {
    renderer_->renderTexture(cache_, x_, y_, 0, 0, width_, height_, true);
}

void NodeWithCache::cacheBegin() {
    if (!cache_) {
        cache_ = Texture::createAsTarget(renderer_, width_, height_);
        cache_->enableBlendMode(true);
    }
    renderer_->setTargetTexture(cache_);
}

void NodeWithCache::cacheEnd() {
    renderer_->setTargetTexture(nullptr);
}

}
