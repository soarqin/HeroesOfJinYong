#pragma once

#include "action.hh"
#include "bag.hh"
#include "savedata.hh"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hojy::world::state {

struct ItemPolicyInfo final {
    std::int16_t id = -1;
    std::int16_t itemType = -1;
    std::int16_t user = -1;
    std::wstring name;
};

/** Immutable item/world values used to construct a selection request. */
struct ItemSelectionSnapshot final {
    std::vector<std::pair<std::int16_t, std::int16_t>> bagItems;
    std::vector<std::int16_t> teamMemberIds;
};

enum class ItemEquipValidation : std::uint8_t {
    Valid,
    Invalid,
    SkillSlotsFull,
    Requirements,
};

class ItemActionCandidate final {
public:
    ItemActionCandidate(ItemActionCandidate &&) noexcept = default;
    ItemActionCandidate &operator=(ItemActionCandidate &&) noexcept = default;
    ItemActionCandidate(const ItemActionCandidate &) = delete;
    ItemActionCandidate &operator=(const ItemActionCandidate &) = delete;

    [[nodiscard]] std::int16_t itemId() const noexcept { return itemId_; }
    [[nodiscard]] std::int16_t characterId() const noexcept { return characterId_; }
    [[nodiscard]] std::int16_t itemType() const noexcept { return itemType_; }
    [[nodiscard]] std::int16_t resultValue() const noexcept { return resultValue_; }
    [[nodiscard]] const std::map<PropType, std::int16_t> &changes() const noexcept {
        return changes_;
    }
    [[nodiscard]] const std::vector<std::pair<std::int16_t, std::int16_t>> &
    bagItems() const noexcept { return bagItems_; }

private:
    friend std::optional<ItemActionCandidate> prepareEquipItem(
        std::int16_t, std::int16_t);
    friend std::optional<ItemActionCandidate> prepareConsumeItem(
        std::int16_t, std::int16_t);
    friend bool commitItemAction(ItemActionCandidate &&);

    ItemActionCandidate(SaveData saveData, Bag bag, std::int16_t itemId,
                        std::int16_t characterId, std::int16_t itemType,
                        std::int16_t resultValue,
                        std::map<PropType, std::int16_t> changes,
                        std::vector<std::pair<std::int16_t, std::int16_t>> bagItems,
                        std::uint64_t revision) noexcept;

    SaveData saveData_;
    Bag bag_;
    std::int16_t itemId_ = -1;
    std::int16_t characterId_ = -1;
    std::int16_t itemType_ = -1;
    std::int16_t resultValue_ = 0;
    std::map<PropType, std::int16_t> changes_;
    std::vector<std::pair<std::int16_t, std::int16_t>> bagItems_;
    std::uint64_t revision_ = 0;
};

[[nodiscard]] std::optional<ItemPolicyInfo>
itemPolicyInfo(std::int16_t itemId);

[[nodiscard]] ItemSelectionSnapshot itemSelectionSnapshot();

[[nodiscard]] std::optional<ItemActionCandidate>
prepareEquipItem(std::int16_t itemId, std::int16_t characterId);

[[nodiscard]] ItemEquipValidation validateEquipItem(
        std::int16_t itemId, std::int16_t characterId) noexcept;

[[nodiscard]] std::optional<ItemActionCandidate>
prepareConsumeItem(std::int16_t itemId, std::int16_t characterId);

/** Commit a candidate only if no newer item transaction has committed. */
bool commitItemAction(ItemActionCandidate &&candidate);

[[nodiscard]] std::uint64_t itemTransactionRevision() noexcept;

}
