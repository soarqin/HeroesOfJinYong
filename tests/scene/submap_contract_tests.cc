#include "scene/logic/submap_contract.hh"
#include "test_support.hh"

#include <array>
#include <iostream>
#include <limits>
#include <vector>

namespace {

using hojy::scene::logic::SubMapEventCount;
using hojy::scene::logic::SubMapEventRecord;

void testActiveEventBitmapAndAnimationContract() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[0] = {0, 0, 2, 4, 2, 2, 1, 2};
    std::array<bool, SubMapEventCount> active{};
    const auto textureValid = [](std::int16_t value) {
        return value >= 0;
    };

    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapEventTable(
                      events, 64, 64, 8, textureValid, active), true);
    HOJY_CHECK_EQ(active[0], true);
    HOJY_CHECK_EQ(active[1], false);

    events[0].texDelay = 1;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapEventTable(
                      events, 64, 64, 8, textureValid, active), false);
}

void testLayerReferencesRequireActiveEventAtMatchingCoordinate() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[0] = {0, 0, 2, 2, 2, 0, 1, 2};
    std::array<bool, SubMapEventCount> active{};
    const auto textureValid = [](std::int16_t value) { return value >= 0; };
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapEventTable(
                      events, 4, 4, 8, textureValid, active), true);

    std::vector<std::int16_t> layer(16, -1);
    layer[2 * 4 + 1] = 0;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapLayerEventReferences(
                      layer, events, active, 4, 4), true);

    layer[2 * 4 + 1] = 1;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapLayerEventReferences(
                      layer, events, active, 4, 4), false);
}

void testEventLinksAreOrderIndependentButMustBeReferenced() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    // The first animated event points forward to the second active record.
    events[0] = {0, 1, 2, 4, 2, 2, 1, 1};
    events[1] = {0, 0, 4, 4, 4, 0, 2, 1};
    std::array<bool, SubMapEventCount> active{};
    const auto textureValid = [](std::int16_t value) { return value >= 0; };
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapEventTable(
                      events, 4, 4, 8, textureValid, active), true);

    std::vector<std::int16_t> layer(16, -1);
    layer[1 * 4 + 1] = 0;
    layer[1 * 4 + 2] = 1;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapLayerEventReferences(
                      layer, events, active, 4, 4), true);

    layer[1 * 4 + 2] = -1;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapLayerEventReferences(
                      layer, events, active, 4, 4), false);
}

void testAnimationClockSlotDoesNotNeedToReferenceAnActiveEvent() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[0] = {0, static_cast<std::int16_t>(SubMapEventCount - 1),
                 2, 4, 2, 2, 1, 1};
    std::vector<std::int16_t> layer(16, -1);
    layer[5] = 0;
    std::array<bool, SubMapEventCount> active{};
    const auto textureValid = [](std::int16_t value) { return value >= 0; };

    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapState(
                      events, layer, 4, 4, 8, textureValid, active), true);
    events[0].index = static_cast<std::int16_t>(SubMapEventCount);
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapState(
                      events, layer, 4, 4, 8, textureValid, active), false);
}

void testLayerReferencesDefineActiveEventsWithoutPrefixOrPositiveXSentinel() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[7] = {0, -1, 2, 2, 2, 0, 0, 2};
    std::vector<std::int16_t> layer(16, -1);
    layer[8] = 7;
    std::array<bool, SubMapEventCount> active{};
    const auto textureValid = [](std::int16_t value) { return value >= 0; };

    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapState(
                      events, layer, 4, 4, 8, textureValid, active), true);
    HOJY_CHECK_EQ(active[0], false);
    HOJY_CHECK_EQ(active[7], true);
}

void testOptionalTextureSentinelRejectsOtherNegativeValues() {
    std::int16_t decoded = 123;
    HOJY_CHECK_EQ(hojy::scene::logic::decodeOptionalSubMapAsset(
                      -1, 8, decoded), true);
    HOJY_CHECK_EQ(decoded, -1);

    decoded = 123;
    HOJY_CHECK_EQ(hojy::scene::logic::decodeOptionalSubMapAsset(
                      -2, 8, decoded), false);
    HOJY_CHECK_EQ(decoded, 123);

    HOJY_CHECK_EQ(hojy::scene::logic::decodeOptionalSubMapAsset(
                      6, 4, decoded), true);
    HOJY_CHECK_EQ(decoded, 3);
    HOJY_CHECK_EQ(hojy::scene::logic::decodeOptionalSubMapAsset(
                      8, 4, decoded), false);
}

void testMovementBlockingPreservesOriginalBuildingSentinelSemantics() {
    HOJY_CHECK_EQ(hojy::scene::logic::subMapCellBlocksMovement(-1, false), true);
    HOJY_CHECK_EQ(hojy::scene::logic::subMapCellBlocksMovement(0, false), false);
    HOJY_CHECK_EQ(hojy::scene::logic::subMapCellBlocksMovement(1, false), true);
    HOJY_CHECK_EQ(hojy::scene::logic::subMapCellBlocksMovement(0, true), true);
}

