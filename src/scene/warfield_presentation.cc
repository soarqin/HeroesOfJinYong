#include "warfield.hh"

#include "charlistmenu.hh"
#include "item_selection_controller.hh"
#include "itemview.hh"
#include "menu.hh"
#include "messagebox.hh"
#include "menu_action_adapter.hh"
#include "statusview.hh"
#include "core/config.hh"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace hojy::scene {
namespace {

class DirectionSelectionResultSink {
public:
    virtual ~DirectionSelectionResultSink() = default;
    virtual void submit(Map::Direction direction) = 0;
};

template<typename Function>
class DirectionSelectionResultSinkAdapter final:
    public DirectionSelectionResultSink {
public:
    explicit DirectionSelectionResultSinkAdapter(Function function):
        function_(std::move(function)) {}

    void submit(Map::Direction direction) override {
        function_(direction);
    }

private:
    Function function_;
};

template<typename Function>
std::unique_ptr<DirectionSelectionResultSink>
makeDirectionSelectionResultSink(Function function) {
    return std::make_unique<DirectionSelectionResultSinkAdapter<Function>>(
        std::move(function));
}

class DirectionSelectionMessageBox final : public MessageBox {
public:
    using MessageBox::MessageBox;

    void setDirectionResultSink(
            std::unique_ptr<DirectionSelectionResultSink> sink) {
        directionResultSink_ = std::move(sink);
    }

    void consumeKeyIntent(Key key) override {
        pendingInput_ = key;
    }

    void applyInputLogic() override {
        const auto key = pendingInput_;
        pendingInput_ = KeyNone;
        switch (key) {
        case KeyUp:
            choose(Map::DirUp);
            break;
        case KeyLeft:
            choose(Map::DirLeft);
            break;
        case KeyRight:
            choose(Map::DirRight);
            break;
        case KeyDown:
            choose(Map::DirDown);
            break;
        case KeyCancel: {
            requestPresentationCleanup();
            submitResult({false});
            break;
        }
        default:
            break;
        }
    }

private:
    void choose(Map::Direction direction) {
        auto sink = std::move(directionResultSink_);
        requestPresentationCleanup();
        if (sink) { sink->submit(direction); }
    }

    std::unique_ptr<DirectionSelectionResultSink> directionResultSink_;
    Key pendingInput_ = KeyNone;
};

}

void Warfield::postPresentationCommand(
        std::weak_ptr<PresentationOwnerState> ownerState,
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage, std::int16_t expectedActorId,
        std::function<void(Warfield &, SceneCommandContext &)> command) {
    if (!command) { return; }
    auto state = ownerState.lock();
    if (!state || !state->owner) { return; }
    state->owner->postCommand(
        [ownerState, sessionToken, actionGeneration, expectedStage,
         expectedActorId, command = std::move(command)](
                SceneCommandContext &context) mutable {
            auto state = ownerState.lock();
            if (!state || !state->owner
                || !state->owner->matchesPresentationContext(
                    sessionToken, actionGeneration, expectedStage,
                    expectedActorId)) {
                return;
            }
            command(*state->owner, context);
        });
}

void Warfield::postBattleCommand(
        std::weak_ptr<PresentationOwnerState> ownerState,
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        std::function<void(SceneCommandContext &)> command) {
    if (!command) { return; }
    auto state = ownerState.lock();
    if (!state || !state->owner) { return; }
    state->owner->postCommand(
        [ownerState, sessionToken, actionGeneration,
         command = std::move(command)](SceneCommandContext &context) mutable {
            auto state = ownerState.lock();
            if (!state || !state->owner
                || !state->owner->matchesPresentationContext(
                    sessionToken, actionGeneration,
                    BattlePresentationStage::Any)) {
                return;
            }
            command(context);
        });
}

