#pragma once

#include "presentation.hh"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hojy::world::state {
class NewGameCandidate;
}

namespace hojy::scene {

/**
 * Services available only at the fixed-update command barrier.
 *
 * Logic commands depend on this port rather than on platform or presentation
 * types. The host supplies the concrete implementation at the barrier. The
 * default implementations keep the queue usable in isolated tests.
 */
enum class ScenePopupType : std::uint8_t {
    Normal,
    YesNo,
    PressToCloseTop,
    PressToCloseThis,
    PressToCloseParent,
};

enum class OptionCommandId : std::uint8_t {
    MiniPanel,
    Minimap,
    MusicVolume,
    SoundVolume,
    Save,
};

enum class OptionAdjustment : std::int8_t {
    None = 0,
    Previous = -1,
    Next = 1,
};

struct OptionsCommitRequest final {
    OptionCommandId id = OptionCommandId::Save;
    OptionAdjustment adjustment = OptionAdjustment::None;
};

struct OptionsCommitResult final {
    bool applied = false;
    int value = 0;
};

// Completion values for transitions whose visual fade has finished.  These
// are value-only command payloads; they carry no Window/Node pointers.
struct SubMapTransitionCompletion final {
    std::uint64_t transitionToken = 0;
    std::int16_t subMapId = -1;
    std::int16_t x = 0;
    std::int16_t y = 0;
    int direction = 0;
    int music = -1;
    bool switching = false;
};

struct BattleTransitionCompletion final {
    std::uint64_t transitionToken = 0;
    bool won = false;
};

class SceneCommandContext {
public:
    virtual ~SceneCommandContext() = default;

    virtual void title() {}
    virtual void endscreen() {}
    virtual bool startNewGame(::hojy::world::state::NewGameCandidate &&) { return false; }
    virtual bool loadGame(int) { return false; }
    virtual bool saveGame(int) { return false; }
    virtual void forceQuit() {}
    virtual void exitToGlobalMap(int) {}
    virtual void enterSubMap(std::int16_t, int) {}
    virtual bool enterWar(std::int16_t, bool, bool = false) { return false; }
    virtual void endWar(bool, bool = false) {}
    virtual void playerDie() {}
    virtual void useQuestItem(std::int16_t) {}
    virtual void forceEvent(std::int16_t) {}
    virtual void closePopup() {}
    virtual void endPopup(bool = false, bool = true) {}
    virtual void showMainMenu(bool) {}
    virtual void runTalk(const std::wstring &, std::int16_t, std::int16_t) {}
    virtual bool runShop(std::int16_t) { return false; }
    virtual void showMessage(std::vector<std::wstring>, ScenePopupType) {}
    virtual void showEventMenu(EventMenuRequest) {}
    virtual void showEventOverlay(EventOverlayRequest) {}
    virtual void clearEventPresentation(EventPresentationClearRequest) {}
    virtual void fadeEventIn(EventFadeRequest) {}
    virtual void fadeEventOut(EventFadeRequest) {}
    virtual void showCharacterSelection(CharacterSelectionRequest) {}
    virtual void showItemMessage(ItemMessageRequest) {}
    virtual void showBattleDirectionSelection(BattleDirectionSelectionRequest) {}
    virtual void showBattleSkillLevelUp(BattleSkillLevelUpRequest) {}
    virtual void showBattleItemResult(BattleItemResultRequest) {}
    virtual void showBattleMenu(BattleMenuRequest) {}
    virtual void showBattleItemSelection(BattleItemSelectionRequest) {}
    virtual void showBattleStatusSelection(BattleStatusSelectionRequest) {}
    virtual void showBattleFinishMessages(BattleFinishMessagesRequest) {}
    virtual void endWar(BattleEndRequest) {}
    virtual void abortBattle(BattleAbortRequest) {}
    virtual void playMusic(int) {}
    virtual void playAtkSound(int) {}
    virtual void playEffectSound(int) {}
    virtual void setGlobalMapPosition(int, int) {}
    virtual void beginTextInput() {}
    virtual void setTextInputRect(int, int, int, int) {}
    virtual void endTextInput() {}
    virtual OptionsCommitResult commitOptions(OptionsCommitRequest) {
        return {};
    }
    virtual void continueEvent(bool) {}
    virtual void completeSubMapTransition(SubMapTransitionCompletion) {}
    virtual void completeBattleTransition(BattleTransitionCompletion) {}
};

class SceneCommand {
public:
    virtual ~SceneCommand() = default;
    virtual void execute(SceneCommandContext &context) = 0;
};

class FunctionSceneCommand final : public SceneCommand {
public:
    explicit FunctionSceneCommand(std::function<void(SceneCommandContext &)> function)
        : function_(std::move(function)) {}

    void execute(SceneCommandContext &context) override {
        if (function_) {
            function_(context);
        }
    }

private:
    std::function<void(SceneCommandContext &)> function_;
};

class SceneCommandQueue final {
public:
    void push(std::unique_ptr<SceneCommand> command);
    void push(std::function<void(SceneCommandContext &)> function);

    void executeGeneration(SceneCommandContext &context);

    /** Discard commands appended after a fixed-logic transaction checkpoint. */
    void discardAfter(std::size_t checkpoint) noexcept;

    [[nodiscard]] bool empty() const noexcept { return commands_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return commands_.size(); }

private:
    std::deque<std::unique_ptr<SceneCommand>> commands_;
};

}
