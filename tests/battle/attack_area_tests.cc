#include "battle/attack_area.hh"
#include "test_support.hh"

#include <iostream>
#include <utility>
#include <vector>

namespace {

using hojy::battle::AttackCell;

std::vector<std::pair<int, int>> positions(const std::vector<AttackCell> &cells) {
    std::vector<std::pair<int, int>> result;
    result.reserve(cells.size());
    for (const auto &cell: cells) {
        result.emplace_back(cell.x, cell.y);
    }
    return result;
}

void lineIsEnumeratedNearToFar() {
    const auto cells = hojy::battle::enumerateAttackCells(
        7, 1, 0, 0, 0, 0, 1, 5, 0,
        hojy::battle::AttackDirection::Right);
    HOJY_CHECK_EQ(positions(cells),
                  (std::vector<std::pair<int, int>>{{1, 0}, {2, 0}, {3, 0},
                                                     {4, 0}, {5, 0}}));
    HOJY_CHECK_EQ(cells.front().distance, 1);
    HOJY_CHECK_EQ(cells.back().distance, 5);
}

void crossUsesOriginalDirectionOrder() {
    const auto cells = hojy::battle::enumerateAttackCells(
        5, 5, 2, 2, 2, 2, 2, 2, 0,
        hojy::battle::AttackDirection::Up);
    HOJY_CHECK_EQ(positions(cells),
                  (std::vector<std::pair<int, int>>{{2, 1}, {2, 3}, {1, 2},
                                                     {3, 2}, {2, 0}, {2, 4},
                                                     {0, 2}, {4, 2}}));
}

void areaUsesActorDistanceAndColumnMajorScan() {
    const auto cells = hojy::battle::enumerateAttackCells(
        5, 5, 0, 0, 2, 2, 3, 0, 1,
        hojy::battle::AttackDirection::Down);
    HOJY_CHECK_EQ(positions(cells),
                  (std::vector<std::pair<int, int>>{{1, 1}, {1, 2}, {1, 3},
                                                     {2, 1}, {2, 2}, {2, 3},
                                                     {3, 1}, {3, 2}, {3, 3}}));
    HOJY_CHECK_EQ(cells[0].distance, 2);
    HOJY_CHECK_EQ(cells[4].distance, 4);
    HOJY_CHECK_EQ(cells.back().distance, 6);
}

void singleCellUsesActualDistanceAndSkipsOutOfBounds() {
    const auto cells = hojy::battle::enumerateAttackCells(
        3, 3, 0, 0, 9, 9, 0, 0, 0,
        hojy::battle::AttackDirection::Down);
    HOJY_CHECK_EQ(cells.size(), 0U);

    const auto inBounds = hojy::battle::enumerateAttackCells(
        3, 3, 0, 0, 2, 1, 0, 0, 0,
        hojy::battle::AttackDirection::Down);
    HOJY_CHECK_EQ(positions(inBounds),
                  (std::vector<std::pair<int, int>>{{2, 1}}));
    HOJY_CHECK_EQ(inBounds[0].distance, 3);
}

}

int main() {
    try {
        lineIsEnumeratedNearToFar();
        crossUsesOriginalDirectionOrder();
        areaUsesActorDistanceAndColumnMajorScan();
        singleCellUsesActualDistanceAndSkipsOutOfBounds();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
