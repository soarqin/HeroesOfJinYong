#include "scene/logic/battle_effect_snapshot.hh"

#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

using hojy::scene::logic::BattleEffectCell;
using hojy::scene::logic::BattleEffectOverlaySnapshot;

void testSnapshotUsesStableCellIndicesAndDropsDuplicates() {
    BattleEffectOverlaySnapshot snapshot;
    const std::vector<BattleEffectCell> cells{{1, 1}, {3, 2}, {1, 1}};

    HOJY_CHECK_EQ(
        hojy::scene::logic::buildBattleEffectOverlaySnapshot(
            5, 4, 7, -2, cells, snapshot),
        true);
    HOJY_CHECK_EQ(snapshot.effectAssetId, 7);
    HOJY_CHECK_EQ(snapshot.frameIndex, -2);
    HOJY_CHECK_EQ(snapshot.cellIndices,
                  (std::vector<std::size_t>{6U, 13U}));
}

void testInvalidCandidateLeavesPreviousSnapshotUntouched() {
    BattleEffectOverlaySnapshot snapshot;
    snapshot.effectAssetId = 3;
    snapshot.frameIndex = 4;
    snapshot.cellIndices = {9U};

    HOJY_CHECK_EQ(
        hojy::scene::logic::buildBattleEffectOverlaySnapshot(
            3, 3, 8, 0, {{3, 0}}, snapshot),
        false);
    HOJY_CHECK_EQ(snapshot.effectAssetId, 3);
    HOJY_CHECK_EQ(snapshot.frameIndex, 4);
    HOJY_CHECK_EQ(snapshot.cellIndices,
                  (std::vector<std::size_t>{9U}));
}

}

int main() {
    try {
        testSnapshotUsesStableCellIndicesAndDropsDuplicates();
        testInvalidCandidateLeavesPreviousSnapshotUntouched();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
