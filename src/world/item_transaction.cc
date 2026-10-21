#include "item_transaction.hh"

#include "transaction.hh"
#include "world/strings.hh"

#include <cstddef>
#include <utility>

namespace hojy::world::state {
namespace {

std::vector<std::pair<std::int16_t, std::int16_t>> bagItems(
        const Bag &bag) {
    std::vector<std::pair<std::int16_t, std::int16_t>> result;
    result.reserve(bag.items().size());
    for (const auto &entry: bag.items()) {
        result.emplace_back(entry.first, entry.second);
    }
    return result;
}

std::vector<std::int16_t> memberIds(const SaveData &saveData) {
    std::vector<std::int16_t> result;
    const auto *base = saveData.baseInfo.operator->();
    if (!base) { return result; }
    result.reserve(::hojy::content::TeamMemberCount);
    for (const auto id: base->members) {
        if (id >= 0) { result.emplace_back(id); }
    }
    return result;
}

bool validCharacter(const SaveData &saveData, std::int16_t id) noexcept {
    return id >= 0 && static_cast<std::size_t>(id) < saveData.charInfo.size()
        && saveData.charInfo[id] != nullptr;
}

bool validItem(const SaveData &saveData, std::int16_t id) noexcept {
    return id >= 0 && static_cast<std::size_t>(id) < saveData.itemInfo.size()
        && saveData.itemInfo[id] != nullptr;
}

}

ItemActionCandidate::ItemActionCandidate(
        SaveData saveData, Bag bag, std::int16_t itemId,
        std::int16_t characterId, std::int16_t itemType,
        std::int16_t resultValue,
        std::map<PropType, std::int16_t> changes,
        std::vector<std::pair<std::int16_t, std::int16_t>> bagItems,
        std::uint64_t revision) noexcept:
    saveData_(std::move(saveData)), bag_(std::move(bag)), itemId_(itemId),
    characterId_(characterId), itemType_(itemType), resultValue_(resultValue),
    changes_(std::move(changes)), bagItems_(std::move(bagItems)),
    revision_(revision) {
}

std::optional<ItemPolicyInfo> itemPolicyInfo(std::int16_t itemId) {
    if (!validItem(gSaveData, itemId)) { return std::nullopt; }
    const auto *item = gSaveData.itemInfo[itemId];
    return ItemPolicyInfo{itemId, item->itemType, item->user,
                          GETITEMNAME(itemId)};
}

ItemSelectionSnapshot itemSelectionSnapshot() {
    return ItemSelectionSnapshot{bagItems(gBag), memberIds(gSaveData)};
}

ItemEquipValidation validateEquipItem(
        std::int16_t itemId, std::int16_t characterId) noexcept {
    if (!validCharacter(gSaveData, characterId)
        || !validItem(gSaveData, itemId)) {
        return ItemEquipValidation::Invalid;
    }
    const auto *item = gSaveData.itemInfo[itemId];
    const auto *character = gSaveData.charInfo[characterId];
    if (item->itemType != 1 && item->itemType != 2) {
        return ItemEquipValidation::Invalid;
    }
    if (item->itemType == 2 && skillFull(gSaveData, characterId)) {
        return ItemEquipValidation::SkillSlotsFull;
    }
    return canUseItem(character, item)
        ? ItemEquipValidation::Valid
        : ItemEquipValidation::Requirements;
}

std::optional<ItemActionCandidate>
prepareEquipItem(std::int16_t itemId, std::int16_t characterId) {
    const auto validation = validateEquipItem(itemId, characterId);
    if (validation != ItemEquipValidation::Valid) { return std::nullopt; }
    const auto *item = gSaveData.itemInfo[itemId];
    auto candidateSave = gSaveData;
    auto candidateBag = gBag;
    if (!equipItem(candidateSave, characterId, itemId)) {
        return std::nullopt;
    }
    return ItemActionCandidate(
        std::move(candidateSave), std::move(candidateBag), itemId,
        characterId, item->itemType, 0, {}, bagItems(gBag), stateRevision());
}

std::optional<ItemActionCandidate>
prepareConsumeItem(std::int16_t itemId, std::int16_t characterId) {
    if (!validCharacter(gSaveData, characterId)
        || !validItem(gSaveData, itemId) || gBag[itemId] <= 0) {
        return std::nullopt;
    }
    auto candidateSave = gSaveData;
    auto candidateBag = gBag;
    std::map<PropType, std::int16_t> changes;
    auto *character = candidateSave.charInfo[characterId];
    if (!useItem(candidateSave.itemInfo, candidateBag, character, itemId,
                 changes)) {
        return std::nullopt;
    }
    return ItemActionCandidate(
        std::move(candidateSave), std::move(candidateBag), itemId,
        characterId, gSaveData.itemInfo[itemId]->itemType, 0,
        std::move(changes), bagItems(candidateBag),
        stateRevision());
}

bool commitItemAction(ItemActionCandidate &&candidate) {
    if (candidate.revision_ != stateRevision()) { return false; }
    gSaveData = std::move(candidate.saveData_);
    gBag = std::move(candidate.bag_);
    bumpStateRevision();
    return true;
}

std::uint64_t itemTransactionRevision() noexcept {
    return stateRevision();
}

}
