#include "warfield.hh"

#include "content/constants.hh"
#include "core/config.hh"
#include "item_snapshot_builder.hh"
#include "status_snapshot_builder.hh"
#include "world/action.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <algorithm>
#include <utility>
#include <vector>

namespace hojy::scene {

namespace {

const WarfieldInputMode &passiveMode() {
    static const PassiveWarfieldInputMode mode;
    return mode;
}

const WarfieldInputMode &moveSelectingMode() {
    static const MoveSelectingInputMode mode;
    return mode;
}

const WarfieldInputMode &attackSelectingMode() {
    static const AttackSelectingInputMode mode;
    return mode;
}

}

void Warfield::requestPlayerMenu() {
    if (!currentActor_) {
        setStage(Idle);
        return;
    }
    setStage(PlayerMenu);
    const auto sessionToken = presentationSessionToken();
    const auto actionGeneration = presentationGeneration_;
    const auto actorId = currentActor_->id;
    BattleMenuRequest request{sessionToken};
    request.actorId = actorId;
    request.actionGeneration = actionGeneration;
    request.expectedStage = BattlePresentationStage::PlayerMenu;
    request.initialIndex = lastMenuIndex_;
    request.noSkillMessage = GETTEXT(115);
    const auto &info = currentActor_->info;
    if (currentActor_->steps > 0 && info.stamina > 5) {
        request.entries.push_back({0, GETTEXT(82)});
    }
    if (info.stamina > 10) {
        request.entries.push_back({1, GETTEXT(83)});
        if (info.poison >= 20) {
            request.entries.push_back({2, GETTEXT(84)});
        }
    }
    if (info.stamina > 50) {
        if (info.depoison >= 20) {
            request.entries.push_back({3, GETTEXT(85)});
        }
        if (info.medic >= 20) {
            request.entries.push_back({4, GETTEXT(86)});
        }
    }
    request.entries.push_back({5, GETTEXT(87)});
    if (charQueue_.size() > 1) {
        request.entries.push_back({6, GETTEXT(88)});
    }
    request.entries.push_back({7, GETTEXT(89)});
    request.entries.push_back({8, GETTEXT(90)});
    request.entries.push_back({9, GETTEXT(91)});
    for (int index = 0; index < ::hojy::content::LearnSkillCount; ++index) {
        const auto skillId = info.skillId[index];
        if (skillId <= 0
            || static_cast<std::size_t>(skillId)
                >= ::hojy::world::state::gSaveData.skillInfo.size()) {
            continue;
        }
        const auto *skill = ::hojy::world::state::gSaveData.skillInfo[skillId];
        if (!skill) { continue; }
        const auto level = ::hojy::world::state::calcRealSkillLevel(
            skill->reqMp,
            std::clamp<std::int16_t>(info.skillLevel[index] / 100, 0, 9),
            info.mp);
        if (level >= 0) {
            request.skills.push_back({index, GETSKILLNAME(skillId)});
        }
    }
    postCommand([request = std::move(request)](
                        SceneCommandContext &context) mutable {
        context.showBattleMenu(std::move(request));
    });
}

void Warfield::applyPlayerMenuSelection(
        std::int16_t actorId, int menuIndex, int action) {
    if (menuIndex >= 0) { lastMenuIndex_ = menuIndex; }
    applyPlayerMenuAction(actorId, action);
}

void Warfield::applyPlayerMenuAction(std::int16_t actorId, int action) {
    auto *ch = currentActor_;
    if (!ch || ch->id != actorId) { return; }
    switch (action) {
    case 0:
        maskSelectableArea(ch->steps, 0);
        setStage(MoveSelecting);
        markWorldChanged();
        break;
    case 2:
        (void)tryUseSkill(-3);
        break;
    case 3:
        (void)tryUseSkill(-2);
        break;
    case 4:
        (void)tryUseSkill(-1);
        break;
    case 5: {
        setStage(PoppingUp);
        setPresentationStage(BattlePresentationStage::ItemSelection);
        BattleItemSelectionRequest request{
            presentationSessionToken(), actorId};
        request.actionGeneration = presentationGeneration_;
        request.expectedStage = BattlePresentationStage::ItemSelection;
        request.items = buildBattleItemViewSnapshot(
            battleInventorySnapshot());
        postCommand([request = std::move(request)](
                            SceneCommandContext &context) mutable {
            context.showBattleItemSelection(std::move(request));
        });
        break;
    }
    case 6: {
        const auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
        if (ite != charQueue_.end()) {
            charQueue_.erase(ite);
        }
        charQueue_.insert(charQueue_.begin(), ch);
        currentActor_ = nullptr;
        pendingAutoAction_ = nullptr;
        setStage(Idle);
        break;
    }
    case 7: {
        BattleStatusSelectionRequest request{presentationSessionToken()};
        request.actorId = actorId;
        request.actionGeneration = presentationGeneration_;
        request.expectedStage = BattlePresentationStage::StatusSelection;
        request.characters.title = {GETTEXT(59)};
        request.characters.columnTitle = GETTEXT(24);
        request.characters.rows.reserve(chars_.size());
        request.statuses.reserve(chars_.size());
        for (const auto &candidate: chars_) {
            if (candidate.side == 0 && candidate.id >= 0) {
                request.characters.rows.push_back({
                    candidate.id, false, GETCHARNAME(candidate.id),
                    std::to_wstring(candidate.info.level)});
                auto status = buildCharacterStatusSnapshot(
                    candidate.info, ::hojy::world::state::gSaveData.itemInfo,
                    false, core::config.showPotential());
                if (status) { request.statuses.push_back(std::move(*status)); }
            }
        }
        if (request.characters.rows.empty()
            || request.statuses.size() != request.characters.rows.size()) {
            return;
        }
        setStage(PoppingUp);
        setPresentationStage(BattlePresentationStage::StatusSelection);
        request.actionGeneration = presentationGeneration_;
        postCommand([request = std::move(request)](
                            SceneCommandContext &context) mutable {
            context.showBattleStatusSelection(std::move(request));
        });
        break;
    }
    case 8:
        doRest(ch);
        break;
    case 9:
        autoControl_ = true;
        setStage(Idle);
        break;
    default:
        break;
    }
}

void Warfield::applyPlayerSkillSelection(std::int16_t actorId, int skillIndex) {
    auto *ch = currentActor_;
    if (!ch || ch->id != actorId || skillIndex < 0
        || skillIndex >= ::hojy::content::LearnSkillCount) {
        return;
    }
    if (!tryUseSkill(skillIndex)) {
        const auto sessionToken = presentationSessionToken();
        const auto actionGeneration = presentationGeneration_;
        postPresentationCommand(
            presentationOwnerHandle(), sessionToken, actionGeneration,
            BattlePresentationStage::PlayerMenu, actorId,
            [](Warfield &, SceneCommandContext &context) {
                context.showItemMessage(
                    {{GETTEXT(115)},
                     static_cast<std::uint8_t>(
                         ScenePopupType::PressToCloseThis),
                     0, {}});
            });
    }
}

void Warfield::refreshStatusSnapshot() {
    std::optional<CharacterStatusSnapshot> candidate;
    if (currentActor_) {
        candidate = buildCharacterStatusSnapshot(
            currentActor_->info, ::hojy::world::state::gSaveData.itemInfo,
            false, true);
    }
    statusSnapshot_ = std::move(candidate);
    ++statusSnapshotRevision_;
    if (statusSnapshotRevision_ == 0) { statusSnapshotRevision_ = 1; }
}

void Warfield::setStage(Stage stage) {
    ++presentationGeneration_;
    if (presentationGeneration_ == 0) { presentationGeneration_ = 1; }
    stage_ = stage;
    presentationStage_ = stage == PlayerMenu
        ? BattlePresentationStage::PlayerMenu
        : stage == Finished
            ? BattlePresentationStage::FinishMessages
            : BattlePresentationStage::Any;
    if (stage == Idle || stage == PlayerMenu || stage == Moving) {
        refreshStatusSnapshot();
    } else {
        statusSnapshot_.reset();
        ++statusSnapshotRevision_;
        if (statusSnapshotRevision_ == 0) { statusSnapshotRevision_ = 1; }
    }
    switch (stage) {
    case MoveSelecting:
        inputMode_ = &moveSelectingMode();
        break;
    case AttackSelecting:
        inputMode_ = &attackSelectingMode();
        break;
    default:
        inputMode_ = &passiveMode();
        break;
    }
}

void Warfield::update() {
}

void Warfield::applyInputLogic() {
    if (hasPendingModeKey_) {
        if (!inputMode_) { setStage(stage_); }
        inputMode_->consume(*this, pendingModeKey_);
        pendingModeKey_ = InputKey::None;
        hasPendingModeKey_ = false;
    }
    auto action = std::move(pendingInputAction_);
    if (action) {
        action->execute(*this);
    }
}

void Warfield::queueMoveCursor(InputKey key) {
    pendingInputAction_ = std::make_unique<MoveCursorAction>(key);
}

void Warfield::queueConfirmMove() {
    pendingInputAction_ = std::make_unique<ConfirmMoveAction>();
}

void Warfield::queueConfirmAttack() {
    pendingInputAction_ = std::make_unique<ConfirmAttackAction>();
}

void Warfield::queueCancelSelection() {
    pendingInputAction_ = std::make_unique<CancelSelectionAction>();
}

void Warfield::queueCancelAutoControl() {
    pendingInputAction_ = std::make_unique<CancelAutoControlAction>();
}

void Warfield::executeMoveCursor(InputKey key) {
    int x = cursorX_;
    int y = cursorY_;
    switch (key) {
    case KeyUp:
        y = cursorY_ - 1;
        if (y < 0 || !cellInfo_[y * mapWidth_ + cursorX_].insideMovingArea) { return; }
        break;
    case KeyDown:
        y = cursorY_ + 1;
        if (y >= mapHeight_ || !cellInfo_[y * mapWidth_ + cursorX_].insideMovingArea) { return; }
        break;
    case KeyLeft:
        x = cursorX_ - 1;
        if (x < 0 || !cellInfo_[cursorY_ * mapWidth_ + x].insideMovingArea) { return; }
        break;
    case KeyRight:
        x = cursorX_ + 1;
        if (x >= mapWidth_ || !cellInfo_[cursorY_ * mapWidth_ + x].insideMovingArea) { return; }
        break;
    default:
        return;
    }

    cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 1;
    cursorX_ = x;
    cursorY_ = y;
    cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 2;
    markWorldChanged();
}

void Warfield::executeConfirmMove() {
    const int x = cursorX_;
    const int y = cursorY_;
    if (x == cameraX_ && y == cameraY_
        || cellInfo_[y * mapWidth_ + x].charInfo) {
        setStage(Idle);
    } else {
        auto ite = selCells_.find(std::make_pair(x, y));
        if (ite == selCells_.end()) {
            setStage(Idle);
        } else {
            setStage(Moving);
            movingPath_.clear();
            auto *cell = &ite->second;
            while (cell) {
                movingPath_.emplace_back(std::make_pair(cell->x, cell->y));
                cell = cell->moveParent;
            }
        }
    }
    unmaskArea();
    markWorldChanged();
}

void Warfield::executeConfirmAttack() {
    startActAction();
    unmaskArea();
    markWorldChanged();
}

void Warfield::executeCancelSelection() {
    unmaskArea();
    clearActionState(false);
    markWorldChanged();
    requestPlayerMenu();
}

void Warfield::executeCancelAutoControl() {
    if (currentActor_ && currentActor_->side == 0) {
        pendingAutoAction_ = nullptr;
        resumeAutoAttack_ = false;
        movingPath_.clear();
        if (stage_ == Moving) {
            setStage(Idle);
        }
    }
    autoControl_ = false;
}

}
