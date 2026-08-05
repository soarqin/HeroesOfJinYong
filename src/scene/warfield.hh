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

#include "battle/ai.hh"
#include "battle/ai_policy.hh"
#include "battle/engine.hh"
#include "battle/game_random.hh"
#include "battle/movement.hh"
#include "map.hh"
#include "logic/battle_effect_snapshot.hh"
#include "logic/warfield_input_mode.hh"
#include "status_snapshot.hh"
#include "world/bag.hh"
#include "world/character.hh"
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace hojy::scene {

class Warfield: public Map,
                private WarfieldInputContext,
                private WarfieldInputExecutionContext {
    friend class Window;
    struct PresentationOwnerState final {
        Warfield *owner = nullptr;
    };
    enum {
        FightTextureListCount = 110,
    };
    enum Stage {
        Idle,
        PlayerMenu,
        MoveSelecting,
        AttackSelecting,
        Moving,
        Acting,
        PoppingUp,
        Finished,
    };
    struct CharInfo {
        std::uint8_t side; /* 0-self 1-enemy */
        std::int16_t id;
        std::int16_t texId;
        std::int16_t x, y;
        Direction direction;
        ::hojy::world::state::CharacterData info;
        std::uint16_t exp;
        std::int16_t steps;
        std::int16_t initialSteps;
        std::int16_t attack, defence;
        battle::AiStats aiEntryStats;
        battle::AiStats aiEquipmentBonusStats;
        /* 8/9 are persistent AI request markers (medic/depoison), not UI-only codes. */
        std::int16_t actionCode = 0;
        std::int16_t persistentEntryMaxMp = 0;
        std::int16_t battleEntryMaxMp = 0;
    };
    struct CellInfo {
        std::int16_t earthId = 0, buildingId = 0;
        bool blocked = false;
        CharInfo *charInfo = nullptr;
        std::uint8_t insideMovingArea = 0;
    };
    using SelectableCell = battle::SelectableCell;
    struct PopupNumber {
        std::wstring str;
        int x, y;
        std::uint8_t r, g, b;
    };
public:
    Warfield(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale);
    ~Warfield() override;

    [[nodiscard]] bool ready() const noexcept { return resourcesReady_; }
    [[nodiscard]] std::int16_t takePendingBattleMusic() noexcept {
        const auto music = pendingBattleMusic_;
        pendingBattleMusic_ = -1;
        return music;
    }

    void setHeadTextureProvider(std::function<const Texture *(std::int16_t)> provider) {
        headTextureProvider_ = std::move(provider);
    }

    void cleanup();
    void abortPresentationState() noexcept;
    bool load(std::int16_t warId);
    inline void setGetExpOnLose(bool b) { getExpOnLose_ = b; }
    inline void setDeadOnLose(bool b) { deadOnLose_ = b; }
    bool getDefaultChars(std::set<std::int16_t> &chars) const;
    bool putChars(const std::vector<std::int16_t> &chars);

    void prepareRender() override;
    void render() const override;
    void update() override;
    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;

protected:
    void frameUpdate() override;

    void nextAction();
    void autoAction();
    void autoActionSkill(CharInfo *ch, int actorIndex,
                         const battle::AiStats &actorAiStats,
                         const battle::AiResourceState &resourceState,
                         const std::vector<battle::AiAllyState> &allies,
                         const battle::AiPowerSummary &allyPower,
                         bool resumeAutoAttack, bool requestSupport,
                         bool supportWithoutPosition,
                         battle::AiResourceAction resourceAction,
                         battle::RandomSource &resourceRandom);
    void recalcKnowledge();
    void requestPlayerMenu();
    void applyPlayerMenuSelection(
        std::int16_t actorId, int menuIndex, int action);
    void applyPlayerMenuAction(std::int16_t actorId, int action);
    void applyPlayerSkillSelection(std::int16_t actorId, int skillIndex);
    void resumeAfterSkillLevelUp(std::int16_t actorId);
    void maskSelectableArea(int steps, int ranges, bool zoecheck = false);
    void unmaskArea();
    void getSelectableArea(CharInfo *ch, std::map<std::pair<int, int>, SelectableCell> &selCells, int steps, int ranges, bool zoecheck = false);
    bool tryUseSkill(int index);
    void applyDirectionSelection(std::int16_t actorId, Direction direction);
    void cancelDirectionSelection(std::int16_t actorId);
    void finishBattleItemResult(std::int16_t actorId, std::int16_t itemId);
    void selectBattleItem(std::int16_t actorId, std::int16_t itemId);
    void startActAction();
    void commitEffectOverlaySnapshot(
        const std::vector<logic::BattleEffectCell> &cells) noexcept;
    void makeDamage(CharInfo *ch, int x, int y, int distance);
    void doRest(CharInfo *expectedActor = nullptr);
    void endTurn(CharInfo *expectedActor = nullptr);
    bool checkWarEnd();
    void endWar();
    void clearActionState(bool clearPopupNumbers);
    void syncBattleParticipantsToWorking() noexcept;
    void syncBattleParticipantsFromWorking() noexcept;
    void discardBattleSession() noexcept;
    void commitBattleBag() noexcept;
    void queueBattleAbortTransition() noexcept;
    [[nodiscard]] std::optional<battle::ParticipantId> participantIndex(
        const CharInfo *character) const noexcept;
    [[nodiscard]] battle::InventorySnapshot battleInventorySnapshot() const;
    bool recordBattleAction(const battle::BattleAction &action);

    // Presentation adapter entry points.  They are invoked only by
    // SceneCommandContext implementations at the fixed-logic barrier.
    void presentDirectionSelection(BattleDirectionSelectionRequest request);
    void presentSkillLevelUp(BattleSkillLevelUpRequest request);
    void presentItemResult(BattleItemResultRequest request);
    void presentPlayerMenu(BattleMenuRequest request);
    void presentItemSelection(BattleItemSelectionRequest request);
    void presentStatusSelection(BattleStatusSelectionRequest request);
    void presentFinishMessages(BattleFinishMessagesRequest request);

    bool beginPresentationSession();
    void invalidatePresentationSession() noexcept;
    [[nodiscard]] std::uint64_t presentationSessionToken() const noexcept;
    [[nodiscard]] BattlePresentationSession::Handle presentationSessionHandle() const noexcept;
    [[nodiscard]] std::weak_ptr<PresentationOwnerState>
        presentationOwnerHandle() const noexcept;
    [[nodiscard]] bool isCurrentPresentationSession(std::uint64_t token) const noexcept;
    [[nodiscard]] bool matchesPresentationContext(
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage,
        std::int16_t expectedActorId = -1) const noexcept;
    void postPresentationCommand(
        std::weak_ptr<PresentationOwnerState> ownerState,
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage, std::int16_t expectedActorId,
        std::function<void(Warfield &, SceneCommandContext &)> command);
    void postBattleCommand(
        std::weak_ptr<PresentationOwnerState> ownerState,
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        std::function<void(SceneCommandContext &)> command);

    void setStage(Stage stage);
    void refreshStatusSnapshot();
    void setPresentationStage(BattlePresentationStage stage) noexcept {
        presentationStage_ = stage;
    }
    void queueMoveCursor(InputKey key) override;
    void queueConfirmMove() override;
    void queueConfirmAttack() override;
    void queueCancelSelection() override;
    void queueCancelAutoControl() override;
    void executeMoveCursor(InputKey key) override;
    void executeConfirmMove() override;
    void executeConfirmAttack() override;
    void executeCancelSelection() override;
    void executeCancelAutoControl() override;

private:
    std::int16_t warId_ = -1;
    bool getExpOnLose_ = false;
    bool deadOnLose_ = false;
    std::vector<CellInfo> cellInfo_;
    std::set<std::int16_t> warMapLoaded_;

    std::vector<CharInfo> chars_;
    std::vector<std::unique_ptr<battle::BattleParticipant>> battleParticipants_;
    battle::GameRandom battleGameRandom_;
    battle::RecordingRandom battleRandom_;
    battle::BattleEngine battleEngine_;
    ::hojy::world::state::Bag battleBag_;
    bool battleBagActive_ = false;
    std::vector<CharInfo*> turnOrder_;
    std::vector<CharInfo*> charQueue_;
    CharInfo *currentActor_ = nullptr;
    std::uint32_t round_ = 0;
    Stage stage_ = Idle;
    std::uint64_t presentationGeneration_ = 1;
    BattlePresentationStage presentationStage_ = BattlePresentationStage::Any;
    const WarfieldInputMode *inputMode_ = nullptr;
    int lastMenuIndex_ = 0;
    std::uint16_t knowledge_[2] = {0, 0};
    int cursorX_ = 0, cursorY_ = 0;
    bool autoControl_ = false;
    bool won_ = false;
    bool skillLevelup_ = false;
    std::map<std::pair<int, int>, SelectableCell> selCells_;
    std::vector<std::pair<int, int>> movingPath_;
    /* -3poison -2depoison -1medic 0~skillId */
    std::int16_t actIndex_ = -1, actId_ = -1, actLevel_ = 0;
    std::int16_t actItemSlot_ = -1;
    std::vector<battle::ActionTarget> actionTargets_;
    int effectId_ = -1, effectTexIdx_ = -1, fightTexIdx_ = -1, fightTexCount_ = 0, fightFrame_ = 0;
    logic::BattleEffectOverlaySnapshot effectOverlaySnapshot_;
    int attackTimesLeft_ = 0;
    const std::vector<std::string> *fightTex_ = nullptr;
    std::vector<PopupNumber> popupNumbers_;
    std::function<void()> pendingAutoAction_;
    std::function<void()> pendingSkillLevelUpContinuation_;
    std::int16_t pendingSkillLevelUpActorId_ = -1;
    std::int16_t pendingSkillLevelUpSkillId_ = -1;
    std::int16_t pendingSkillLevelUpSkillIndex_ = -1;
    std::int16_t pendingItemResultActorId_ = -1;
    std::int16_t pendingItemResultItemId_ = -1;
    std::unique_ptr<WarfieldInputAction> pendingInputAction_;
    InputKey pendingModeKey_ = InputKey::None;
    bool hasPendingModeKey_ = false;
    bool resumeAutoAttack_ = false;
    Node *statusPanel_ = nullptr;
    // Logic only requests release; the presentation preparation phase owns
    // the actual node destruction.
    bool statusPanelReleaseRequested_ = false;
    // Logic may invalidate battle presentation children, but the node tree
    // and fade resources are changed only by prepareRender().
    bool presentationCleanupRequested_ = false;
    // Loading a different serialized battlefield invalidates the presentation
    // texture cache.  The request is recorded by fixed logic and consumed by
    // prepareRender(), where GPU resources are owned.
    bool presentationTextureResetRequested_ = false;
    std::optional<BattleFinishMessagesRequest> pendingFinishMessages_;
    std::optional<CharacterStatusSnapshot> statusSnapshot_;
    std::uint64_t statusSnapshotRevision_ = 0;
    std::uint64_t renderedStatusSnapshotRevision_ = 0;
    Texture *drawingTerrainTex2_ = nullptr;
    bool resourcesReady_ = false;
    std::int16_t pendingBattleMusic_ = -1;
    std::vector<std::vector<std::string>> fightTexData_;
    std::function<const Texture *(std::int16_t)> headTextureProvider_;
    BattlePresentationSession presentationSession_;
    std::shared_ptr<PresentationOwnerState> presentationOwnerState_;
};

}
