#include "movement.hh"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <queue>
#include <vector>

namespace hojy::battle {

namespace {

constexpr std::array<std::pair<int, int>, 4> kOriginalDirections{{
    {0, -1}, {1, 0}, {-1, 0}, {0, 1},
}};

constexpr std::array<std::pair<int, int>, 4> kOriginalRangeDirections{{
    {-1, 0}, {0, -1}, {1, 0}, {0, 1},
}};

}

void getSelectableArea(
    int width, int height, std::pair<int, int> start, int steps, int ranges,
    SelectableCells &cells,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied,
    const std::function<bool(int, int)> &sameSide) {
    cells.clear();
    if (width <= 0 || height <= 0
        || width > std::numeric_limits<int>::max() / height
        || start.first < 0 || start.first >= width
        || start.second < 0 || start.second >= height) {
        return;
    }
    auto &origin = cells[start];
    origin.x = start.first;
    origin.y = start.second;
    origin.moves = 0;
    origin.ranges = 0;
    if (steps > 0) {
        std::queue<SelectableCell *> queue;
        queue.push(&origin);
        while (!queue.empty()) {
            auto *current = queue.front();
            queue.pop();
            bool sameSideBlock = false;
            std::array<std::pair<int, int>, 4> neighbors{};
            int count = 0;
            for (const auto [dx, dy]: kOriginalDirections) {
                const auto x = current->x + dx;
                const auto y = current->y + dy;
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    neighbors[count++] = {x, y};
                }
            }
            for (int i = 0; i < count; ++i) {
                if (sameSide(neighbors[i].first, neighbors[i].second)) {
                    sameSideBlock = true;
                    break;
                }
            }
            if (sameSideBlock) { continue; }
            for (int i = 0; i < count; ++i) {
                const auto [x, y] = neighbors[i];
                if (occupied(x, y) || blocked(x, y)) { continue; }
                if (cells.find({x, y}) != cells.end()) { continue; }
                auto &cell = cells[{x, y}];
                cell.x = x;
                cell.y = y;
                cell.moves = current->moves + 1;
                cell.moveParent = current;
                if (cell.moves < steps) { queue.push(&cell); }
            }
        }
    }
    if (ranges <= 0) { return; }
    std::queue<SelectableCell *> queue;
    for (auto &entry: cells) { queue.push(&entry.second); }
    while (!queue.empty()) {
        auto *current = queue.front();
        queue.pop();
        std::array<std::pair<int, int>, 4> neighbors{};
        int count = 0;
        for (const auto [dx, dy]: kOriginalRangeDirections) {
            const auto x = current->x + dx;
            const auto y = current->y + dy;
            if (x >= 0 && x < width && y >= 0 && y < height) {
                neighbors[count++] = {x, y};
            }
        }
        for (int i = 0; i < count; ++i) {
            const auto [x, y] = neighbors[i];
            if (blocked(x, y)) { continue; }
            if (cells.find({x, y}) != cells.end()) { continue; }
            auto &cell = cells[{x, y}];
            cell.x = x;
            cell.y = y;
            cell.moves = -1;
            cell.ranges = current->ranges + 1;
            cell.rangeParent = current;
            if (cell.ranges < ranges) { queue.push(&cell); }
        }
    }
}

