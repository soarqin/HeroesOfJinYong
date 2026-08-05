#pragma once

#include "bag.hh"
#include "savedata.hh"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hojy::world::state {

class UtilityActionCandidate final {
public:
    UtilityActionCandidate(UtilityActionCandidate &&) noexcept = default;
    UtilityActionCandidate &operator=(UtilityActionCandidate &&) noexcept = default;
    UtilityActionCandidate(const UtilityActionCandidate &) = delete;
    UtilityActionCandidate &operator=(const UtilityActionCandidate &) = delete;

    [[nodiscard]] std::int16_t result() const noexcept { return result_; }
    [[nodiscard]] std::int16_t eventId() const noexcept { return eventId_; }

private:
    friend std::optional<UtilityActionCandidate> prepareMedic(
        std::int16_t, std::int16_t, std::int16_t);
    friend std::optional<UtilityActionCandidate> prepareDepoison(
        std::int16_t, std::int16_t, std::int16_t);
    friend std::optional<UtilityActionCandidate> prepareLeaveTeam(std::int16_t);
    friend bool commitUtilityAction(UtilityActionCandidate &&);

    UtilityActionCandidate(SaveData saveData, std::int16_t result,
                           std::int16_t eventId, std::uint64_t revision) noexcept;

    SaveData saveData_;
    std::int16_t result_ = 0;
    std::int16_t eventId_ = -1;
    std::uint64_t revision_ = 0;
};

[[nodiscard]] std::optional<UtilityActionCandidate> prepareMedic(
        std::int16_t actorId, std::int16_t targetId, std::int16_t stamina);
[[nodiscard]] std::optional<UtilityActionCandidate> prepareDepoison(
        std::int16_t actorId, std::int16_t targetId, std::int16_t stamina);
[[nodiscard]] std::optional<UtilityActionCandidate> prepareLeaveTeam(
        std::int16_t characterId);
bool commitUtilityAction(UtilityActionCandidate &&candidate);

struct ShopListing final {
    std::int16_t slot = -1;
    std::int16_t itemId = -1;
    std::int16_t total = 0;
    std::int16_t price = 0;
    std::wstring name;
};

struct ShopSnapshot final {
    std::int16_t shopId = -1;
    std::uint64_t revision = 0;
    std::vector<ShopListing> listings;
};

class ShopPurchaseCandidate final {
public:
    ShopPurchaseCandidate(ShopPurchaseCandidate &&) noexcept = default;
    ShopPurchaseCandidate &operator=(ShopPurchaseCandidate &&) noexcept = default;
    ShopPurchaseCandidate(const ShopPurchaseCandidate &) = delete;
    ShopPurchaseCandidate &operator=(const ShopPurchaseCandidate &) = delete;

    [[nodiscard]] std::int16_t shopId() const noexcept { return shopId_; }
    [[nodiscard]] std::int16_t slot() const noexcept { return slot_; }
    [[nodiscard]] std::int16_t itemId() const noexcept { return itemId_; }
    [[nodiscard]] std::int16_t price() const noexcept { return price_; }

private:
    friend std::optional<ShopPurchaseCandidate> prepareShopPurchase(
        std::int16_t, std::int16_t);
    friend bool commitShopPurchase(ShopPurchaseCandidate &&);

    ShopPurchaseCandidate(SaveData saveData, Bag bag, std::int16_t shopId,
                          std::int16_t slot, std::int16_t itemId,
                          std::int16_t price, std::uint64_t revision) noexcept;

    SaveData saveData_;
    Bag bag_;
    std::int16_t shopId_ = -1;
    std::int16_t slot_ = -1;
    std::int16_t itemId_ = -1;
    std::int16_t price_ = 0;
    std::uint64_t revision_ = 0;
};

[[nodiscard]] std::optional<ShopSnapshot>
shopSnapshot(std::int16_t shopId);
[[nodiscard]] std::optional<ShopPurchaseCandidate>
prepareShopPurchase(std::int16_t shopId, std::int16_t slot);
bool commitShopPurchase(ShopPurchaseCandidate &&candidate);

}
