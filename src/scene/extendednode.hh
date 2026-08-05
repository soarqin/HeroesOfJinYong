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

#include "nodewithcache.hh"
#include <vector>
#include <tuple>
#include <string>
#include <functional>
#include <cstdint>
#include <memory>

namespace hojy::scene {

class Texture;

class ExtendedTextureProvider {
public:
    virtual ~ExtendedTextureProvider() = default;
    [[nodiscard]] virtual const Texture *subMapTexture(
        std::int16_t id) const = 0;
    [[nodiscard]] virtual const Texture *headTexture(
        std::int16_t id) const = 0;
};

class ExtendedTextureRequest {
public:
    ExtendedTextureRequest(int x, int y, std::pair<int, int> scale) noexcept:
        x_(x), y_(y), scale_(scale) {}
    virtual ~ExtendedTextureRequest() = default;

    [[nodiscard]] virtual const Texture *resolve(
        const ExtendedTextureProvider &provider) const = 0;
    [[nodiscard]] int x() const noexcept { return x_; }
    [[nodiscard]] int y() const noexcept { return y_; }
    [[nodiscard]] std::pair<int, int> scale() const noexcept { return scale_; }

private:
    int x_ = 0;
    int y_ = 0;
    std::pair<int, int> scale_ = {1, 1};
};

class SubMapTextureRequest final: public ExtendedTextureRequest {
public:
    SubMapTextureRequest(int x, int y, std::int16_t id,
                         std::pair<int, int> scale) noexcept:
        ExtendedTextureRequest(x, y, scale), id_(id) {}

    [[nodiscard]] const Texture *resolve(
        const ExtendedTextureProvider &provider) const override {
        return provider.subMapTexture(id_);
    }

private:
    std::int16_t id_ = -1;
};

class HeadTextureRequest final: public ExtendedTextureRequest {
public:
    HeadTextureRequest(int x, int y, std::int16_t id,
                       std::pair<int, int> scale) noexcept:
        ExtendedTextureRequest(x, y, scale), id_(id) {}

    [[nodiscard]] const Texture *resolve(
        const ExtendedTextureProvider &provider) const override {
        return provider.headTexture(id_);
    }

private:
    std::int16_t id_ = -1;
};

struct ExtendedInputResult final {
    InputKey key = InputKey::None;
    bool timedOut = false;
};

class ExtendedInputCompletionSink {
public:
    virtual ~ExtendedInputCompletionSink() = default;
    virtual void submit(ExtendedInputResult result) = 0;
};

class ExtendedNode: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;
    enum class TextureKind: std::uint8_t { SubMap, Head };
    void setTimeToClose(std::uint32_t millisec);
    void setWaitForKeyPress();
    void addBox(int x0, int y0, int x1, int y1);
    void addText(int x, int y, const std::wstring &text, int c0, int c1);
    void setTexturePort(std::unique_ptr<ExtendedTextureProvider> provider) {
        textureProvider_ = std::move(provider);
        requestPresentationRefresh();
    }
    void addTextureResource(int x, int y, TextureKind kind, std::int16_t id, std::pair<int, int> scale);
    void prepareRender() override;
    void setInputCompletionSink(std::unique_ptr<ExtendedInputCompletionSink> sink) {
        completionSink_ = std::move(sink);
    }
    // A session can be invalidated during fixed logic while the node remains
    // in the tree until the next render-preparation pass.
    void suspendInput() noexcept { inputSuspended_ = true; }
    void checkTimeout();

    [[nodiscard]] bool acceptsInput() const noexcept override {
        return !inputSuspended_ && Node::acceptsInput();
    }

    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;

protected:
    bool prepareTextResources() override;
    void makeCache() override;

private:
    int closeType_ = -1;
    std::uint64_t closeDeadline_ = 0;
    std::vector<std::tuple<int, int, int, int>> boxlist_;
    std::vector<std::tuple<int, int, std::wstring, int, int>> textlist_;
    struct ResolvedTexture final {
        int x = 0;
        int y = 0;
        const Texture *texture = nullptr;
        std::pair<int, int> scale = {1, 1};
    };
    std::vector<std::unique_ptr<ExtendedTextureRequest>> textureRequests_;
    std::vector<ResolvedTexture> texturelist_;
    std::unique_ptr<ExtendedTextureProvider> textureProvider_;
    std::unique_ptr<ExtendedInputCompletionSink> completionSink_;
    Key pendingInput_ = KeyNone;
    bool inputSuspended_ = false;
};

}