void Warfield::presentDirectionSelection(BattleDirectionSelectionRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::DirectionSelection,
            request.actorId)) { return; }
    auto *messageBox = new DirectionSelectionMessageBox(
        this, 0, 0, rootWidth(), rootHeight());
    messageBox->popup({request.prompt});
    const auto ownerState = presentationOwnerHandle();
    const auto sessionToken = request.sessionToken;
    const auto actionGeneration = request.actionGeneration;
    const auto expectedStage = BattlePresentationStage::DirectionSelection;
    const auto actorId = request.actorId;
    messageBox->setResultSink(makeMessageBoxResultSink(
        [ownerState, sessionToken, actionGeneration,
         expectedStage, actorId](MessageBoxResult) {
        auto state = ownerState.lock();
        if (!state || !state->owner) { return; }
        state->owner->postPresentationCommand(
            ownerState, sessionToken, actionGeneration, expectedStage, actorId,
            [actorId](Warfield &owner, SceneCommandContext &) {
                owner.cancelDirectionSelection(actorId);
            });
        }));
    messageBox->setDirectionResultSink(makeDirectionSelectionResultSink(
        [ownerState, sessionToken, actionGeneration, expectedStage,
         actorId](Map::Direction direction) {
            auto state = ownerState.lock();
            if (!state || !state->owner) { return; }
            state->owner->postPresentationCommand(
                ownerState, sessionToken, actionGeneration, expectedStage, actorId,
                [actorId, direction](Warfield &owner, SceneCommandContext &) {
                    owner.applyDirectionSelection(actorId, direction);
                });
        }));
}

void Warfield::presentSkillLevelUp(BattleSkillLevelUpRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::SkillLevelUp,
            request.actorId)) { return; }
    if (request.message.empty()) { return; }
    auto *msgBox = new MessageBox(this, 0, height_ / 3, width_, 60);
    msgBox->popup({std::move(request.message)},
                   MessageBox::PressToCloseThis);
    const auto ownerState = presentationOwnerHandle();
    const auto sessionToken = request.sessionToken;
    const auto actionGeneration = request.actionGeneration;
    const auto expectedStage = BattlePresentationStage::SkillLevelUp;
    const auto actorId = request.actorId;
    msgBox->setResultSink(makeMessageBoxResultSink(
        [ownerState, sessionToken, actionGeneration,
         expectedStage, actorId](MessageBoxResult) {
        auto state = ownerState.lock();
        if (!state || !state->owner) { return; }
        state->owner->postPresentationCommand(
            ownerState, sessionToken, actionGeneration, expectedStage, actorId,
            [actorId](Warfield &owner, SceneCommandContext &) {
                owner.resumeAfterSkillLevelUp(actorId);
            });
        }));
}

void Warfield::presentItemResult(BattleItemResultRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::ItemResult,
            request.actorId)) { return; }
    if (pendingItemResultActorId_ != request.actorId
        || pendingItemResultItemId_ != request.itemId) {
        return;
    }
    if (request.messages.empty()) { return; }
    auto *messageBox = new MessageBox(this, 0, 0, rootWidth(), rootHeight());
    messageBox->popup(std::move(request.messages), MessageBox::PressToCloseThis);
    const auto ownerState = presentationOwnerHandle();
    const auto sessionToken = request.sessionToken;
    const auto actionGeneration = request.actionGeneration;
    const auto expectedStage = BattlePresentationStage::ItemResult;
    const auto actorId = request.actorId;
    const auto itemId = request.itemId;
    messageBox->setResultSink(makeMessageBoxResultSink(
        [ownerState, sessionToken, actionGeneration,
         expectedStage, actorId, itemId](MessageBoxResult) {
        auto state = ownerState.lock();
        if (!state || !state->owner) { return; }
        state->owner->postPresentationCommand(
            ownerState, sessionToken, actionGeneration, expectedStage, actorId,
            [actorId, itemId](Warfield &owner, SceneCommandContext &) {
                owner.finishBattleItemResult(actorId, itemId);
            });
        }));
}