void getReachableRangeArea(
    int width, int height, std::pair<int, int> target, int range,
    SelectableCells &cells,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied) {
    cells.clear();
    if (width <= 0 || height <= 0
        || width > std::numeric_limits<int>::max() / height
        || target.first < 0 || target.first >= width
        || target.second < 0 || target.second >= height
        || blocked(target.first, target.second)) {
        return;
    }
    range = std::max(0, range);
    const auto indexOf = [width](int x, int y) {
        return static_cast<std::size_t>(y * width + x);
    };
    std::vector<int> distances(static_cast<std::size_t>(width * height), -1);
    std::queue<std::pair<int, int>> queue;
    distances[indexOf(target.first, target.second)] = 0;
    queue.push(target);
    while (!queue.empty()) {
        const auto current = queue.front();
        queue.pop();
        const auto currentDistance = distances[indexOf(current.first, current.second)];
        if (currentDistance >= range) { continue; }
        for (const auto [dx, dy]: kOriginalRangeDirections) {
            const auto next = std::make_pair(current.first + dx, current.second + dy);
            if (next.first < 0 || next.first >= width
                || next.second < 0 || next.second >= height
                || blocked(next.first, next.second)
                || (next != target && occupied(next.first, next.second))) {
                continue;
            }
            auto &distance = distances[indexOf(next.first, next.second)];
            if (distance >= 0) { continue; }
            distance = currentDistance + 1;
            queue.push(next);
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto distance = distances[indexOf(x, y)];
            if (distance < 0 || distance > range
                || (std::make_pair(x, y) != target && occupied(x, y))) {
                continue;
            }
            auto &cell = cells[{x, y}];
            cell.x = x;
            cell.y = y;
            cell.moves = -1;
            cell.ranges = distance;
        }
    }
}

void getCastRangeArea(
    int width, int height, std::pair<int, int> target, int range,
    SelectableCells &cells,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied) {
    cells.clear();
    if (width <= 0 || height <= 0
        || width > std::numeric_limits<int>::max() / height
        || target.first < 0 || target.first >= width
        || target.second < 0 || target.second >= height
        || blocked(target.first, target.second)) {
        return;
    }
    range = std::max(0, range);
    const auto indexOf = [width](int x, int y) {
        return static_cast<std::size_t>(y * width + x);
    };
    std::vector<int> distances(static_cast<std::size_t>(width * height), -1);
    std::queue<std::pair<int, int>> queue;
    distances[indexOf(target.first, target.second)] = 0;
    queue.push(target);
    while (!queue.empty()) {
        const auto current = queue.front();
        queue.pop();
        const auto currentDistance = distances[indexOf(current.first, current.second)];
        if (currentDistance >= range) { continue; }
        for (const auto [dx, dy]: kOriginalRangeDirections) {
            const auto next = std::make_pair(current.first + dx, current.second + dy);
            if (next.first < 0 || next.first >= width
                || next.second < 0 || next.second >= height
                || blocked(next.first, next.second)) {
                continue;
            }
            auto &distance = distances[indexOf(next.first, next.second)];
            if (distance >= 0) { continue; }
            distance = currentDistance + 1;
            queue.push(next);
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto distance = distances[indexOf(x, y)];
            const auto position = std::make_pair(x, y);
            if (distance < 0 || distance > range
                || (position != target && occupied(x, y))) {
                continue;
            }
            auto &cell = cells[position];
            cell.x = x;
            cell.y = y;
            cell.moves = -1;
            cell.ranges = distance;
        }
    }
}

int shortestPathDistance(
    int width, int height, std::pair<int, int> start,
    std::pair<int, int> target,
    const std::function<bool(int, int)> &blocked,
    const std::function<bool(int, int)> &occupied) {
    if (width <= 0 || height <= 0
        || width > std::numeric_limits<int>::max() / height) {
        return -1;
    }
    const auto inBounds = [width, height](int x, int y) {
        return x >= 0 && x < width && y >= 0 && y < height;
    };
    if (!inBounds(start.first, start.second)
        || !inBounds(target.first, target.second)
        || blocked(target.first, target.second)) {
        return -1;
    }
    if (start == target) { return 0; }

    std::vector<int> distances(static_cast<std::size_t>(width * height), -1);
    const auto indexOf = [width](int x, int y) {
        return static_cast<std::size_t>(y * width + x);
    };
    std::queue<std::pair<int, int>> queue;
    distances[indexOf(start.first, start.second)] = 0;
    queue.push(start);
    while (!queue.empty()) {
        const auto current = queue.front();
        queue.pop();
        const auto currentDistance = distances[indexOf(current.first, current.second)];
        for (const auto [dx, dy]: kOriginalDirections) {
            const auto next = std::make_pair(current.first + dx, current.second + dy);
            if (!inBounds(next.first, next.second)
                || blocked(next.first, next.second)
                || (next != target && occupied(next.first, next.second))) {
                continue;
            }
            auto &distance = distances[indexOf(next.first, next.second)];
            if (distance >= 0) { continue; }
            distance = currentDistance + 1;
            if (next == target) { return distance; }
            queue.push(next);
        }
    }
    return -1;
}

