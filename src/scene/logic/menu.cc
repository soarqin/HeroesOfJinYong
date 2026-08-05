#include "menu.hh"

namespace hojy::scene {

void ActionMenuController::bind(
        std::int32_t entryId, std::unique_ptr<MenuAction> action) {
    if (!action) { return; }
    actions_[entryId] = std::move(action);
}

void ActionMenuController::bindDefault(std::unique_ptr<MenuAction> action) {
    defaultAction_ = std::move(action);
}

void ActionMenuController::bindCancel(std::unique_ptr<MenuAction> action) {
    cancelAction_ = std::move(action);
}

void ActionMenuController::submit(MenuSelection selection) {
    if (selection.gesture == MenuGesture::Cancel) {
        if (cancelAction_) { cancelAction_->execute(std::move(selection)); }
        return;
    }
    const auto ite = actions_.find(selection.entryId);
    if (ite != actions_.end() && ite->second) {
        ite->second->execute(std::move(selection));
    } else if (defaultAction_) {
        defaultAction_->execute(std::move(selection));
    }
}

std::unique_ptr<MenuInputAction> VerticalMenuInputMode::keyAction(
        InputKey key) const {
    switch (key) {
    case InputKey::Up:
        return std::make_unique<MoveMenuSelectionAction>(-1, true);
    case InputKey::Down:
        return std::make_unique<MoveMenuSelectionAction>(1, true);
    case InputKey::Left:
        return std::make_unique<SelectMenuBoundaryAction>(false);
    case InputKey::Right:
        return std::make_unique<SelectMenuBoundaryAction>(true);
    case InputKey::Accept:
    case InputKey::Space:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Activate);
    case InputKey::Cancel:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Cancel);
    default:
        return nullptr;
    }
}

std::unique_ptr<MenuInputAction> HorizontalMenuInputMode::keyAction(
        InputKey key) const {
    switch (key) {
    case InputKey::Left:
        return std::make_unique<MoveMenuSelectionAction>(-1, true);
    case InputKey::Right:
        return std::make_unique<MoveMenuSelectionAction>(1, true);
    case InputKey::Up:
        return std::make_unique<SelectMenuBoundaryAction>(false);
    case InputKey::Down:
        return std::make_unique<SelectMenuBoundaryAction>(true);
    case InputKey::Accept:
    case InputKey::Space:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Activate);
    case InputKey::Cancel:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Cancel);
    default:
        return nullptr;
    }
}

std::unique_ptr<MenuInputAction> YesNoMenuInputMode::keyAction(
        InputKey key) const {
    switch (key) {
    case InputKey::Up:
    case InputKey::Left:
        return std::make_unique<SelectMenuBoundaryAction>(false);
    case InputKey::Down:
    case InputKey::Right:
        return std::make_unique<SelectMenuBoundaryAction>(true);
    case InputKey::Accept:
    case InputKey::Space:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Activate);
    case InputKey::Cancel:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Cancel);
    default:
        return nullptr;
    }
}

std::unique_ptr<MenuInputAction> VerticalOptionMenuInputMode::keyAction(
        InputKey key) const {
    switch (key) {
    case InputKey::Left:
        return std::make_unique<SubmitMenuGestureAction>(
            MenuGesture::AdjustPrevious);
    case InputKey::Right:
        return std::make_unique<SubmitMenuGestureAction>(
            MenuGesture::AdjustNext);
    case InputKey::Accept:
    case InputKey::Space:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Activate);
    case InputKey::Cancel:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Cancel);
    case InputKey::Up:
    case InputKey::Down:
        return std::make_unique<MoveMenuSelectionAction>(
            key == InputKey::Up ? -1 : 1, true);
    default:
        return nullptr;
    }
}

std::unique_ptr<MenuInputAction> HorizontalOptionMenuInputMode::keyAction(
        InputKey key) const {
    switch (key) {
    case InputKey::Left:
        return std::make_unique<SubmitMenuGestureAction>(
            MenuGesture::AdjustPrevious);
    case InputKey::Right:
        return std::make_unique<SubmitMenuGestureAction>(
            MenuGesture::AdjustNext);
    case InputKey::Accept:
    case InputKey::Space:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Activate);
    case InputKey::Cancel:
        return std::make_unique<SubmitMenuGestureAction>(MenuGesture::Cancel);
    case InputKey::Up:
    case InputKey::Down:
        return std::make_unique<MoveMenuSelectionAction>(
            key == InputKey::Up ? -1 : 1, true);
    default:
        return nullptr;
    }
}

}
