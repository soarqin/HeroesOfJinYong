#include "battle/movement.hh"
#include "test_support.hh"

#include <iostream>
#include <set>

namespace {

using Cell = std::pair<int, int>;

hojy::battle::SelectableCells makeMovementCells(
    int width, Cell actor, Cell target, int steps,
    const std::set<Cell> &blocked = {}) {
    hojy::battle::SelectableCells cells;
    hojy::battle::getSelectableArea(
        width, 1, actor, steps, 0, cells,
        [&](int x, int y) { return blocked.count({x, y}) != 0; },
        [&](int x, int y) { return Cell{x, y} == target; },
        [](int, int) { return false; });
    return cells;
}

hojy::battle::SelectableCells makeCastRangeCells(
    int width, Cell target, int range,
    const std::set<Cell> &blocked = {}) {
    hojy::battle::SelectableCells cells;
    hojy::battle::getSelectableArea(
        width, 1, target, 0, range, cells,
        [&](int x, int y) { return blocked.count({x, y}) != 0; },
        [](int, int) { return false; },
        [](int, int) { return false; });
    return cells;
}

void testSelectableAreaTracksMovementAndRangeParents() {
    const std::set<std::pair<int, int>> blocked{{1, 0}};
    const std::set<std::pair<int, int>> occupied{{2, 1}};
    hojy::battle::SelectableCells cells;
    hojy::battle::getSelectableArea(
        4, 3, {0, 1}, 2, 1, cells,
        [&](int x, int y) { return blocked.count({x, y}) != 0; },
        [&](int x, int y) { return occupied.count({x, y}) != 0; },
        [&](int, int) { return false; });
    HOJY_CHECK_EQ(cells.at({0, 1}).moves, 0);
    HOJY_CHECK_EQ(cells.at({0, 0}).moves, 1);
    HOJY_CHECK_EQ(cells.count({1, 0}), 0U);
    HOJY_CHECK_EQ(cells.at({2, 1}).moves, -1);
    HOJY_CHECK_EQ(cells.at({2, 2}).moves, -1);
    HOJY_CHECK_EQ(cells.at({2, 2}).ranges, 1);
}

void testSelectableAreaUsesOriginalDirectionPriority() {
    hojy::battle::SelectableCells cells;
    hojy::battle::getSelectableArea(
        4, 4, {1, 1}, 2, 0, cells,
        [](int, int) { return false; },
        [](int, int) { return false; },
        [](int, int) { return false; });

    const auto *parent = cells.at({2, 0}).moveParent;
    HOJY_CHECK_EQ(parent != nullptr, true);
    const auto parentPosition = std::make_pair(parent->x, parent->y);
    const auto expectedPosition = std::make_pair(1, 0);
    HOJY_CHECK_EQ(parentPosition, expectedPosition);
}

void testSelectableAreaUsesOriginalRangeDirectionPriority() {
    hojy::battle::SelectableCells cells;
    hojy::battle::getSelectableArea(
        3, 3, {1, 1}, 0, 2, cells,
        [](int, int) { return false; },
        [](int, int) { return false; },
        [](int, int) { return false; });

    const auto *parent = cells.at({0, 0}).rangeParent;
    HOJY_CHECK_EQ(parent != nullptr, true);
    const auto parentPosition = std::make_pair(parent->x, parent->y);
    const auto expectedPosition = std::make_pair(0, 1);
    HOJY_CHECK_EQ(parentPosition, expectedPosition);
}

void testReachableRangeAreaHonorsCharacterBlockers() {
    hojy::battle::SelectableCells cells;
    hojy::battle::getReachableRangeArea(
        5, 3, {4, 1}, 10, cells,
        [](int, int) { return false; },
        [](int x, int y) { return x == 2 && y == 1; });

    HOJY_CHECK_EQ(cells.count({2, 1}), 0U);
    HOJY_CHECK_EQ(cells.at({4, 1}).ranges, 0);
    HOJY_CHECK_EQ(cells.at({0, 1}).ranges, 6);
}

void testCastRangeAreaIgnoresOccupancyForTerrainDistance() {
    hojy::battle::SelectableCells cells;
    hojy::battle::getCastRangeArea(
        5, 1, {4, 0}, 4, cells,
        [](int, int) { return false; },
        [](int x, int) { return x == 2; });

    HOJY_CHECK_EQ(cells.count({2, 0}), 0U);
    HOJY_CHECK_EQ(cells.at({1, 0}).ranges, 3);
    HOJY_CHECK_EQ(cells.at({0, 0}).ranges, 4);
}

void testSupportPositionStaysPutWhenTargetIsAlreadyInRange() {
    const auto movement = makeMovementCells(7, {0, 0}, {2, 0}, 3);
    const auto castRange = makeCastRangeCells(7, {2, 0}, 2);

    const auto position = hojy::battle::chooseSupportPosition(
        movement, castRange);

    HOJY_CHECK_EQ(position.has_value(), true);
    HOJY_CHECK_EQ(position->first, 0);
    HOJY_CHECK_EQ(position->second, 0);
}

void testSupportPositionUsesMinimumMovementNeededToEnterRange() {
    const auto movement = makeMovementCells(7, {0, 0}, {4, 0}, 4);
    const auto castRange = makeCastRangeCells(7, {4, 0}, 2);

    const auto position = hojy::battle::chooseSupportPosition(
        movement, castRange);

    HOJY_CHECK_EQ(position.has_value(), true);
    HOJY_CHECK_EQ(position->first, 2);
    HOJY_CHECK_EQ(position->second, 0);
}

void testSupportPositionReturnsEmptyWhenRangeCannotBeReached() {
    const auto movement = makeMovementCells(7, {0, 0}, {5, 0}, 2);
    const auto castRange = makeCastRangeCells(7, {5, 0}, 2);

    const auto position = hojy::battle::chooseSupportPosition(
        movement, castRange);

    HOJY_CHECK_EQ(position.has_value(), false);
}

void testSupportPositionDoesNotCastThroughBlockedCells() {
    const std::set<Cell> blocked{{3, 0}};
    const auto movement = makeMovementCells(7, {0, 0}, {4, 0}, 2, blocked);
    const auto castRange = makeCastRangeCells(7, {4, 0}, 2, blocked);

    const auto position = hojy::battle::chooseSupportPosition(
        movement, castRange);

    HOJY_CHECK_EQ(position.has_value(), false);
}

void testApproachPositionStaysPutWhenAlreadyAdjacent() {
    const auto movement = makeMovementCells(7, {0, 0}, {1, 0}, 3);

    const auto position = hojy::battle::chooseApproachPosition(
        movement, {1, 0});

    HOJY_CHECK_EQ(position.has_value(), true);
    const Cell expected{0, 0};
    HOJY_CHECK_EQ(*position, expected);
}

void testApproachPositionChoosesNearestReachableCell() {
    const auto movement = makeMovementCells(7, {0, 0}, {5, 0}, 3);

    const auto position = hojy::battle::chooseApproachPosition(
        movement, {5, 0});

    HOJY_CHECK_EQ(position.has_value(), true);
    const Cell expected{3, 0};
    HOJY_CHECK_EQ(*position, expected);
}

void testApproachPositionStopsAtBlockedFrontier() {
    const std::set<Cell> blocked{{2, 0}};
    const auto movement = makeMovementCells(7, {0, 0}, {5, 0}, 3, blocked);

    const auto position = hojy::battle::chooseApproachPosition(
        movement, {5, 0});

    HOJY_CHECK_EQ(position.has_value(), true);
    const Cell expected{1, 0};
    HOJY_CHECK_EQ(*position, expected);
}

void testApproachPositionReturnsEmptyForNoMovementCells() {
    const hojy::battle::SelectableCells movement;

    const auto position = hojy::battle::chooseApproachPosition(
        movement, {2, 0});

    HOJY_CHECK_EQ(position.has_value(), false);
}

void testApproachPositionCanRejectManhattanDeadEnds() {
    hojy::battle::SelectableCells cells;
    cells[{0, 0}].x = 0; cells[{0, 0}].y = 0; cells[{0, 0}].moves = 0;
    cells[{1, 0}].x = 1; cells[{1, 0}].y = 0; cells[{1, 0}].moves = 1;
    cells[{2, 0}].x = 2; cells[{2, 0}].y = 0; cells[{2, 0}].moves = 2;

    const auto position = hojy::battle::chooseApproachPosition(
        cells, {4, 0}, [](Cell from, Cell) {
            if (from == Cell{1, 0}) { return -1; }
            return from == Cell{0, 0} ? 9 : 4;
        });

    HOJY_CHECK_EQ(position.has_value(), true);
    HOJY_CHECK_EQ(*position, (Cell{2, 0}));
}

void testAlignedSkillPositionPrefersMaximumStandoffBeforeMovementCost() {
    hojy::battle::SelectableCells cells;
    cells[{0, 0}] = hojy::battle::SelectableCell{0, 0, 0};
    cells[{1, 0}] = hojy::battle::SelectableCell{1, 0, 5};
    cells[{2, 0}] = hojy::battle::SelectableCell{2, 0, 1};

    const auto position = hojy::battle::chooseAlignedSkillPosition(
        cells, {0, 0}, {4, 0}, 3);
    HOJY_CHECK_EQ(position.has_value(), true);
    HOJY_CHECK_EQ(*position, (Cell{1, 0}));
}

void testAlignedSkillPositionUsesReachablePathDistance() {
    hojy::battle::SelectableCells cells;
    cells[{0, 0}] = hojy::battle::SelectableCell{0, 0, 0};
    cells[{1, 0}] = hojy::battle::SelectableCell{1, 0, 1};
    cells[{2, 0}] = hojy::battle::SelectableCell{2, 0, 2};

    const auto position = hojy::battle::chooseAlignedSkillPosition(
        cells, {0, 0}, {4, 0}, 3,
        [](Cell from, Cell target) {
            if (from == Cell{1, 0}) { return 4; }
            return std::abs(from.first - target.first)
                 + std::abs(from.second - target.second);
        });
    HOJY_CHECK_EQ(position.has_value(), true);
    HOJY_CHECK_EQ(*position, (Cell{2, 0}));
}

void testCastLegalityUsesShapeAfterTerrainDistance() {
    HOJY_CHECK_EQ(
        hojy::battle::canCastFromPosition(0, 3, 3, {0, 0}, {2, 1}), true);
    HOJY_CHECK_EQ(
        hojy::battle::canCastFromPosition(1, 3, 3, {0, 0}, {2, 1}), false);
    HOJY_CHECK_EQ(
        hojy::battle::canCastFromPosition(2, 3, 2, {0, 0}, {2, 0}), true);
    HOJY_CHECK_EQ(
        hojy::battle::canCastFromPosition(3, 2, 3, {0, 0}, {3, 0}), false);
}

void testCastMovementKeepsOriginWhenTerrainLegalityAlreadyPasses() {
    const auto movement = makeMovementCells(7, {0, 0}, {4, 0}, 2);
    const hojy::battle::SelectableCells blockedCastRange;

    const auto choice = hojy::battle::chooseCastMovementPosition(
        movement, blockedCastRange, {0, 0}, {4, 0}, 4,
        hojy::battle::CastMovementMode::Approach, true);

    HOJY_CHECK_EQ(choice.position.has_value(), true);
    HOJY_CHECK_EQ(*choice.position, (Cell{0, 0}));
    HOJY_CHECK_EQ(choice.expectedInRange, true);
}

void testCastMovementAdvancesWhenRangeCannotBeReachedThisTurn() {
    const auto movement = makeMovementCells(7, {0, 0}, {5, 0}, 2);
    const auto castRange = makeCastRangeCells(7, {5, 0}, 1);

    const auto choice = hojy::battle::chooseCastMovementPosition(
        movement, castRange, {0, 0}, {5, 0}, 1,
        hojy::battle::CastMovementMode::Approach, false);

    HOJY_CHECK_EQ(choice.position.has_value(), true);
    HOJY_CHECK_EQ(*choice.position, (Cell{2, 0}));
    HOJY_CHECK_EQ(choice.expectedInRange, false);
}

void testCastMovementReportsNoPositionWhenNoProgressIsPossible() {
    hojy::battle::SelectableCells movement;
    movement[{0, 0}] = hojy::battle::SelectableCell{0, 0, 0};
    const hojy::battle::SelectableCells castRange;

    const auto choice = hojy::battle::chooseCastMovementPosition(
        movement, castRange, {0, 0}, {4, 0}, 1,
        hojy::battle::CastMovementMode::Approach, false);

    HOJY_CHECK_EQ(choice.position.has_value(), false);
    HOJY_CHECK_EQ(choice.expectedInRange, false);
}

void testPoisonRepositionPrefersMaximumReachableRange() {
    const auto movement = makeMovementCells(7, {2, 0}, {4, 0}, 2);
    const auto castRange = makeCastRangeCells(7, {4, 0}, 3);

    const auto choice = hojy::battle::chooseCastMovementPosition(
        movement, castRange, {2, 0}, {4, 0}, 3,
        hojy::battle::CastMovementMode::Reposition, true);

    HOJY_CHECK_EQ(choice.position.has_value(), true);
    HOJY_CHECK_EQ(*choice.position, (Cell{1, 0}));
    HOJY_CHECK_EQ(choice.expectedInRange, true);
}

void testPoisonRepositionKeepsOriginAtMaximumRange() {
    const auto movement = makeMovementCells(7, {1, 0}, {4, 0}, 2);
    const auto castRange = makeCastRangeCells(7, {4, 0}, 3);

    const auto choice = hojy::battle::chooseCastMovementPosition(
        movement, castRange, {1, 0}, {4, 0}, 3,
        hojy::battle::CastMovementMode::Reposition, true);

    HOJY_CHECK_EQ(choice.position.has_value(), true);
    HOJY_CHECK_EQ(*choice.position, (Cell{1, 0}));
    HOJY_CHECK_EQ(choice.expectedInRange, true);
}

void testShortestPathDistanceHonorsBlockedCellsAndTargetOccupancy() {
    const std::set<Cell> blocked{{2, 0}, {2, 1}};
    const auto distance = hojy::battle::shortestPathDistance(
        5, 3, {0, 1}, {4, 1},
        [&](int x, int y) { return blocked.count({x, y}) != 0; },
        [](int x, int y) { return x == 4 && y == 1; });
    HOJY_CHECK_EQ(distance, 6);

    const std::set<Cell> sealedWall{{2, 0}, {2, 1}, {2, 2}};
    const auto unreachable = hojy::battle::shortestPathDistance(
        5, 3, {0, 1}, {4, 1},
        [&](int x, int y) { return sealedWall.count({x, y}) != 0; },
        [](int x, int y) { return x == 4 && y == 1; });
    HOJY_CHECK_EQ(unreachable, -1);
}

void testTerrainPathDistanceIgnoresCharacterOccupancy() {
    const std::set<Cell> blocked{{2, 0}};
    const std::set<Cell> occupied{{2, 1}};
    const auto occupancyAware = hojy::battle::shortestPathDistance(
        5, 3, {0, 1}, {4, 1},
        [&](int x, int y) { return blocked.count({x, y}) != 0; },
        [&](int x, int y) { return occupied.count({x, y}) != 0; });
    const auto terrainOnly = hojy::battle::terrainPathDistance(
        5, 3, {0, 1}, {4, 1},
        [&](int x, int y) { return blocked.count({x, y}) != 0; });
    HOJY_CHECK_EQ(occupancyAware, 6);
    HOJY_CHECK_EQ(terrainOnly, 4);
}

void testMovementAndDeathPredicates() {
    HOJY_CHECK_EQ(hojy::battle::hasMoved(4, 4), false);
    HOJY_CHECK_EQ(hojy::battle::hasMoved(4, 3), true);
    HOJY_CHECK_EQ(hojy::battle::hasMoved(0, 0), false);

    HOJY_CHECK_EQ(hojy::battle::shouldClearDeadPosition(0, 0, 3), true);
    HOJY_CHECK_EQ(hojy::battle::shouldClearDeadPosition(-1, 4, 3), true);
    HOJY_CHECK_EQ(hojy::battle::shouldClearDeadPosition(0, -1, 3), false);
    HOJY_CHECK_EQ(hojy::battle::shouldClearDeadPosition(0, 4, -1), false);
    HOJY_CHECK_EQ(hojy::battle::shouldClearDeadPosition(1, 0, 3), false);
}

void testMovementContinuationKeepsRequestingActorAlive() {
    HOJY_CHECK_EQ(
        hojy::battle::shouldContinueAfterMovement(false, false, true), true);
    HOJY_CHECK_EQ(
        hojy::battle::shouldContinueAfterMovement(false, true, false), true);
    HOJY_CHECK_EQ(
        hojy::battle::shouldContinueAfterMovement(false, false, false), false);
}

void testPlayerMovementReturnsToTheSameTurn() {
    HOJY_CHECK_EQ(
        hojy::battle::shouldContinueAfterMovement(true, false, false), true);
}

}

