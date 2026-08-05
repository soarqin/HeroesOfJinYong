#include "submap_contract.hh"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace hojy::scene::logic {
namespace {

bool validAssetValue(std::int16_t value, std::size_t textureCount,
                     const std::function<bool(std::int16_t)> &textureValid) {
    if (value < -1) { return false; }
    if (value == -1) { return true; }
    const auto id = static_cast<std::size_t>(value >> 1);
    return id < textureCount && textureValid && textureValid(value);
}

bool between(std::int16_t value, std::int16_t first, std::int16_t last) {
    const auto low = std::min(first, last);
    const auto high = std::max(first, last);
    return value >= low && value <= high;
}

bool validateActiveEventTable(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::array<bool, SubMapEventCount> &active,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid) {
    if (mapWidth <= 0 || mapHeight <= 0 || textureCount == 0
        || !textureValid) {
        return false;
    }
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (!active[i]) { continue; }
        const auto &event = events[i];
        if (event.x < 0 || event.x >= mapWidth
            || event.y < 0 || event.y >= mapHeight
            || !validAssetValue(event.currTex, textureCount, textureValid)
            || !validAssetValue(event.begTex, textureCount, textureValid)
            || !validAssetValue(event.endTex, textureCount, textureValid)) {
            return false;
        }
        if (event.begTex == event.endTex) { continue; }
        if (event.index < 0
            || static_cast<std::size_t>(event.index) >= events.size()
            || event.texDelay <= 0
            || event.texDelay < std::abs(
                static_cast<int>(event.endTex) - static_cast<int>(event.begTex))
            || !between(event.currTex, event.begTex, event.endTex)) {
            return false;
        }
    }
    return true;
}

bool collectActiveLayerReferences(
    const std::vector<std::int16_t> &eventLayer,
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    int mapWidth, int mapHeight,
    std::array<bool, SubMapEventCount> &active) {
    if (mapWidth <= 0 || mapHeight <= 0
        || eventLayer.size() != static_cast<std::size_t>(mapWidth)
            * static_cast<std::size_t>(mapHeight)) {
        return false;
    }
    std::array<bool, SubMapEventCount> candidateActive{};
    for (std::size_t pos = 0; pos < eventLayer.size(); ++pos) {
        const auto eventIndex = eventLayer[pos];
        if (eventIndex == -1) { continue; }
        if (eventIndex < -1) { return false; }
        const auto index = static_cast<std::size_t>(eventIndex);
        if (index >= events.size() || candidateActive[index]) { return false; }
        const auto x = static_cast<int>(pos % static_cast<std::size_t>(mapWidth));
        const auto y = static_cast<int>(pos / static_cast<std::size_t>(mapWidth));
        if (events[index].x != x || events[index].y != y) { return false; }
        candidateActive[index] = true;
    }
    active = candidateActive;
    return true;
}

}

bool decodeOptionalSubMapAsset(std::int16_t encoded,
                               std::size_t assetCount,
                               std::int16_t &id) noexcept {
    if (encoded == -1) {
        id = -1;
        return true;
    }
    if (encoded < -1) { return false; }
    const auto decoded = static_cast<std::size_t>(encoded >> 1);
    if (decoded >= assetCount
        || decoded > static_cast<std::size_t>(
            std::numeric_limits<std::int16_t>::max())) {
        return false;
    }
    id = static_cast<std::int16_t>(decoded);
    return true;
}

bool translateSubMapAnimationEnd(std::int16_t sourceBegin,
                                 std::int16_t sourceEnd,
                                 std::int16_t targetBegin,
                                 std::int16_t &targetEnd) noexcept {
    const auto value = static_cast<std::int32_t>(targetBegin)
        + static_cast<std::int32_t>(sourceEnd)
        - static_cast<std::int32_t>(sourceBegin);
    if (value < std::numeric_limits<std::int16_t>::min()
        || value > std::numeric_limits<std::int16_t>::max()) {
        return false;
    }
    targetEnd = static_cast<std::int16_t>(value);
    return true;
}

bool advanceSubMapAnimation(SubMapAnimationState &state,
                            std::int32_t beg, std::int32_t end,
                            std::int32_t texDelay) noexcept {
    if (beg == end) { return false; }
    const auto span = std::llabs(
        static_cast<long long>(end) - static_cast<long long>(beg));
    if (span <= 0 || span > std::numeric_limits<std::int32_t>::max()
        || texDelay <= 0 || static_cast<long long>(texDelay) < span
        || state.current < std::min(beg, end)
        || state.current > std::max(beg, end)
        || state.loop < 0 || state.delay < 0) {
        return false;
    }
    if (state.current == beg && state.delay > 0) {
        --state.delay;
        if (state.delay == 0) { state.loop = 0; }
        return false;
    }
    if (state.current == end) {
        state.current = beg;
        if (state.loop >= 2) {
            state.delay = static_cast<std::int32_t>(
                static_cast<long long>(texDelay) - span);
            state.loop = 0;
        } else {
            ++state.loop;
        }
        return true;
    }
    state.current += beg < end ? 1 : -1;
    return true;
}

bool validateSubMapEventTable(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid,
    std::array<bool, SubMapEventCount> &active) {
    std::array<bool, SubMapEventCount> candidateActive{};
    for (std::size_t i = 0; i < events.size(); ++i) {
        const auto &event = events[i];
        candidateActive[i] = event.x > 0;
    }
    if (!validateActiveEventTable(events, candidateActive, mapWidth, mapHeight,
                                  textureCount, textureValid)) { return false; }
    active = candidateActive;
    return true;
}

bool validateSubMapLayerEventReferences(
    const std::vector<std::int16_t> &eventLayer,
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::array<bool, SubMapEventCount> &active,
    int mapWidth, int mapHeight) {
    std::array<bool, SubMapEventCount> referenced{};
    return collectActiveLayerReferences(
               eventLayer, events, mapWidth, mapHeight, referenced)
        && referenced == active;
}

bool validateSubMapState(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::vector<std::int16_t> &eventLayer,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid,
    std::array<bool, SubMapEventCount> &active) {
    std::array<bool, SubMapEventCount> candidateActive{};
    if (!collectActiveLayerReferences(
            eventLayer, events, mapWidth, mapHeight, candidateActive)
        || !validateActiveEventTable(
            events, candidateActive, mapWidth, mapHeight,
            textureCount, textureValid)) {
        return false;
    }
    active = candidateActive;
    return true;
}

bool buildSubMapStateSnapshot(
    const std::array<SubMapEventRecord, SubMapEventCount> &events,
    const std::vector<std::int16_t> &eventLayer,
    int mapWidth, int mapHeight, std::size_t textureCount,
    const std::function<bool(std::int16_t)> &textureValid,
    SubMapStateSnapshot &snapshot) {
    SubMapStateSnapshot candidate;
    if (!validateSubMapState(
            events, eventLayer, mapWidth, mapHeight, textureCount,
            textureValid, candidate.activeEvents)) {
        return false;
    }
    candidate.eventAssetIds.assign(eventLayer.size(), -1);
    for (std::size_t pos = 0; pos < eventLayer.size(); ++pos) {
        const auto eventIndex = eventLayer[pos];
        if (eventIndex < 0) { continue; }
        const auto encoded = events[static_cast<std::size_t>(eventIndex)].currTex;
        std::int16_t textureId = -1;
        if (!decodeOptionalSubMapAsset(encoded, textureCount, textureId)
            || encoded >= 0 && !textureValid(encoded)) {
            return false;
        }
        candidate.eventAssetIds[pos] = textureId;
    }
    snapshot = std::move(candidate);
    return true;
}

}
