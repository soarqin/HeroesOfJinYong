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

#include "renderer.hh"
#include "texture.hh"
#include "mapwithevent.hh"
#include "messagebox.hh"
#include "core/input_event.hh"
#include "logic/command.hh"
#include "logic/input.hh"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <vector>

namespace hojy::scene {

class Window;

struct WindowLifetimeState final {
    Window *owner = nullptr;
};

using WindowLifetimeHandle = std::weak_ptr<WindowLifetimeState>;

class Warfield;
class ExtendedNode;
class EventOverlaySurfaceAdapter;

class Window final: public SceneCommandContext {
public:
    Window(int w, int h);
    ~Window();

    [[nodiscard]] Renderer *renderer() {
        return renderer_;
    }

    [[nodiscard]] inline int width() const { return width_; }
    [[nodiscard]] inline int height() const { return height_; }
    [[nodiscard]] bool ready() const noexcept { return ready_; }

    [[nodiscard]] WindowLifetimeHandle windowLifetimeHandle() const noexcept {
        return WindowLifetimeHandle(lifetimeState_);
    }

    [[nodiscard]] std::uint64_t currTime() { return currTime_; }

    [[nodiscard]] inline const Texture *headTexture(std::int16_t id) const { return headTextureMgr_[id]; }
    [[nodiscard]] const Texture *smpTexture(std::int16_t id) const;
    void renderItemTexture(std::int16_t id, int x, int y, int w, int h);
    [[nodiscard]] int itemTexWidth() const { return itemTexW_; }
    [[nodiscard]] int itemTexHeight() const { return itemTexH_; }

    [[nodiscard]] MapWithEvent *globalMap() const { return globalMap_; }

    void dispatchInput(const core::InputEvent &event);
    void updateInput();
    void updateFixed();
    void compatibilityUpdate();
    void update();
    bool prepareRender();
    void render() const;
    bool flush();

    [[nodiscard]] bool quitRequested() const { return quitRequested_; }
    void requestQuit() { quitRequested_ = true; }
    void setSimulationTime(std::uint64_t timestamp);
    void applyDeferredCommands();

    void playMusic(int idx) override;
    void playAtkSound(int idx) override;
    void playEffectSound(int idx) override;

    void title() override;
    void endscreen() override;
    bool startNewGame(::hojy::world::state::NewGameCandidate &&) override;
    bool loadGame(int slot) override;
    bool saveGame(int slot) override;
    void forceQuit() override;
    void exitToGlobalMap(int direction) override;
    void enterSubMap(std::int16_t subMapId, int direction) override;
    bool enterWar(std::int16_t warId, bool getExpOnLose, bool deadOnLose = false) override;
    void endWar(bool won, bool instantDie = false);
    void endWar(BattleEndRequest request) override;
    void abortBattle(BattleAbortRequest request) override;
    void playerDie() override;
    void useQuestItem(std::int16_t itemId) override;
    void forceEvent(std::int16_t eventId) override;

    void closePopup() override;
    void endPopup(bool close = false, bool result = true) override;