int terrainPathDistance(
    int width, int height, std::pair<int, int> start,
    std::pair<int, int> target,
    const std::function<bool(int, int)> &blocked) {
    return shortestPathDistance(
        width, height, start, target, blocked,
        [](int, int) { return false; });
}

bool hasMoved(int initialSteps, int remainingSteps) noexcept {
    return remainingSteps != initialSteps;
}

bool shouldClearDeadPosition(int hp, int x, int y) noexcept {
    return hp <= 0 && x >= 0 && y >= 0;
}

bool shouldContinueAfterMovement(bool playerControlled,
                                 bool hasPendingAction,
                                 bool resumeAutoAttack) noexcept {
    return playerControlled || hasPendingAction || resumeAutoAttack;
}

bool canCastFromPosition(int attackAreaType, int range, int terrainDistance,
                         std::pair<int, int> actor,
                         std::pair<int, int> target) noexcept {
    range = std::max(0, range);
    if (terrainDistance < 0 || terrainDistance > range) { return false; }
    switch (attackAreaType) {
    case 0:
    case 3:
        return true;
    case 1:
    case 2:
        return actor.first == target.first || actor.second == target.second;
    default:
        return false;
    }
}

namespace {

std::optional<std::pair<int, int>> chooseMaximumCastPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells,
    std::pair<int, int> origin) {
    std::optional<std::pair<int, int>> selected;
    int selectedRange = -1;
    int selectedDistance = std::numeric_limits<int>::max();
    for (const auto &[position, cell]: movementCells) {
        if (cell.moves < 0) { continue; }
        const auto range = castRangeCells.find(position);
        if (range == castRangeCells.end()) { continue; }
        const auto castDistance = range->second.ranges;
        const auto distance = std::abs(position.first - origin.first)
            + std::abs(position.second - origin.second);
        if (!selected || castDistance > selectedRange
            || (castDistance == selectedRange && distance < selectedDistance)) {
            selected = position;
            selectedRange = castDistance;
            selectedDistance = distance;
        }
    }
    return selected;
}

}

std::optional<std::pair<int, int>> chooseSupportPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells) {
    if (movementCells.empty()) { return std::nullopt; }
    std::pair<int, int> origin{};
    bool haveOrigin = false;
    for (const auto &[position, cell]: movementCells) {
        if (cell.moves == 0) {
            origin = position;
            haveOrigin = true;
            break;
        }
    }
    if (!haveOrigin) { return std::nullopt; }
    if (castRangeCells.find(origin) != castRangeCells.end()) {
        return origin;
    }
    return chooseMaximumCastPosition(movementCells, castRangeCells, origin);
}

std::optional<std::pair<int, int>> chooseApproachPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> target) {
    return chooseApproachPosition(
        movementCells, target,
        [](std::pair<int, int> from, std::pair<int, int> destination) {
            return std::abs(from.first - destination.first)
                 + std::abs(from.second - destination.second);
        });
}

std::optional<std::pair<int, int>> chooseApproachPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> target,
    const PositionDistance &distance) {
    std::optional<std::pair<int, int>> selected;
    auto selectedDistance = std::numeric_limits<int>::max();
    auto selectedMoves = std::numeric_limits<int>::max();
    for (const auto &[position, cell]: movementCells) {
        if (cell.moves < 0) { continue; }
        const auto candidateDistance = distance ? distance(position, target) : -1;
        if (candidateDistance < 0) { continue; }
        if (!selected || candidateDistance < selectedDistance
            || (candidateDistance == selectedDistance && cell.moves < selectedMoves)) {
            selected = position;
            selectedDistance = candidateDistance;
            selectedMoves = cell.moves;
        }
    }
    return selected;
}