void Warfield::presentPlayerMenu(BattleMenuRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::PlayerMenu,
            request.actorId)
        || request.entries.empty()) { return; }
    const auto windowBorder = core::config.windowBorder();
    auto *menu = new MenuTextList(this, windowBorder * 4, windowBorder * 4,
                                  width_ - windowBorder * 8,
                                  height_ - windowBorder * 8);
    const auto actorId = request.actorId;
    const auto sessionToken = request.sessionToken;
    const auto actionGeneration = request.actionGeneration;
    const auto ownerState = presentationOwnerHandle();
    auto entries = std::make_shared<const std::vector<BattleMenuEntrySnapshot>>(
        std::move(request.entries));
    auto skills = std::make_shared<const std::vector<BattleSkillEntrySnapshot>>(
        std::move(request.skills));
    auto noSkillMessage = std::move(request.noSkillMessage);
    MenuEntries menuEntries;
    menuEntries.reserve(entries->size());
    for (const auto &entry: *entries) {
        menuEntries.push_back({entry.actionId, entry.label, L"", true});
    }
    menu->popup(menuEntries, request.initialIndex);
    auto controller = std::make_shared<ActionMenuController>();
    for (const auto &entry: *entries) {
        if (entry.actionId == 1) {
            controller->bind(entry.actionId, makeMenuAction(
                [ownerState, menu, skills,
                 noSkillMessage = noSkillMessage, actorId,
                 sessionToken, actionGeneration](MenuSelection) {
                auto state = ownerState.lock();
                if (!state || !state->owner
                    || !state->owner->matchesPresentationContext(
                        sessionToken, actionGeneration,
                        BattlePresentationStage::PlayerMenu, actorId)) {
                    return;
                }
                auto &owner = *state->owner;
                if (skills->empty()) {
                    menu->requestPresentationCleanup();
                    owner.postPresentationCommand(
                        ownerState, sessionToken, actionGeneration,
                        BattlePresentationStage::PlayerMenu, actorId,
                        [message = noSkillMessage](
                                Warfield &, SceneCommandContext &context) {
                            context.showItemMessage(
                                {{message}, static_cast<std::uint8_t>(
                                    ScenePopupType::PressToCloseThis), 0, {}});
                        });
                    return;
                }
                const auto border = core::config.windowBorder();
                auto *submenu = new MenuTextList(
                    menu, menu->x() + menu->width() + border,
                    border * 4,
                    owner.width_ - menu->x() + menu->width() - border,
                    owner.height_ - border * 8);
                MenuEntries skillEntries;
                skillEntries.reserve(skills->size());
                for (const auto &skill: *skills) {
                    skillEntries.push_back({skill.skillIndex, skill.label, L"", true});
                }
                submenu->popup(skillEntries);
                auto skillController = std::make_shared<ActionMenuController>();
                for (const auto &skill: *skills) {
                    skillController->bind(skill.skillIndex, makeMenuAction(
                        [ownerState, menu, submenu, actorId,
                         sessionToken, actionGeneration,
                         skillIndex = skill.skillIndex](MenuSelection) {
                            auto state = ownerState.lock();
                            if (!state || !state->owner
                                || !state->owner->matchesPresentationContext(
                                    sessionToken, actionGeneration,
                                    BattlePresentationStage::PlayerMenu, actorId)) {
                                return;
                            }
                            menu->requestPresentationCleanup();
                            submenu->requestPresentationCleanup();
                            state->owner->postPresentationCommand(
                                ownerState, sessionToken, actionGeneration,
                                BattlePresentationStage::PlayerMenu, actorId,
                                [actorId, skillIndex](
                                    Warfield &owner, SceneCommandContext &) {
                                    owner.applyPlayerSkillSelection(actorId, skillIndex);
                                });
                        }));
                }
                skillController->bindCancel(makeMenuAction(
                    [submenu](MenuSelection) { submenu->requestPresentationCleanup(); }));
                submenu->setSelectionSink(std::move(skillController));
            }));
        } else {
            const auto actionId = entry.actionId;
            controller->bind(actionId, makeMenuAction(
                [ownerState, menu, actorId, sessionToken,
                 actionGeneration, actionId](MenuSelection) {
                    auto state = ownerState.lock();
                    if (!state || !state->owner
                        || !state->owner->matchesPresentationContext(
                            sessionToken, actionGeneration,
                            BattlePresentationStage::PlayerMenu, actorId)) {
                        return;
                    }
                    menu->requestPresentationCleanup();
                    state->owner->postPresentationCommand(
                        ownerState, sessionToken, actionGeneration,
                        BattlePresentationStage::PlayerMenu, actorId,
                        [actorId, actionId](
                            Warfield &owner, SceneCommandContext &) {
                            owner.applyPlayerMenuAction(actorId, actionId);
                        });
                }));
        }
    }
    controller->bindCancel(makeMenuAction(
        [menu](MenuSelection) { menu->requestPresentationCleanup(); }));
    menu->setSelectionSink(std::move(controller));
}

