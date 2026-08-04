#include "world_state.hh"

#include "world/baseinfo.hh"
#include "world/savedata.hh"

#include <algorithm>

namespace hojy::world {

bool operator==(const InventoryItem &left, const InventoryItem &right) noexcept {
    return left.id == right.id && left.count == right.count;
}

bool operator==(const WorldPosition &left, const WorldPosition &right) noexcept {
    return left.onShip == right.onShip
        && left.subMap == right.subMap
        && left.mainX == right.mainX
        && left.mainY == right.mainY
        && left.subX == right.subX
        && left.subY == right.subY
        && left.direction == right.direction
        && left.shipX == right.shipX
        && left.shipY == right.shipY
        && left.shipX1 == right.shipX1
        && left.shipY1 == right.shipY1
        && left.encode == right.encode;
}

bool operator==(const SaveSnapshot &left, const SaveSnapshot &right) noexcept {
    return left.position == right.position
        && left.team == right.team
        && left.inventory == right.inventory;
}

InventoryView::InventoryView(::hojy::world::state::BaseData &base) noexcept:
    base_(&base), mutableBase_(&base) {
}

InventoryView::InventoryView(const ::hojy::world::state::BaseData &base) noexcept:
    base_(&base) {
}

std::vector<InventoryItem> InventoryView::items() const {
    std::vector<InventoryItem> result;
    result.reserve(content::BagItemCount);
    for (const auto &item: base_->items) {
        if (item.id >= 0 && item.count > 0) {
            result.push_back(InventoryItem{item.id, item.count});
        }
    }
    return result;
}

bool InventoryView::replace(const std::vector<InventoryItem> &items) {
    if (mutableBase_ == nullptr || items.size() > content::BagItemCount) {
        return false;
    }
    for (const auto &item: items) {
        if (item.id < 0 || item.id >= content::BagItemCount || item.count <= 0) {
            return false;
        }
    }

    ::hojy::world::state::BaseData candidate = *mutableBase_;
    std::size_t index = 0;
    for (const auto &item: items) {
        candidate.items[index++] = {item.id, item.count};
    }
    for (; index < content::BagItemCount; ++index) {
        candidate.items[index] = {-1, 0};
    }
    *mutableBase_ = candidate;
    return true;
}

WorldStateView::WorldStateView(::hojy::world::state::SaveData &save) noexcept:
    save_(&save) {
}

SaveSnapshot WorldStateView::snapshot() const {
    const auto &base = *save_->baseInfo.operator->();
    SaveSnapshot result;
    result.position = {
        base.onShip, base.subMap, base.mainX, base.mainY, base.subX, base.subY,
        base.direction, base.shipX, base.shipY, base.shipX1, base.shipY1, base.encode,
    };
    std::copy(std::begin(base.members), std::end(base.members), result.team.begin());
    result.inventory = InventoryView(base).items();
    return result;
}

bool WorldStateView::apply(const SaveSnapshot &snapshot) {
    ::hojy::world::state::BaseData candidate = *save_->baseInfo.operator->();
    candidate.onShip = snapshot.position.onShip;
    candidate.subMap = snapshot.position.subMap;
    candidate.mainX = snapshot.position.mainX;
    candidate.mainY = snapshot.position.mainY;
    candidate.subX = snapshot.position.subX;
    candidate.subY = snapshot.position.subY;
    candidate.direction = snapshot.position.direction;
    candidate.shipX = snapshot.position.shipX;
    candidate.shipY = snapshot.position.shipY;
    candidate.shipX1 = snapshot.position.shipX1;
    candidate.shipY1 = snapshot.position.shipY1;
    candidate.encode = snapshot.position.encode;
    std::copy(snapshot.team.begin(), snapshot.team.end(), std::begin(candidate.members));
    if (!InventoryView(candidate).replace(snapshot.inventory)) {
        return false;
    }
    *save_->baseInfo.operator->() = candidate;
    return true;
}

}
