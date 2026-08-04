#pragma once

#include "content/constants.hh"

#include <array>
#include <cstdint>
#include <vector>

namespace hojy::world::state {
struct BaseData;
class SaveData;
}

namespace hojy::world {

struct InventoryItem {
    std::int16_t id = -1;
    std::int16_t count = 0;
};

[[nodiscard]] bool operator==(const InventoryItem &left,
                              const InventoryItem &right) noexcept;

struct WorldPosition {
    std::int16_t onShip = 0;
    std::int16_t subMap = 0;
    std::int16_t mainX = 0;
    std::int16_t mainY = 0;
    std::int16_t subX = 0;
    std::int16_t subY = 0;
    std::int16_t direction = 0;
    std::int16_t shipX = 0;
    std::int16_t shipY = 0;
    std::int16_t shipX1 = 0;
    std::int16_t shipY1 = 0;
    std::int16_t encode = 0;
};

[[nodiscard]] bool operator==(const WorldPosition &left,
                              const WorldPosition &right) noexcept;

struct SaveSnapshot {
    WorldPosition position;
    std::array<std::int16_t, content::TeamMemberCount> team{};
    std::vector<InventoryItem> inventory;
};

[[nodiscard]] bool operator==(const SaveSnapshot &left,
                              const SaveSnapshot &right) noexcept;

class InventoryView final {
public:
    explicit InventoryView(::hojy::world::state::BaseData &base) noexcept;
    explicit InventoryView(const ::hojy::world::state::BaseData &base) noexcept;

    [[nodiscard]] std::vector<InventoryItem> items() const;
    [[nodiscard]] bool replace(const std::vector<InventoryItem> &items);

private:
    const ::hojy::world::state::BaseData *base_ = nullptr;
    ::hojy::world::state::BaseData *mutableBase_ = nullptr;
};

class WorldStateView final {
public:
    explicit WorldStateView(::hojy::world::state::SaveData &save) noexcept;

    [[nodiscard]] SaveSnapshot snapshot() const;
    [[nodiscard]] bool apply(const SaveSnapshot &snapshot);

private:
    ::hojy::world::state::SaveData *save_ = nullptr;
};

}
