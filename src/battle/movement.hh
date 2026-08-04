#pragma once

#include <functional>
#include <map>
#include <optional>
#include <utility>

namespace hojy::battle {

struct SelectableCell {
    int x = 0;
    int y = 0;
    int moves = -1;
    int ranges = 0;
    SelectableCell *moveParent = nullptr;
    SelectableCell *rangeParent = nullptr;
};

using SelectableCells = std::map<std::pair<int, int>, SelectableCell>;
using PositionDistance = std::function<int(
    std::pair<int, int> from, std::pair<int, int> target)>;

enum class CastMovementMode {
    Approach,
    Aligned,
    Reposition,
};

struct CastMovementChoice {
    std::optional<std::pair<int, int>> position;
    bool expectedInRange = false;
};

void getSelectableArea(
    int width, int height, std::pair<int, int> start, int steps, int ranges,
    SelectableCells &cells,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied,
    const std::function<bool(int, int)> &sameSide);

/* Build terrain- and occupancy-aware cast positions around an occupied target. */
void getReachableRangeArea(
    int width, int height, std::pair<int, int> target, int range,
    SelectableCells &cells,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied);

/* Build a terrain-only range map while excluding occupied result tiles. */
void getCastRangeArea(
    int width, int height, std::pair<int, int> target, int range,
    SelectableCells &cells,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied);

// Computes the obstacle-aware map distance used by original target scans.
// Occupied cells remain impassable, except for the requested target cell so
// callers can measure the distance to an enemy's occupied tile.
int shortestPathDistance(
    int width, int height, std::pair<int, int> start,
    std::pair<int, int> target,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied);

// Computes the terrain-only distance used by original target scoring scans.
// Character occupancy is deliberately ignored; execution paths use
// shortestPathDistance() when occupied cells must remain impassable.
int terrainPathDistance(
    int width, int height, std::pair<int, int> start,
    std::pair<int, int> target,
    const std::function<bool(int, int)> &blocked);

bool hasMoved(int initialSteps, int remainingSteps) noexcept;
bool shouldClearDeadPosition(int hp, int x, int y) noexcept;
bool shouldContinueAfterMovement(bool playerControlled,
                                 bool hasPendingAction,
                                 bool resumeAutoAttack) noexcept;
bool canCastFromPosition(int attackAreaType, int range, int terrainDistance,
                         std::pair<int, int> actor,
                         std::pair<int, int> target) noexcept;

std::optional<std::pair<int, int>> chooseSupportPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells);

std::optional<std::pair<int, int>> chooseApproachPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> target);
std::optional<std::pair<int, int>> chooseApproachPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> target,
    const PositionDistance &distance);

/* Choose an aligned line/cross casting tile, preserving maximum standoff. */
std::optional<std::pair<int, int>> chooseAlignedSkillPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range);
std::optional<std::pair<int, int>> chooseAlignedSkillPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range);
std::optional<std::pair<int, int>> chooseAlignedSkillPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range,
    const PositionDistance &distance);

CastMovementChoice chooseCastMovementPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range,
    CastMovementMode mode,
    bool canCastNow,
    const PositionDistance &approachDistance = {});

}
