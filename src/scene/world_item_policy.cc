#include "item_selection_controller.hh"

#include "character_list_snapshot_builder.hh"
#include "item_snapshot_builder.hh"
#include "logic/command.hh"
#include "world/item_transaction.hh"
#include "world/strings.hh"

#include <fmt/xchar.h>

#include <memory>
#include <type_traits>
#include <utility>

namespace hojy::scene {
namespace {

bool continuationAlive(const std::weak_ptr<ItemSelectionLifetime> &lifetime,
                       bool bound) {
    if (!bound) { return true; }
    const auto token = lifetime.lock();
    return token && token->alive;
}

template<typename Result, typename Function>
class LambdaContinuation final
    : public std::conditional_t<std::is_same_v<Result, CharacterSelectionResult>,
                                 CharacterSelectionContinuation,
                                 ItemMessageContinuation> {
public:
    explicit LambdaContinuation(Function function): function_(std::move(function)) {}

    void submit(Result result) override {
        if (function_) { function_(std::move(result)); }
    }

private:
    Function function_;
};

using CharacterContinuation = LambdaContinuation<
    CharacterSelectionResult,
    std::function<void(CharacterSelectionResult)>>;
using MessageContinuation = LambdaContinuation<
    ItemMessageResult,
    std::function<void(ItemMessageResult)>>;

ItemMessageRequest messageRequest(
        std::vector<std::wstring> text,
        ScenePopupType type,
        std::uint64_t token = 0,
        std::shared_ptr<ItemMessageContinuation> continuation = {}) {
    ItemMessageRequest request;
    request.text = std::move(text);
    request.popupType = static_cast<std::uint8_t>(type);
    request.continuationToken = token;
    request.continuation = std::move(continuation);
    return request;
}

std::vector<CharacterListSource> characterSources(
        const std::vector<std::int16_t> &ids) {
    std::vector<CharacterListSource> result;
    result.reserve(ids.size());
    for (const auto id: ids) {
        if (id >= 0) { result.push_back({id, false}); }
    }
    return result;
}

class EquipItemSelectionAction final: public ItemSelectionAction {
public:
    void execute(WorldItemSelectionController &controller,
                 ItemSelectionHost &host, std::int16_t itemId) override {
        controller.beginEquipSelection(host, itemId);
    }
};

class ConsumeItemSelectionAction final: public ItemSelectionAction {
public:
    void execute(WorldItemSelectionController &controller,
                 ItemSelectionHost &host, std::int16_t itemId) override {
        controller.beginConsumeSelection(host, itemId);
    }
};

class QuestItemSelectionAction final: public ItemSelectionAction {
public:
    void execute(WorldItemSelectionController &controller,
                 ItemSelectionHost &host, std::int16_t itemId) override {
        controller.beginQuestSelection(host, itemId);
    }
};

class IgnoreItemSelectionAction final: public ItemSelectionAction {
public:
    void execute(WorldItemSelectionController &controller,
                 ItemSelectionHost &host, std::int16_t) override {
        controller.ignoreSelection(host);
    }
};

}

WorldItemSelectionController::WorldItemSelectionController(
        std::optional<std::pair<int, int>> compassPosition,
        CloseFunction closeFunction,
        UseQuestItemFunction useQuestItemFunction):
    compassPosition_(compassPosition),
    closeFunction_(std::move(closeFunction)),
    useQuestItemFunction_(std::move(useQuestItemFunction)) {
}

void WorldItemSelectionController::cancel(ItemSelectionHost &host) {
    host.closeItemSelection();
}

void WorldItemSelectionController::bindItems(
        const std::vector<ItemViewEntrySnapshot> &items) {
    actions_.clear();
    for (const auto &entry: items) {
        const auto info = ::hojy::world::state::itemPolicyInfo(entry.itemId);
        if (!info) { continue; }
        switch (info->itemType) {
        case 1:
        case 2:
            actions_[entry.itemId] =
                std::make_unique<EquipItemSelectionAction>();
            break;
        case 3:
            actions_[entry.itemId] =
                std::make_unique<ConsumeItemSelectionAction>();
            break;
        case 4:
            actions_[entry.itemId] =
                std::make_unique<IgnoreItemSelectionAction>();
            break;
        default:
            actions_[entry.itemId] =
                std::make_unique<QuestItemSelectionAction>();
            break;
        }
    }
}

void WorldItemSelectionController::select(ItemSelectionHost &host,
                                           std::int16_t itemId) {
    if (!continuationAlive(lifetime_, lifetimeBound_)) { return; }
    const auto action = actions_.find(itemId);
    if (action != actions_.end() && action->second) {
        action->second->execute(*this, host, itemId);
    }
}

void WorldItemSelectionController::beginEquipSelection(
        ItemSelectionHost &host, std::int16_t itemId) {
    const auto itemInfo = ::hojy::world::state::itemPolicyInfo(itemId);
    if (!itemInfo) { return; }
        CharacterSelectionRequest request;
        const auto selection =
            ::hojy::world::state::itemSelectionSnapshot();
        request.characters = buildCharacterListSnapshot(
            {GETTEXT(itemInfo->itemType == 1 ? 38 : 39) + itemInfo->name},
            characterSources(selection.teamMemberIds), {levelProjection()});
        request.continuationToken = nextContinuationToken_++;
        request.continuation = std::make_shared<CharacterContinuation>(
            [this, hostPtr = &host, weakLifetime = lifetime_,
             lifetimeBound = lifetimeBound_, itemId,
             itemType = itemInfo->itemType, wasEquipped = itemInfo->user >= 0]
            (CharacterSelectionResult result) {
                if (!continuationAlive(weakLifetime, lifetimeBound)) { return; }
                if (!result.accepted || result.characterId < 0) {
                    hostPtr->closeItemSelection();
                    return;
                }
                if (itemType == 2 && wasEquipped) {
                    auto continuation = std::make_shared<MessageContinuation>(
                        [this, hostPtr, weakLifetime, lifetimeBound,
                         itemId, charId = result.characterId]
                        (ItemMessageResult confirmation) {
                            if (!continuationAlive(weakLifetime, lifetimeBound)) { return; }
                            if (confirmation.accepted) {
                                equipItem(*hostPtr, itemId, charId);
                            } else {
                                hostPtr->closeItemSelection();
                            }
                        });
                    hostPtr->showItemMessage(messageRequest(
                        {GETTEXT(44), GETTEXT(45)}, ScenePopupType::YesNo,
                        nextContinuationToken_++, std::move(continuation)));
                    return;
                }
                equipItem(*hostPtr, itemId, result.characterId);
            });
        host.showCharacterSelection(std::move(request));
}

void WorldItemSelectionController::beginConsumeSelection(
        ItemSelectionHost &host, std::int16_t itemId) {
    const auto itemInfo = ::hojy::world::state::itemPolicyInfo(itemId);
    if (!itemInfo) { return; }
        CharacterSelectionRequest request;
        const auto selection =
            ::hojy::world::state::itemSelectionSnapshot();
        request.characters = buildCharacterListSnapshot(
            {GETTEXT(36) + L' ' + itemInfo->name},
            characterSources(selection.teamMemberIds), {levelProjection()});
        request.continuationToken = nextContinuationToken_++;
        request.x = 0;
        request.y = 0;
        request.width = -1;
        request.height = -1;
        request.continuation = std::make_shared<CharacterContinuation>(
            [this, hostPtr = &host, weakLifetime = lifetime_,
             lifetimeBound = lifetimeBound_, itemId]
            (CharacterSelectionResult result) {
                if (!continuationAlive(weakLifetime, lifetimeBound)) { return; }
                if (!result.accepted || result.characterId < 0) {
                    hostPtr->closeItemSelection();
                    return;
                }
                consumeItem(*hostPtr, itemId, result.characterId);
            });
        host.showCharacterSelection(std::move(request));
}

void WorldItemSelectionController::beginQuestSelection(
        ItemSelectionHost &host, std::int16_t itemId) {
    if (useQuestItemFunction_) {
        useQuestItemFunction_(host, itemId);
    } else if (closeFunction_) {
        closeFunction_(host);
    } else {
        host.closeItemSelection();
    }
}

void WorldItemSelectionController::ignoreSelection(ItemSelectionHost &) {
}

void WorldItemSelectionController::equipItem(ItemSelectionHost &host,
                                              std::int16_t itemId,
                                              std::int16_t charId) {
    const auto itemInfo = ::hojy::world::state::itemPolicyInfo(itemId);
    if (!itemInfo) {
        host.closeItemSelection();
        return;
    }
    const auto validation = ::hojy::world::state::validateEquipItem(
        itemId, charId);
    if (validation == ::hojy::world::state::ItemEquipValidation::SkillSlotsFull) {
        host.showItemMessage(messageRequest(
            {GETTEXT(42)}, ScenePopupType::PressToCloseThis));
        return;
    }
    auto candidate = ::hojy::world::state::prepareEquipItem(itemId, charId);
    if (!candidate) {
        host.showItemMessage(messageRequest(
            {GETTEXT(itemInfo->itemType == 2 ? 43 : 46)},
            ScenePopupType::PressToCloseThis));
        return;
    }
    const auto nextItems = candidate->bagItems();
    if (!::hojy::world::state::commitItemAction(std::move(*candidate))) {
        host.closeItemSelection();
        return;
    }
    host.replaceItemSelection(buildItemViewSnapshot(nextItems, compassPosition_));
    host.closeItemSelection();
}

void WorldItemSelectionController::consumeItem(ItemSelectionHost &host,
                                                std::int16_t itemId,
                                                std::int16_t charId) {
    const auto itemInfo = ::hojy::world::state::itemPolicyInfo(itemId);
    auto candidate = ::hojy::world::state::prepareConsumeItem(itemId, charId);
    if (!itemInfo || !candidate) {
        host.closeItemSelection();
        return;
    }

    std::vector<std::wstring> messages = {
        GETTEXT(37) + L' ' + itemInfo->name};
    for (const auto &change: candidate->changes()) {
        messages.emplace_back(fmt::format(
            L"{} {} {}", ::hojy::world::state::propToName(change.first),
            GETTEXT(change.second ? 34 : 35), change.second));
    }
    const auto nextItems = candidate->bagItems();
    if (!::hojy::world::state::commitItemAction(std::move(*candidate))) {
        host.closeItemSelection();
        return;
    }
    host.replaceItemSelection(buildItemViewSnapshot(nextItems, compassPosition_));
    host.showItemMessage(messageRequest(
        std::move(messages), ScenePopupType::PressToCloseParent));
}

}
