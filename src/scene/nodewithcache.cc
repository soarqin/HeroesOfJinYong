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

#include <stdexcept>

namespace hojy::scene {

NodeWithCache::~NodeWithCache() {
    if (renderer_) {
        auto *bound = renderer_->targetTexture();
        if (bound == buildingCache_ || bound == cache_) {
            if (!renderer_->setTargetTexture(nullptr)) {
                // Never destroy a texture that the renderer still targets.
                // Retaining it is safer than leaving a dangling GPU target.
                return;
            }
        }
    }
    delete buildingCache_;
    delete cache_;
}

void NodeWithCache::update() {
}

void NodeWithCache::prepareRender() {
    const auto requestedPresentationRevision = requestedPresentationRevision_;
    if (preparedPresentationRevision_ != requestedPresentationRevision) {
        cacheDirty_ = true;
    }
    const auto oldX = x_;
    const auto oldY = y_;
    const auto oldWidth = width_;
    const auto oldHeight = height_;
    if (!prepareTextResources()) {
        onPrepareFailed();
        return;
    }
    try {
        ensureLayout();
    } catch (...) {
        x_ = oldX;
        y_ = oldY;
        width_ = oldWidth;
        height_ = oldHeight;
        onPrepareFailed();
        throw;
    }
    if (width_ != oldWidth || height_ != oldHeight) {
        cacheDirty_ = true;
    }
    if (!rebuildCache()) {
        x_ = oldX;
        y_ = oldY;
        width_ = oldWidth;
        height_ = oldHeight;
        onPrepareFailed();
    } else {
        if (layoutCenterRequested_) {
            Node::makeCenter(layoutCenterWidth_, layoutCenterHeight_,
                             layoutCenterX_, layoutCenterY_);
        }
        preparedPresentationRevision_ = requestedPresentationRevision;
    }
}

bool NodeWithCache::rebuildCache() {
    if (!cacheDirty_) { return true; }
    // A committed cache must never be destroyed while it is still bound by
    // the renderer.  This can happen when a nested preparation failed to
    // restore its caller's target; preserve the old cache and retry later.
    if (cache_ && renderer_ && renderer_->targetTexture() == cache_) {
        return false;
    }
    if (buildingCache_) {
        if (renderer_ && renderer_->targetTexture() == buildingCache_
            && !renderer_->setTargetTexture(nullptr)) {
            return false;
        }
        delete buildingCache_;
        buildingCache_ = nullptr;
    }
    auto *candidate = Texture::createAsTarget(renderer_, width_, height_);
    if (!candidate || !candidate->enableBlendMode(true)) {
        delete candidate;
        return false;
    }
    auto *previousTarget = renderer_ ? renderer_->targetTexture() : nullptr;
    buildingCache_ = candidate;
    try {
        makeCache();
    } catch (...) {
        const bool restored = !renderer_
            || renderer_->setTargetTexture(previousTarget);
        if (!restored && renderer_->targetTexture() == candidate) {
            buildingCache_ = candidate;
            return false;
        }
        buildingCache_ = nullptr;
        delete candidate;
        return false;
    }
    if (renderer_ && !renderer_->setTargetTexture(previousTarget)) {
        if (renderer_->targetTexture() == candidate) {
            buildingCache_ = candidate;
            return false;
        }
        buildingCache_ = nullptr;
        delete candidate;
        return false;
    }
    buildingCache_ = nullptr;
    delete cache_;
    cache_ = candidate;
    cacheDirty_ = false;
    return true;
}

void NodeWithCache::makeCenter(int w, int h, int x, int y) {
    layoutCenterRequested_ = true;
    layoutCenterWidth_ = w;
    layoutCenterHeight_ = h;
    layoutCenterX_ = x;
    layoutCenterY_ = y;
    requestPresentationRefresh();
}

void NodeWithCache::close() {
    if (renderer_) {
        auto *bound = renderer_->targetTexture();
        if (bound == buildingCache_ || bound == cache_) {
            if (!renderer_->setTargetTexture(nullptr)) {
                return;
            }
        }
    }
    delete buildingCache_;
    buildingCache_ = nullptr;
    delete cache_;
    cache_ = nullptr;
    Node::close();
}

void NodeWithCache::render() const {
    if (!cache_) { return; }
    renderer_->renderTexture(cache_, x_, y_, 0, 0, cache_->width(), cache_->height(), true);
}

void NodeWithCache::cacheBegin() {
    if (!buildingCache_ || !renderer_ || !renderer_->setTargetTexture(buildingCache_)) {
        throw std::runtime_error("failed to bind render cache target");
    }
}

void NodeWithCache::cacheEnd() {
    if (!renderer_ || !renderer_->setTargetTexture(nullptr)) {
        throw std::runtime_error("failed to release render cache target");
    }
}

}