void Warfield::presentItemSelection(BattleItemSelectionRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::ItemSelection,
            request.actorId)
        || !renderer_) { return; }
    const auto windowBorder = core::config.windowBorder();
    auto *view = new ItemView(this, windowBorder * 4, windowBorder * 4,
                              rootWidth() - windowBorder * 4,
                              rootHeight() - windowBorder * 4);
    const auto actorId = request.actorId;
    const auto sessionToken = request.sessionToken;
    const auto actionGeneration = request.actionGeneration;
    const auto ownerState = presentationOwnerHandle();
    auto controller = std::make_unique<FunctionItemSelectionController>(
        [ownerState, sessionToken, actionGeneration, actorId](
                ItemSelectionHost &host, std::int16_t itemId) {
            host.closeItemSelection();
            auto state = ownerState.lock();
            if (!state || !state->owner) { return; }
            state->owner->postPresentationCommand(
                ownerState, sessionToken, actionGeneration,
                BattlePresentationStage::ItemSelection, actorId,
                [actorId, itemId](Warfield &owner, SceneCommandContext &) {
                    owner.selectBattleItem(actorId, itemId);
                });
        },
        [ownerState, sessionToken, actionGeneration, actorId](
                ItemSelectionHost &host) {
            host.closeItemSelection();
            auto state = ownerState.lock();
            if (!state || !state->owner) { return; }
            state->owner->postPresentationCommand(
                ownerState, sessionToken, actionGeneration,
                BattlePresentationStage::ItemSelection, actorId,
                [](Warfield &owner, SceneCommandContext &) {
                    owner.requestPlayerMenu();
                });
        });
    view->show(std::move(request.items), std::move(controller));
}

void Warfield::presentStatusSelection(BattleStatusSelectionRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::StatusSelection,
            request.actorId)
        || request.characters.rows.empty()
        || request.characters.rows.size() != request.statuses.size()) {
        return;
    }
    struct StatusEntry final {
        std::int16_t characterId = -1;
        CharacterStatusSnapshot snapshot;
    };
    std::vector<StatusEntry> statusEntries;
    statusEntries.reserve(request.statuses.size());
    for (std::size_t index = 0; index < request.statuses.size(); ++index) {
        const auto characterId = request.characters.rows[index].characterId;
        if (characterId < 0) { return; }
        statusEntries.push_back(
            {characterId, std::move(request.statuses[index])});
    }
    auto *menu = new CharListMenu(this, 0, 0, rootWidth(), rootHeight());
    const auto ownerState = presentationOwnerHandle();
    const auto sessionToken = request.sessionToken;
    const auto actionGeneration = request.actionGeneration;
    const auto actorId = request.actorId;
    auto entries = std::make_shared<const std::vector<StatusEntry>>(
        std::move(statusEntries));
    auto controller = std::make_shared<ActionMenuController>();
    for (const auto &entry: *entries) {
        controller->bind(entry.characterId, makeMenuAction(
            [ownerState, sessionToken, actionGeneration, actorId,
             snapshot = entry.snapshot](MenuSelection) mutable {
                auto state = ownerState.lock();
                if (!state || !state->owner) { return; }
                state->owner->postPresentationCommand(
                    ownerState, sessionToken, actionGeneration,
                    BattlePresentationStage::StatusSelection, actorId,
                    [snapshot = std::move(snapshot)](
                            Warfield &owner, SceneCommandContext &) mutable {
                        auto *status = new StatusView(&owner, 0, 0, 0, 0);
                        status->setHeadTextureProvider(owner.headTextureProvider_);
                        status->show(std::move(snapshot));
                        status->makeCenter(
                            owner.width_, owner.height_, owner.x_, owner.y_);
                    });
            }));
    }
    controller->bindCancel(makeMenuAction(
        [menu](MenuSelection) { menu->requestPresentationCleanup(); }));
    menu->init(std::move(request.characters), std::move(controller));
    menu->makeCenter(rootWidth(), rootHeight() * 4 / 5, x_, y_);
}

