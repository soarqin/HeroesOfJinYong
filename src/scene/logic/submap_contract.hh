#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace hojy::scene::logic {

inline constexpr std::size_t SubMapEventCount = 200;

struct SubMapEventRecord final {
    std::int16_t blocked = 0;
    std::int16_t index = -1;
    std::int16_t currTex = -1;
    std::int16_t endTex = -1;
    std::int16_t begTex = -1;
    std::int16_t texDelay = 0;
    std::int16_t x = 0;
    std::int16_t y = 0;
};

struct SubMapAnimationState final {
    std::int32_t current = 0;
    std::int32_t loop = 0;
    std::int32_t delay = 0;
};

struct SubMapStateSnapshot final {
    std::array<bool, SubMapEventCount> activeEvents{};
    std::vector<std::int16_t> eventAssetIds;
};

bool decodeOptionalSubMapAsset(std::int16_t encoded,
                               std::size_t assetCount,
                               std::int16_t &id) noexcept;

bool translateSubMapAnimationEnd(std::int16_t sourceBegin,
                                 std::int16_t sourceEnd,
                                 std::int16_t targetBegin,
                                 std::int16_t &targetEnd) noexcept;

bool advanceSubMapAnimation(SubMapAnimationState &state,
                            std::int32_t beg, std::int32_t end,
                            std::int32_t texDelay) noexcept;

bool validateSubMapEventTable(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid,
    std::array<bool, SubMapEventCount> &active);

bool validateSubMapLayerEventReferences(
    const std::vector<std::int16_t> &eventLayer,
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::array<bool, SubMapEventCount> &active,
    int mapWidth, int mapHeight);

bool validateSubMapState(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::vector<std::int16_t> &eventLayer,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid,
    std::array<bool, SubMapEventCount> &active);

bool buildSubMapStateSnapshot(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::vector<std::int16_t> &eventLayer,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid,
    SubMapStateSnapshot &snapshot);

}
