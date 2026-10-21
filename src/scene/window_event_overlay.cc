#include "window.hh"

#include "extendednode.hh"
#include "util/math.hh"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

namespace hojy::scene {
namespace {

constexpr int EventCanvasWidth = 320;
constexpr int EventCanvasHeight = 200;

struct EventViewport final {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

EventViewport eventViewport(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) { return {}; }
    auto width = windowWidth;
    auto height = static_cast<int>(
        static_cast<std::int64_t>(windowWidth) * EventCanvasHeight
        / EventCanvasWidth);
    if (height > windowHeight) {
        height = windowHeight;
        width = static_cast<int>(
            static_cast<std::int64_t>(windowHeight) * EventCanvasWidth
            / EventCanvasHeight);
    }
    return {(windowWidth - width) / 2, (windowHeight - height) / 2,
            width, height};
}

std::pair<int, int> transformEventPoint(int x, int y,
                                        int windowWidth, int windowHeight) {
    const auto viewport = eventViewport(windowWidth, windowHeight);
    return {
        viewport.x + static_cast<int>(
            static_cast<std::int64_t>(viewport.width) * x / EventCanvasWidth),
        viewport.y + static_cast<int>(
            static_cast<std::int64_t>(viewport.height) * y / EventCanvasHeight),
    };
}

std::int16_t directionalValue(Node::Key key) {
    switch (key) {
    case Node::KeyLeft: return 154;
    case Node::KeyRight: return 156;
    case Node::KeyUp: return 158;
    case Node::KeyDown: return 152;
    default: return 0;
    }
}

class EventOverlayTextureProvider final: public ExtendedTextureProvider {
public:
    explicit EventOverlayTextureProvider(Window &window): window_(window) {}

    [[nodiscard]] const Texture *subMapTexture(
            std::int16_t id) const override {
        return window_.smpTexture(id);
    }

    [[nodiscard]] const Texture *headTexture(
            std::int16_t id) const override {
        return window_.headTexture(id);
    }

private:
    Window &window_;
};

class EventOverlayInputSink final: public ExtendedInputCompletionSink {
public:
    EventOverlayInputSink(std::uint64_t token,
                          std::shared_ptr<EventInputContinuation> continuation,
                          bool directional) noexcept:
        token_(token), continuation_(std::move(continuation)),
        directional_(directional) {}

    void submit(ExtendedInputResult result) override {
        if (!continuation_) { return; }
        const auto value = directional_
            ? directionalValue(result.key)
            : static_cast<std::int16_t>(result.key == Node::KeyOK ? 0 : 1);
        continuation_->submit({token_, value});
        continuation_.reset();
    }

private:
    std::uint64_t token_ = 0;
    std::shared_ptr<EventInputContinuation> continuation_;
    bool directional_ = false;
};

}

class EventOverlaySurfaceAdapter final : public EventOverlaySurface {
public:
    EventOverlaySurfaceAdapter(Window &window, ExtendedNode &overlay)
        : window_(window), overlay_(overlay) {}

    void addText(std::int16_t x, std::int16_t y, std::wstring text,
                 std::int16_t color,
                 std::int16_t alternateColor) override {
        const auto point = transformEventPoint(x, y, window_.width(), window_.height());
        overlay_.addText(point.first, point.second, text, color, alternateColor);
    }

    void addBox(std::int16_t x0, std::int16_t y0,
                std::int16_t x1, std::int16_t y1) override {
        const auto first = transformEventPoint(x0, y0, window_.width(), window_.height());
        const auto second = transformEventPoint(x1, y1, window_.width(), window_.height());
        overlay_.addBox(first.first, first.second, second.first, second.second);
    }

    void addSubMapSprite(std::int16_t x, std::int16_t y,
                         std::int16_t id) override {
        addTexture(x, y, ExtendedNode::TextureKind::SubMap, id);
    }

    void addHeadSprite(std::int16_t x, std::int16_t y,
                       std::int16_t id) override {
        addTexture(x, y, ExtendedNode::TextureKind::Head, id);
    }

    void waitForDirectionalInput(
            std::uint64_t token,
            std::shared_ptr<EventInputContinuation> continuation) override {
        overlay_.setWaitForKeyPress();
        overlay_.setInputCompletionSink(std::make_unique<EventOverlayInputSink>(
            token, std::move(continuation), true));
    }

    void waitForConfirmInput(
            std::uint64_t token,
            std::shared_ptr<EventInputContinuation> continuation) override {
        overlay_.setWaitForKeyPress();
        overlay_.setInputCompletionSink(std::make_unique<EventOverlayInputSink>(
            token, std::move(continuation), false));
    }

    void keepAlive() override {}

private:
    void addTexture(std::int16_t x, std::int16_t y,
                    ExtendedNode::TextureKind kind, std::int16_t id) {
        const auto viewport = eventViewport(window_.width(), window_.height());
        const auto point = transformEventPoint(x, y, window_.width(), window_.height());
        overlay_.addTextureResource(
            point.first, point.second, kind, id,
            util::calcSmallestDivision(viewport.width, EventCanvasWidth));
    }

