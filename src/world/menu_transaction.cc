#include "menu_transaction.hh"

#include "action.hh"
#include "content/constants.hh"
#include "transaction.hh"
#include "world/strings.hh"

#include <cstddef>
#include <utility>

namespace hojy::world::state {
namespace {

bool validCharacter(const SaveData &saveData, std::int16_t id) noexcept {
    return id >= 0 && static_cast<std::size_t>(id) < saveData.charInfo.size()
        && saveData.charInfo[id] != nullptr;
}

}

UtilityActionCandidate::UtilityActionCandidate(
        SaveData saveData, std::int16_t result, std::int16_t eventId,
        std::uint64_t revision) noexcept:
    saveData_(std::move(saveData)), result_(result), eventId_(eventId),
    revision_(revision) {
}

std::optional<UtilityActionCandidate> prepareMedic(
        std::int16_t actorId, std::int16_t targetId, std::int16_t stamina) {
    if (!validCharacter(gSaveData, actorId)
        || !validCharacter(gSaveData, targetId) || stamina < 0) {
        return std::nullopt;
    }
    auto candidate = gSaveData;
    const auto result = actMedic(candidate.charInfo[actorId],
                                 candidate.charInfo[targetId], stamina);
    return UtilityActionCandidate(std::move(candidate), result, -1,
                                  stateRevision());
}

std::optional<UtilityActionCandidate> prepareDepoison(
        std::int16_t actorId, std::int16_t targetId, std::int16_t stamina) {
    if (!validCharacter(gSaveData, actorId)
        || !validCharacter(gSaveData, targetId) || stamina < 0) {
        return std::nullopt;
    }
    auto candidate = gSaveData;
    const auto result = actDepoison(candidate.charInfo[actorId],
                                    candidate.charInfo[targetId], stamina);
    return UtilityActionCandidate(std::move(candidate), result, -1,
                                  stateRevision());
}

std::optional<UtilityActionCandidate> prepareLeaveTeam(
        std::int16_t characterId) {
    auto candidate = gSaveData;
    if (!leaveTeam(candidate, characterId)) { return std::nullopt; }
    return UtilityActionCandidate(
        std::move(candidate), 0, getLeaveEventId(characterId), stateRevision());
}

bool commitUtilityAction(UtilityActionCandidate &&candidate) {
    if (candidate.revision_ != stateRevision()) { return false; }
    gSaveData = std::move(candidate.saveData_);
    bumpStateRevision();
    return true;
}

std::optional<ShopSnapshot> shopSnapshot(std::int16_t shopId) {
    ShopSnapshot result;
    result.shopId = shopId;
    result.revision = stateRevision();
    if (shopId < 0 || static_cast<std::size_t>(shopId) >= gSaveData.shopInfo.size()) {
        return result;
    }
    const auto *shop = gSaveData.shopInfo[shopId];
    if (!shop) { return result; }
    for (int slot = 0; slot < ::hojy::content::ShopItemCount; ++slot) {
        if (shop->id[slot] <= 0 || shop->total[slot] <= 0
            || shop->price[slot] < 0) {
            continue;
        }
        result.listings.push_back(ShopListing{
            static_cast<std::int16_t>(slot), shop->id[slot], shop->total[slot],
            shop->price[slot], GETITEMNAME(shop->id[slot])});
    }
    return result;
}

std::optional<ShopPurchaseCandidate> prepareShopPurchase(
        std::int16_t shopId, std::int16_t slot) {
    if (shopId < 0 || static_cast<std::size_t>(shopId) >= gSaveData.shopInfo.size()
        || slot < 0 || slot >= ::hojy::content::ShopItemCount) {
        return std::nullopt;
    }
    const auto *shop = gSaveData.shopInfo[shopId];
    if (!shop || shop->id[slot] <= 0 || shop->total[slot] <= 0
        || shop->price[slot] < 0) {
        return std::nullopt;
    }
    auto candidateSave = gSaveData;
    auto candidateBag = gBag;
    if (!candidateBag.remove(::hojy::content::ItemIDMoney, shop->price[slot])) {
        return std::nullopt;
    }
    candidateBag.add(shop->id[slot], 1);
    auto *candidateShop = candidateSave.shopInfo[shopId];
    if (!candidateShop) { return std::nullopt; }
    if (candidateShop->total[slot] < 1000) { --candidateShop->total[slot]; }
    return ShopPurchaseCandidate(
        std::move(candidateSave), std::move(candidateBag), shopId, slot,
        shop->id[slot], shop->price[slot], stateRevision());
}

ShopPurchaseCandidate::ShopPurchaseCandidate(
        SaveData saveData, Bag bag, std::int16_t shopId, std::int16_t slot,
        std::int16_t itemId, std::int16_t price, std::uint64_t revision) noexcept:
    saveData_(std::move(saveData)), bag_(std::move(bag)), shopId_(shopId),
    slot_(slot), itemId_(itemId), price_(price), revision_(revision) {
}

bool commitShopPurchase(ShopPurchaseCandidate &&candidate) {
    if (candidate.revision_ != stateRevision()) { return false; }
    gSaveData = std::move(candidate.saveData_);
    gBag = std::move(candidate.bag_);
    bumpStateRevision();
    return true;
}

}