int main() {
    try {
        testSelectableAreaTracksMovementAndRangeParents();
        testSelectableAreaUsesOriginalDirectionPriority();
        testSelectableAreaUsesOriginalRangeDirectionPriority();
        testReachableRangeAreaHonorsCharacterBlockers();
        testCastRangeAreaIgnoresOccupancyForTerrainDistance();
        testSupportPositionStaysPutWhenTargetIsAlreadyInRange();
        testSupportPositionUsesMinimumMovementNeededToEnterRange();
        testSupportPositionReturnsEmptyWhenRangeCannotBeReached();
        testSupportPositionDoesNotCastThroughBlockedCells();
        testApproachPositionStaysPutWhenAlreadyAdjacent();
        testApproachPositionChoosesNearestReachableCell();
        testApproachPositionStopsAtBlockedFrontier();
        testApproachPositionReturnsEmptyForNoMovementCells();
        testApproachPositionCanRejectManhattanDeadEnds();
        testAlignedSkillPositionPrefersMaximumStandoffBeforeMovementCost();
        testAlignedSkillPositionUsesReachablePathDistance();
        testCastLegalityUsesShapeAfterTerrainDistance();
        testCastMovementKeepsOriginWhenTerrainLegalityAlreadyPasses();
        testCastMovementAdvancesWhenRangeCannotBeReachedThisTurn();
        testCastMovementReportsNoPositionWhenNoProgressIsPossible();
        testPoisonRepositionPrefersMaximumReachableRange();
        testPoisonRepositionKeepsOriginAtMaximumRange();
        testShortestPathDistanceHonorsBlockedCellsAndTargetOccupancy();
        testTerrainPathDistanceIgnoresCharacterOccupancy();
        testMovementAndDeathPredicates();
        testMovementContinuationKeepsRequestingActorAlive();
        testPlayerMovementReturnsToTheSameTurn();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