    Window &window_;
    ExtendedNode &overlay_;
};

void Window::showEventOverlay(EventOverlayRequest request) {
    auto *owner = dynamic_cast<MapWithEvent *>(map_);
    if (!owner || !request.operation
        || !owner->isCurrentEventSession(request.sessionToken)) {
        return;
    }

    if (eventOverlay_
        && (eventOverlayOwner_ != owner
            || eventOverlaySession_ != request.sessionToken
            || eventOverlay_->parent() != owner
            || eventOverlay_->deleteRequested())) {
        auto *stale = eventOverlay_;
        eventOverlay_ = nullptr;
        eventOverlayOwner_ = nullptr;
        eventOverlaySession_ = 0;
        stale->requestDelete();
    }

    if (!eventOverlay_) {
        auto *overlay = new ExtendedNode(owner, 0, 0, width_, height_);
        overlay->setTexturePort(
            std::make_unique<EventOverlayTextureProvider>(*this));
        eventOverlay_ = overlay;
        eventOverlayOwner_ = owner;
        eventOverlaySession_ = request.sessionToken;
    }

    EventOverlaySurfaceAdapter surface(*this, *eventOverlay_);
    request.operation->apply(surface);
}

void Window::clearEventPresentation(EventPresentationClearRequest request) {
    if (request.sessionToken == 0) { return; }

    if (eventOverlay_ && eventOverlaySession_ == request.sessionToken) {
        auto *overlay = eventOverlay_;
        eventOverlay_ = nullptr;
        eventOverlayOwner_ = nullptr;
        eventOverlaySession_ = 0;
        overlay->suspendInput();
        pendingEventOverlayCleanup_.push_back(overlay);
    }

    if (eventFadeOwner_ && eventFadeSession_ == request.sessionToken) {
        auto *owner = eventFadeOwner_;
        eventFadeOwner_ = nullptr;
        eventFadeSession_ = 0;
        invalidateTransitions();
        owner->requestFadeCleanup();
        pendingEventFadeCleanup_.push_back(owner);
    }
}

void Window::preparePresentationCleanup() noexcept {
    for (auto *overlay: pendingEventOverlayCleanup_) {
        if (overlay) { overlay->requestDelete(); }
    }
    pendingEventOverlayCleanup_.clear();
    for (auto *owner: pendingEventFadeCleanup_) {
        if (owner) { owner->requestFadeCleanup(); }
    }
    pendingEventFadeCleanup_.clear();
}

void Window::fadeEventIn(EventFadeRequest request) {
    auto *owner = dynamic_cast<MapWithEvent *>(map_);
    if (!owner || !owner->isCurrentEventSession(request.sessionToken)
        || request.continuationToken == 0 || !request.continuation) {
        return;
    }
    if (eventFadeOwner_) {
        clearEventPresentation({eventFadeSession_});
    }

    const auto transition = beginTransition();
    const auto session = request.sessionToken;
    const auto token = request.continuationToken;
    auto continuation = std::move(request.continuation);
    eventFadeOwner_ = owner;
    eventFadeSession_ = session;
    owner->fadeIn([windowLifetime = windowLifetimeHandle(), owner, session,
                   transition, token,
                   continuation = std::move(continuation)]() mutable {
        auto windowState = windowLifetime.lock();
        if (!windowState || !windowState->owner) { return; }
        auto &window = *windowState->owner;
        if (window.eventFadeOwner_ == owner
            && window.eventFadeSession_ == session) {
            window.eventFadeOwner_ = nullptr;
            window.eventFadeSession_ = 0;
        }
        if (!window.isCurrentTransition(transition) || window.map_ != owner
            || !owner->isCurrentEventSession(session)) {
            return;
        }
        continuation->submit({token, 0});
    });
}

void Window::fadeEventOut(EventFadeRequest request) {
    auto *owner = dynamic_cast<MapWithEvent *>(map_);
    if (!owner || !owner->isCurrentEventSession(request.sessionToken)
        || request.continuationToken == 0 || !request.continuation) {
        return;
    }
    if (eventFadeOwner_) {
        clearEventPresentation({eventFadeSession_});
    }

    const auto transition = beginTransition();
    const auto session = request.sessionToken;
    const auto token = request.continuationToken;
    auto continuation = std::move(request.continuation);
    eventFadeOwner_ = owner;
    eventFadeSession_ = session;
    owner->fadeOut([windowLifetime = windowLifetimeHandle(), owner, session,
                    transition, token,
                    continuation = std::move(continuation)]() mutable {
        auto windowState = windowLifetime.lock();
        if (!windowState || !windowState->owner) { return; }
        auto &window = *windowState->owner;
        if (window.eventFadeOwner_ == owner
            && window.eventFadeSession_ == session) {
            window.eventFadeOwner_ = nullptr;
            window.eventFadeSession_ = 0;
        }
        if (!window.isCurrentTransition(transition) || window.map_ != owner
            || !owner->isCurrentEventSession(session)) {
            return;
        }
        continuation->submit({token, 0});
    });
}

void Window::detachEventOverlay(ExtendedNode *overlay,
                                std::uint64_t session) noexcept {
    if (eventOverlay_ != overlay || eventOverlaySession_ != session) { return; }
    eventOverlay_ = nullptr;
    eventOverlayOwner_ = nullptr;
    eventOverlaySession_ = 0;
}

}