void Warfield::presentFinishMessages(BattleFinishMessagesRequest request) {
    if (!matchesPresentationContext(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::FinishMessages)) { return; }
    if (presentationCleanupRequested_) {
        pendingFinishMessages_ = std::move(request);
        return;
    }
    auto ownerState = presentationOwnerHandle();
    struct FinishSequenceState final {
        decltype(ownerState) owner;
        std::vector<std::pair<int, std::wstring>> messages;
        std::size_t index = 0;
        std::uint64_t sessionToken = 0;
        std::uint64_t actionGeneration = 0;
        bool won = false;
        bool instantDie = false;
        std::function<void()> showNext;
    };
    auto sequence = std::make_shared<FinishSequenceState>();
    sequence->owner = ownerState;
    sequence->messages = std::move(request.messages);
    sequence->sessionToken = request.sessionToken;
    sequence->actionGeneration = request.actionGeneration;
    sequence->won = request.won;
    sequence->instantDie = request.instantDie;
    const std::weak_ptr<FinishSequenceState> weakSequence = sequence;
    sequence->showNext = [weakSequence]() {
        auto sequence = weakSequence.lock();
        if (!sequence) { return; }
        auto ownerState = sequence->owner.lock();
        if (!ownerState || !ownerState->owner) { return; }
        auto *owner = ownerState->owner;
        if (!owner->matchesPresentationContext(
                sequence->sessionToken, sequence->actionGeneration,
                BattlePresentationStage::FinishMessages)) {
            return;
        }
        if (sequence->index >= sequence->messages.size()) {
            owner->postPresentationCommand(
                sequence->owner, sequence->sessionToken,
                sequence->actionGeneration,
                BattlePresentationStage::FinishMessages, -1,
                [sessionToken = sequence->sessionToken,
                 actionGeneration = sequence->actionGeneration,
                 won = sequence->won, instantDie = sequence->instantDie](
                        Warfield &, SceneCommandContext &context) {
                    BattleEndRequest request{
                        sessionToken, won, instantDie, actionGeneration,
                        BattlePresentationStage::FinishMessages};
                    context.endWar(std::move(request));
                });
            return;
        }
        const int y = owner->height_ / 3;
        auto *messageBox = new MessageBox(owner, 0, y, owner->width_, 60);
        messageBox->popup({sequence->messages[sequence->index].second},
                          MessageBox::PressToCloseThis);
        ++sequence->index;
        auto *lastMessageBox = messageBox;
        while (sequence->index < sequence->messages.size()
               && sequence->messages[sequence->index].first > 0) {
            auto *child = new MessageBox(
                messageBox, 0,
                y + 60 * sequence->messages[sequence->index].first,
                owner->width_, 60);
            child->popup({sequence->messages[sequence->index].second},
                         MessageBox::PressToCloseParent);
            lastMessageBox = child;
            ++sequence->index;
        }
        lastMessageBox->setResultSink(makeMessageBoxResultSink(
            [sequence](MessageBoxResult) {
            if (sequence->showNext) { sequence->showNext(); }
            }));
    };
    sequence->showNext();
}

}
