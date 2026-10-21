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

#include "extendednode.hh"

#include "colorpalette.hh"
#include "core/config.hh"

namespace hojy::scene {

void ExtendedNode::setTimeToClose(std::uint32_t millisec) {
    if (millisec <= 0) {
        closeType_ = 0;
        closeDeadline_ = 0;
    } else {
        closeType_ = 0;
        closeDeadline_ = phaseTime() + millisec * 1000ULL;
    }
}

void ExtendedNode::setWaitForKeyPress() {
    closeType_ = 1;
}

void ExtendedNode::addBox(int x0, int y0, int x1, int y1) {
    boxlist_.emplace_back(std::make_tuple(x0, y0, x1 - x0 + 1, y1 - y0 + 1));
    requestPresentationRefresh();
}

void ExtendedNode::addText(int x, int y, const std::wstring &text, int c0, int c1) {
    textlist_.emplace_back(std::make_tuple(x, y, text, c0, c1));
    requestPresentationRefresh();
}

void ExtendedNode::addTextureResource(int x, int y, TextureKind kind, std::int16_t id,
                                      std::pair<int, int> scale) {
    std::unique_ptr<ExtendedTextureRequest> request;
    if (kind == TextureKind::Head) {
        request = std::make_unique<HeadTextureRequest>(x, y, id, scale);
    } else {
        request = std::make_unique<SubMapTextureRequest>(x, y, id, scale);
    }
    textureRequests_.push_back(std::move(request));
    requestPresentationRefresh();
}

void ExtendedNode::prepareRender() {
    if (!textureRequests_.empty()) {
        std::vector<ResolvedTexture> candidate;
        candidate.reserve(textureRequests_.size());
        for (const auto &request: textureRequests_) {
            const auto *texture = textureProvider_
                ? request->resolve(*textureProvider_) : nullptr;
            if (!texture) {
                onPrepareFailed();
                return;
            }
            candidate.push_back(ResolvedTexture{
                request->x(), request->y(), texture, request->scale()});
        }
        texturelist_ = std::move(candidate);
    } else {
        texturelist_.clear();
    }
    NodeWithCache::prepareRender();
}

void ExtendedNode::checkTimeout() {
    if (closeType_ == 0 && phaseTime() >= closeDeadline_) {
        auto sink = std::move(completionSink_);
        if (sink) { sink->submit({InputKey::None, true}); }
        requestPresentationCleanup();
    }
}

void ExtendedNode::consumeKeyIntent(Node::Key key) {
    if (inputSuspended_) { return; }
    pendingInput_ = key;
}

void ExtendedNode::applyInputLogic() {
    if (inputSuspended_) {
        pendingInput_ = KeyNone;
        return;
    }
    const auto key = pendingInput_;
    pendingInput_ = KeyNone;
    if (closeType_ != 1) { return; }
    auto sink = std::move(completionSink_);
    if (sink) { sink->submit({key, false}); }
    requestPresentationCleanup();
}

bool ExtendedNode::prepareTextResources() {
    auto *ttf = renderer_->ttf();
    bool ready = true;
    for (const auto &entry: textlist_) {
        ready = ttf->prepareText(std::get<2>(entry)) && ready;
    }
    return ready;
}

void ExtendedNode::makeCache() {
    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    auto windowBorder = core::config.windowBorder();
    for (auto &p: boxlist_) {
        int x0, y0, w, h;
        std::tie(x0, y0, w, h) = p;
        renderer_->drawRoundedRect(x0, y0, w, h, windowBorder, 224, 224, 224, 255);
    }
    auto *ttf = renderer_->ttf();
    for (auto &p: textlist_) {
        auto c = std::get<3>(p);
        if (c >= 0 && c < gNormalPalette.size()) {
            std::uint32_t color = gNormalPalette.colors()[c];
            auto *colorptr = reinterpret_cast<const uint8_t*>(&color);
            ttf->setColor(colorptr[0], colorptr[1], colorptr[2]);
        }
        ttf->renderPrepared(std::get<2>(p), std::get<0>(p), std::get<1>(p), true);
    }
    for (const auto &p: texturelist_) {
        renderer_->renderTexture(p.texture, p.x, p.y, p.scale);
    }
    cacheEnd();
}

}