void testSnapshotBuildIsAtomicAndDerivesEventTextureIds() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[3] = {0, -1, 6, 6, 6, 0, 1, 2};
    std::vector<std::int16_t> layer(16, -1);
    layer[9] = 3;
    const auto textureValid = [](std::int16_t value) { return value >= 0; };
    hojy::scene::logic::SubMapStateSnapshot snapshot;

    HOJY_CHECK_EQ(hojy::scene::logic::buildSubMapStateSnapshot(
                      events, layer, 4, 4, 8, textureValid, snapshot), true);
    HOJY_CHECK_EQ(snapshot.activeEvents[3], true);
    HOJY_CHECK_EQ(snapshot.eventAssetIds.size(), 16U);
    HOJY_CHECK_EQ(snapshot.eventAssetIds[9], 3);
    HOJY_CHECK_EQ(snapshot.eventAssetIds[0], -1);

    const auto before = snapshot;
    events[3].currTex = -2;
    HOJY_CHECK_EQ(hojy::scene::logic::buildSubMapStateSnapshot(
                      events, layer, 4, 4, 8, textureValid, snapshot), false);
    HOJY_CHECK_EQ(snapshot.activeEvents, before.activeEvents);
    HOJY_CHECK_EQ(snapshot.eventAssetIds, before.eventAssetIds);
}

void testTranslatedAnimationEndRejectsInt16Overflow() {
    std::int16_t translated = 123;
    HOJY_CHECK_EQ(hojy::scene::logic::translateSubMapAnimationEnd(
                      10, 14, 20, translated), true);
    HOJY_CHECK_EQ(translated, 24);

    translated = 123;
    HOJY_CHECK_EQ(hojy::scene::logic::translateSubMapAnimationEnd(
                      std::numeric_limits<std::int16_t>::min(),
                      std::numeric_limits<std::int16_t>::max(),
                      1, translated), false);
    HOJY_CHECK_EQ(translated, 123);
}

void testFailedEventValidationDoesNotOverwriteActiveBitmap() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[0] = {0, 0, 2, 2, 2, 0, 1, 1};
    std::array<bool, SubMapEventCount> active{};
    active[7] = true;
    const auto before = active;
    const auto textureValid = [](std::int16_t value) { return value >= 0; };

    events[0].currTex = -2;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapEventTable(
                      events, 4, 4, 8, textureValid, active), false);
    HOJY_CHECK_EQ(active, before);
}

void testCombinedStateValidationRejectsUncommittableEventOrLayerMutation() {
    std::array<SubMapEventRecord, SubMapEventCount> events{};
    events[0] = {0, 0, 2, 2, 2, 0, 1, 1};
    std::vector<std::int16_t> layer(16, -1);
    layer[5] = 0;
    std::array<bool, SubMapEventCount> active{};
    const auto textureValid = [](std::int16_t value) { return value >= 0; };

    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapState(
                      events, layer, 4, 4, 8, textureValid, active), true);

    auto invalid = layer;
    invalid[5] = 1;
    HOJY_CHECK_EQ(hojy::scene::logic::validateSubMapState(
                      events, invalid, 4, 4, 8, textureValid, active), false);
}

void testAnimationClockResetsLoopAtZeroDelay() {
    hojy::scene::logic::SubMapAnimationState state{4, 2, 0};
    HOJY_CHECK_EQ(hojy::scene::logic::advanceSubMapAnimation(
                      state, 2, 4, 2), true);
    HOJY_CHECK_EQ(state.current, 2);
    HOJY_CHECK_EQ(state.loop, 0);
    HOJY_CHECK_EQ(state.delay, 0);
}

void testAnimationClockRejectsShortDelayAndOutOfRangeCurrent() {
    hojy::scene::logic::SubMapAnimationState shortDelay{2, 0, 0};
    HOJY_CHECK_EQ(hojy::scene::logic::advanceSubMapAnimation(
                      shortDelay, 2, 4, 1), false);
    hojy::scene::logic::SubMapAnimationState invalidCurrent{8, 0, 0};
    HOJY_CHECK_EQ(hojy::scene::logic::advanceSubMapAnimation(
                      invalidCurrent, 2, 4, 2), false);

    hojy::scene::logic::SubMapAnimationState overflowSafe{0, 0, 0};
    HOJY_CHECK_EQ(hojy::scene::logic::advanceSubMapAnimation(
                      overflowSafe, std::numeric_limits<std::int32_t>::min(),
                      std::numeric_limits<std::int32_t>::max(),
                      std::numeric_limits<std::int32_t>::max()), false);
}

}

int main() {
    try {
        testActiveEventBitmapAndAnimationContract();
        testLayerReferencesRequireActiveEventAtMatchingCoordinate();
        testEventLinksAreOrderIndependentButMustBeReferenced();
        testAnimationClockSlotDoesNotNeedToReferenceAnActiveEvent();
        testLayerReferencesDefineActiveEventsWithoutPrefixOrPositiveXSentinel();
        testOptionalTextureSentinelRejectsOtherNegativeValues();
        testMovementBlockingPreservesOriginalBuildingSentinelSemantics();
        testSnapshotBuildIsAtomicAndDerivesEventTextureIds();
        testTranslatedAnimationEndRejectsInt16Overflow();
        testFailedEventValidationDoesNotOverwriteActiveBitmap();
        testCombinedStateValidationRejectsUncommittableEventOrLayerMutation();
        testAnimationClockResetsLoopAtZeroDelay();
        testAnimationClockRejectsShortDelayAndOutOfRangeCurrent();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
