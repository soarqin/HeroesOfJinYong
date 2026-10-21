#include "window.hh"

#include "menu_action_adapter.hh"

#include "app/text_input.hh"
#include "menu.hh"

#include <memory>

namespace hojy::scene {

void Window::useQuestItem(std::int16_t itemId) {
    auto *mapev = dynamic_cast<MapWithEvent *>(map_);
    if (mapev) { mapev->onUseItem(itemId); }
}

void Window::forceEvent(std::int16_t eventId) {
    auto *mapev = dynamic_cast<MapWithEvent *>(map_);
    if (mapev) {
        mapev->runEvent(eventId);
    } else if (subMap_) {
        subMap_->runEvent(eventId);
    }
}

void Window::closePopup() {
    if (!popup_) { return; }
    // Input and node-update callbacks run under the fixed-logic dispatch
    // barrier.  Deleting the active root synchronously would destroy the
    // object whose callback is still on the stack.  Defer root destruction to
    // Window::applyDeferredNodes() instead.
    if (processingStage_) {
        auto *victim = popup_;
        const bool owned = freeOnClose_;
        popup_ = nullptr;
        freeOnClose_ = false;
        if (owned) {
            victim->requestDelete();
        } else {
            victim->close();
        }
        deferredPopup_ = victim;
        deferredPopupOwned_ = owned;
        return;
    }
    if (freeOnClose_) {
        delete popup_;
    } else {
        popup_->close();
    }
    popup_ = nullptr;
}

void Window::replacePopup(Node *popup, bool owned) {
    if (!popup || processingStage_) { return; }
    if (deferredPopup_) { applyDeferredNodes(); }
    invalidateTransitions();
    closePopup();
    popup_ = popup;
    freeOnClose_ = owned;
}

void Window::endPopup(bool close, bool result) {
    if (close) {
        closePopup();
    }
    auto *mapev = dynamic_cast<MapWithEvent *>(map_);
    if (mapev) { mapev->continueEvents(result); }
}

void Window::beginTextInput() {
    ::hojy::app::textInput().begin();
}

void Window::setTextInputRect(int x, int y, int w, int h) {
    ::hojy::app::textInput().setRect(x, y, w, h);
}

void Window::endTextInput() {
    ::hojy::app::textInput().end();
}

void Window::showEventMenu(EventMenuRequest request) {
    const auto continuation = std::move(request.continuation);
    const auto token = request.continuationToken;
    auto *owner = dynamic_cast<MapWithEvent *>(map_);
    if (!continuation || token == 0 || !owner
        || !owner->isCurrentEventSession(request.sessionToken)) {
        if (continuation) {
            continuation->submit({token, 0, false});
        }
        return;
    }

    constexpr int originalWidth = 320;
    constexpr int originalHeight = 200;
    auto viewportWidth = width_;
    auto viewportHeight = static_cast<int>(
        static_cast<std::int64_t>(width_) * originalHeight / originalWidth);
    if (viewportHeight > height_) {
        viewportHeight = height_;
        viewportWidth = static_cast<int>(
            static_cast<std::int64_t>(height_) * originalWidth / originalHeight);
    }
    const auto viewportX = (width_ - viewportWidth) / 2;
    const auto viewportY = (height_ - viewportHeight) / 2;
    const auto x = viewportX + static_cast<int>(
        static_cast<std::int64_t>(viewportWidth) * request.x / originalWidth);
    const auto y = viewportY + static_cast<int>(
        static_cast<std::int64_t>(viewportHeight) * request.y / originalHeight);
    const auto menuWidth = request.width > 0 ? request.width : width_ - x;
    const auto menuHeight = request.height > 0 ? request.height : height_ - y;
    auto *menu = new MenuTextList(owner, x, y, menuWidth, menuHeight);
    bindCommandSink(menu);
    auto completed = std::make_shared<bool>(false);
    auto controller = std::make_shared<ActionMenuController>();
    for (std::size_t index = 0; index < request.items.size(); ++index) {
        const auto entryId = static_cast<std::int32_t>(index + 1);
        controller->bind(entryId, makeMenuAction(
            [menu, continuation, completed, token](MenuSelection selection) {
            if (*completed) { return; }
            *completed = true;
            continuation->submit({
                token, static_cast<std::int16_t>(selection.entryId), true});
            menu->requestDelete();
        }));
    }
    controller->bindCancel(makeMenuAction(
        [continuation, completed, token](MenuSelection) {
            if (*completed) { return; }
            *completed = true;
            continuation->submit({token, 0, false});
        }));
    menu->setSelectionSink(std::move(controller));
    MenuEntries entries;
    entries.reserve(request.items.size());
    for (std::size_t index = 0; index < request.items.size(); ++index) {
        entries.push_back({static_cast<std::int32_t>(index + 1),
                           request.items[index], L"", true});
    }
    menu->popup(entries);
}

}