std::optional<std::pair<int, int>> chooseAlignedSkillPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range) {
    range = std::max(0, range);
    const auto aligned = [target](std::pair<int, int> position) {
        return position.first == target.first || position.second == target.second;
    };
    const auto actorRange = castRangeCells.find(actor);
    if (aligned(actor) && actorRange != castRangeCells.end()
        && actorRange->second.ranges > 0
        && actorRange->second.ranges <= range) {
        return actor;
    }
    for (int wanted = range; wanted >= 1; --wanted) {
        std::optional<std::pair<int, int>> selected;
        int selectedCost = std::numeric_limits<int>::max();
        for (const auto &[position, cell]: movementCells) {
            if (cell.moves < 0 || !aligned(position)) { continue; }
            const auto rangeCell = castRangeCells.find(position);
            if (rangeCell == castRangeCells.end()
                || rangeCell->second.ranges != wanted) {
                continue;
            }
            const auto cost = std::abs(position.first - actor.first)
                + std::abs(position.second - actor.second);
            if (!selected || cost < selectedCost) {
                selected = position;
                selectedCost = cost;
            }
        }
        if (selected) { return selected; }
    }
    return std::nullopt;
}

std::optional<std::pair<int, int>> chooseAlignedSkillPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range) {
    return chooseAlignedSkillPosition(
        movementCells, actor, target, range,
        [target](std::pair<int, int> position, std::pair<int, int>) {
            return std::abs(position.first - target.first)
                 + std::abs(position.second - target.second);
        });
}

std::optional<std::pair<int, int>> chooseAlignedSkillPosition(
    const SelectableCells &movementCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range,
    const PositionDistance &distance) {
    range = std::max(0, range);
    const auto aligned = [target](std::pair<int, int> position) {
        return position.first == target.first || position.second == target.second;
    };
    const auto actorDistance = distance ? distance(actor, target) : -1;
    if (aligned(actor) && actorDistance > 0 && actorDistance <= range) {
        return actor;
    }
    for (int wanted = range; wanted >= 1; --wanted) {
        std::optional<std::pair<int, int>> selected;
        int selectedCost = std::numeric_limits<int>::max();
        for (const auto &[position, cell]: movementCells) {
            if (cell.moves < 0 || !aligned(position)) {
                continue;
            }
            const auto candidateDistance = distance ? distance(position, target) : -1;
            if (candidateDistance != wanted) {
                continue;
            }
            const auto cost = std::abs(position.first - actor.first)
                + std::abs(position.second - actor.second);
            if (!selected || cost < selectedCost) {
                selected = position;
                selectedCost = cost;
            }
        }
        if (selected) { return selected; }
    }
    return std::nullopt;
}

CastMovementChoice chooseCastMovementPosition(
    const SelectableCells &movementCells,
    const SelectableCells &castRangeCells,
    std::pair<int, int> actor,
    std::pair<int, int> target,
    int range,
    CastMovementMode mode,
    bool canCastNow,
    const PositionDistance &approachDistance) {
    if (mode != CastMovementMode::Reposition && canCastNow) {
        return CastMovementChoice{actor, true};
    }

    std::optional<std::pair<int, int>> position;
    if (mode == CastMovementMode::Aligned) {
        position = chooseAlignedSkillPosition(
            movementCells, castRangeCells, actor, target, range);
    } else {
        position = chooseMaximumCastPosition(
            movementCells, castRangeCells, actor);
    }
    if (position) {
        return CastMovementChoice{position, true};
    }

    position = approachDistance
        ? chooseApproachPosition(movementCells, target, approachDistance)
        : chooseApproachPosition(movementCells, target);
    if (position && *position == actor && !canCastNow) {
        /* The origin is always present in a movement map.  Returning it as a
         * failed approach would make callers run a fake movement callback and
         * skip their no-progress fallback branch. */
        position.reset();
    }
    return CastMovementChoice{position, false};
}

}
