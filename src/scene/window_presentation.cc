#include "window.hh"

#include "charlistmenu.hh"
#include "item_selection_controller.hh"
#include "messagebox.hh"
#include "menu_action_adapter.hh"
#include "talkbox.hh"
#include "warfield.hh"

#include "core/config.hh"

#include <utility>

namespace hojy::scene {
namespace {

MessageBox::Type popupType(std::uint8_t value) {
    switch (value) {
    case 1: return MessageBox::PressToCloseThis;
    case 2: return MessageBox::PressToCloseParent;
    case 3: return MessageBox::PressToCloseTop;
    case 4: return MessageBox::YesNo;
    default: return MessageBox::Normal;
    }
}

}

void Window::runTalk(const std::wstring &text, std::int16_t headId,
                     std::int16_t position) {
    if (popup_) {
        auto *map = dynamic_cast<MapWithEvent *>(map_);
        if (map) { map->continueEvents(false); }
        return;
    }
    if (!talkBox_) {
        const auto border = width_ / 12;
        talkBox_ = new TalkBox(
            renderer_, border, border, width_ - border * 2, height_ - border * 2);
        bindCommandSink(talkBox_);
        dynamic_cast<TalkBox *>(talkBox_)->setHeadTextureProvider(
            [this](std::int16_t id) { return headTexture(id); });
    }
    dynamic_cast<TalkBox *>(talkBox_)->popup(text, headId, position);
    if (popup_ != talkBox_) { replacePopup(talkBox_, false); }
}

bool Window::battlePresentationAvailable(
        std::uint64_t token, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage,
        std::int16_t expectedActorId) const noexcept {
    return isCurrentBattleSession(warfield_, token)
        && warfield_->ready() && map_ == warfield_
        && warfield_->matchesPresentationContext(
            token, actionGeneration, expectedStage, expectedActorId);
}

void Window::showCharacterSelection(CharacterSelectionRequest request) {
    auto *parent = popup_ ? popup_ : map_;
    if (!parent || request.characters.rows.empty()) {
        if (request.continuation) {
            request.continuation->submit({request.continuationToken, -1, false});
        }
        return;
    }

    const int border = core::config.windowBorder();
    const int x = request.x >= 0 ? request.x : border * 4;
    const int y = request.y >= 0 ? request.y : border * 4;
    const int width = request.width > 0 ? request.width : width_ - x - border * 2;
    const int height = request.height > 0 ? request.height : height_ - y - border * 2;
    auto *menu = new CharListMenu(parent, x, y, width, height);
    bindCommandSink(menu);

    const auto token = request.continuationToken;
    auto continuation = std::move(request.continuation);
    auto completed = std::make_shared<bool>(false);
    auto controller = std::make_shared<ActionMenuController>();
    controller->bindDefault(makeMenuAction(
        [continuation, completed, token, menu](MenuSelection selection) mutable {
                   if (*completed) { return; }
                   *completed = true;
                   if (continuation) {
                       continuation->submit({token,
                           static_cast<std::int16_t>(selection.entryId), true});
                   }
                   menu->requestDelete();
               }));
    controller->bindCancel(makeMenuAction(
        [continuation, completed, token](MenuSelection) mutable {
            if (*completed) { return; }
            *completed = true;
            if (continuation) {
                continuation->submit({token, -1, false});
            }
        }));
    menu->init(std::move(request.characters), std::move(controller));
    menu->makeCenter(width_, height_, 0, 0);
}

void Window::showItemMessage(ItemMessageRequest request) {
    if (!renderer_ || (!popup_ && !map_)) {
        if (request.continuation) {
            request.continuation->submit({request.continuationToken, false});
        }
        return;
    }
    auto *messageBox = popup_
        ? new MessageBox(popup_, 0, 0, width_, height_ * 4 / 5)
        : new MessageBox(renderer_, 0, 0, width_, height_ * 4 / 5);
    if (!popup_) {
        bindCommandSink(messageBox);
        replacePopup(messageBox, true);
    }

    const auto token = request.continuationToken;
    auto continuation = std::move(request.continuation);
    auto completed = std::make_shared<bool>(false);
    const auto type = popupType(request.popupType);
    if (type == MessageBox::YesNo) {
        messageBox->setResultSink(makeMessageBoxResultSink(
            [continuation, completed, token, messageBox](MessageBoxResult result) mutable {
                if (*completed) { return; }
                *completed = true;
                if (continuation) {
                    continuation->submit({token, result.accepted});
                }
                messageBox->requestDelete();
            }));
    } else if (continuation) {
        messageBox->setResultSink(makeMessageBoxResultSink(
            [continuation, completed, token](MessageBoxResult result) mutable {
                if (*completed) { return; }
                *completed = true;
                continuation->submit({token, result.accepted});
            }));
    }
    messageBox->popup(request.text, type);
}

void Window::showBattleDirectionSelection(BattleDirectionSelectionRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::DirectionSelection,
            request.actorId)) { return; }
    warfield_->presentDirectionSelection(std::move(request));
}

void Window::showBattleSkillLevelUp(BattleSkillLevelUpRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::SkillLevelUp,
            request.actorId)) { return; }
    warfield_->presentSkillLevelUp(std::move(request));
}

void Window::showBattleItemResult(BattleItemResultRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::ItemResult,
            request.actorId)) { return; }
    warfield_->presentItemResult(std::move(request));
}

void Window::showBattleMenu(BattleMenuRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::PlayerMenu,
            request.actorId)) { return; }
    warfield_->presentPlayerMenu(std::move(request));
}

void Window::showBattleItemSelection(BattleItemSelectionRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::ItemSelection,
            request.actorId)) { return; }
    warfield_->presentItemSelection(std::move(request));
}

void Window::showBattleStatusSelection(BattleStatusSelectionRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::StatusSelection,
            request.actorId)) { return; }
    warfield_->presentStatusSelection(std::move(request));
}

void Window::showBattleFinishMessages(BattleFinishMessagesRequest request) {
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::FinishMessages)) { return; }
    warfield_->presentFinishMessages(std::move(request));
}

}
