#include "content/constants.hh"
#include "world/savedata.hh"
#include "test_support.hh"
#include "world/world_state.hh"

#include <array>
#include <cstdint>
#include <iostream>

namespace {

void constantsHaveOneContentOwner() {
    static_assert(hojy::content::TeamMemberCount == 6, "team size changed");
    static_assert(hojy::content::BagItemCount == 200, "bag size changed");
    static_assert(hojy::content::SkillCheckCount == 10, "skill table changed");
    HOJY_CHECK_EQ(hojy::content::WarFieldLayerCount, 8);
}

hojy::world::state::SaveData makeSave() {
    hojy::world::state::SaveData save;
    auto &base = *save.baseInfo.operator->();
    base.onShip = 1;
    base.subMap = 3;
    base.mainX = 11;
    base.mainY = 12;
    base.subX = 13;
    base.subY = 14;
    base.direction = 2;
    base.members[0] = 101;
    base.members[1] = 102;
    for (auto &item: base.items) {
        item = {-1, 0};
    }
    base.items[0] = {7, 3};
    base.items[1] = {9, 1};
    return save;
}

void snapshotCapturesOnlyWorldFields() {
    auto save = makeSave();
    hojy::world::WorldStateView view(save);
    const auto snapshot = view.snapshot();
    HOJY_CHECK_EQ(snapshot.position.subMap, 3);
    HOJY_CHECK_EQ(snapshot.position.mainX, 11);
    HOJY_CHECK_EQ(snapshot.position.subY, 14);
    HOJY_CHECK_EQ(snapshot.team[0], 101);
    HOJY_CHECK_EQ(snapshot.inventory.size(), 2U);
    HOJY_CHECK_EQ(snapshot.inventory[1], (hojy::world::InventoryItem{9, 1}));
}

void invalidSnapshotDoesNotPartiallyApply() {
    auto save = makeSave();
    hojy::world::WorldStateView view(save);
    const auto before = view.snapshot();

    auto invalid = before;
    invalid.inventory.resize(hojy::content::BagItemCount + 1);
    HOJY_CHECK_EQ(view.apply(invalid), false);
    HOJY_CHECK_EQ(view.snapshot(), before);

    invalid = before;
    invalid.inventory[0].id = -1;
    HOJY_CHECK_EQ(view.apply(invalid), false);
    HOJY_CHECK_EQ(view.snapshot(), before);
}

void validSnapshotCommitsAsOneOperation() {
    auto save = makeSave();
    hojy::world::WorldStateView view(save);
    auto next = view.snapshot();
    next.position.mainX = 99;
    next.team[0] = 404;
    next.inventory = {{12, 2}};
    HOJY_CHECK_EQ(view.apply(next), true);
    HOJY_CHECK_EQ(view.snapshot(), next);
}

}

int main() {
    try {
        constantsHaveOneContentOwner();
        snapshotCapturesOnlyWorldFields();
        invalidSnapshotDoesNotPartiallyApply();
        validSnapshotCommitsAsOneOperation();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