    void showMainMenu(bool inSubMap) override;
    void runTalk(const std::wstring &text, std::int16_t headId, std::int16_t position) override;
    bool runShop(std::int16_t id) override;
    void showMessage(std::vector<std::wstring> text, ScenePopupType type) override;
    void showEventMenu(EventMenuRequest request) override;
    void showEventOverlay(EventOverlayRequest request) override;
    void clearEventPresentation(EventPresentationClearRequest request) override;
    void fadeEventIn(EventFadeRequest request) override;
    void fadeEventOut(EventFadeRequest request) override;
    void showCharacterSelection(CharacterSelectionRequest request) override;
    void showItemMessage(ItemMessageRequest request) override;
    void showBattleDirectionSelection(BattleDirectionSelectionRequest request) override;
    void showBattleSkillLevelUp(BattleSkillLevelUpRequest request) override;
    void showBattleItemResult(BattleItemResultRequest request) override;
    void showBattleMenu(BattleMenuRequest request) override;
    void showBattleItemSelection(BattleItemSelectionRequest request) override;
    void showBattleStatusSelection(BattleStatusSelectionRequest request) override;
    void showBattleFinishMessages(BattleFinishMessagesRequest request) override;
    void setGlobalMapPosition(int x, int y) override;
    void beginTextInput() override;
    void setTextInputRect(int x, int y, int w, int h) override;
    void endTextInput() override;
    OptionsCommitResult commitOptions(OptionsCommitRequest request) override;
    void continueEvent(bool result) override;
    void completeSubMapTransition(SubMapTransitionCompletion request) override;
    void completeBattleTransition(BattleTransitionCompletion request) override;
    void popupMessageBox(const std::vector<std::wstring> &text, MessageBox::Type type = MessageBox::Normal);

private:
    friend class EventOverlaySurfaceAdapter;
    int width_, height_;
    std::shared_ptr<WindowLifetimeState> lifetimeState_;
    void *win_ = nullptr;
    Renderer *renderer_ = nullptr;
    Map *map_ = nullptr;
    Node *popup_ = nullptr;
    Node *mainMenu_ = nullptr;
    bool freeOnClose_ = false;

    MapWithEvent *globalMap_ = nullptr;
    MapWithEvent *subMap_ = nullptr;
    Warfield *warfield_ = nullptr;
    Node *talkBox_ = nullptr;
    TextureMgr headTextureMgr_;
    Texture *itemTexture_ = nullptr;
    int itemTexW_ = 0, itemTexH_ = 0, itemWCount_ = 0, itemHCount_ = 0;

    std::uint64_t currTime_ = 0;
    bool ready_ = true;
    bool quitRequested_ = false;
    bool processingStage_ = false;
    bool applyingDeferred_ = false;
    std::uint64_t transitionGeneration_ = 0;
    std::uint64_t battleSessionToken_ = 0;
    Warfield *battleSessionOwner_ = nullptr;
    ExtendedNode *eventOverlay_ = nullptr;
    MapWithEvent *eventOverlayOwner_ = nullptr;
    std::uint64_t eventOverlaySession_ = 0;
    MapWithEvent *eventFadeOwner_ = nullptr;
    std::uint64_t eventFadeSession_ = 0;
    std::vector<ExtendedNode *> pendingEventOverlayCleanup_;
    std::vector<Node *> pendingEventFadeCleanup_;
    Node *deferredPopup_ = nullptr;
    bool deferredPopupOwned_ = false;
    bool ownsVideoSubsystem_ = false;
    bool ownsControllerSubsystem_ = false;
    std::deque<core::InputEvent> sampledInputEvents_;
    std::deque<core::InputEvent> pendingInputEvents_;
    QueuedInputPort inputPort_;
    SceneCommandQueue deferredCommands_;
    int playingMusic_ = -1;

    void applyDeferredNodes();
    void applyPresentationCleanup() noexcept;
    void preparePresentationCleanup() noexcept;
    void bindCommandSink(Node *node);
    [[nodiscard]] std::uint64_t beginTransition() noexcept;
    [[nodiscard]] bool isCurrentTransition(std::uint64_t token) const noexcept;
    void invalidateTransitions() noexcept;
    void replacePopup(Node *popup, bool owned = true);
    void detachEventOverlay(ExtendedNode *overlay, std::uint64_t session) noexcept;
    [[nodiscard]] bool initializeItemAtlas();
    void invalidateBattleSession() noexcept;
    [[nodiscard]] bool activateBattleSession(Warfield *owner) noexcept;
    [[nodiscard]] bool ownsBattleSession(
        const Warfield *owner, std::uint64_t token) const noexcept;
    [[nodiscard]] bool battlePresentationAvailable(
        std::uint64_t token, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage,
        std::int16_t expectedActorId = -1) const noexcept;
    [[nodiscard]] bool isCurrentBattleSession(
        const Warfield *owner, std::uint64_t token) const noexcept;
};

}
