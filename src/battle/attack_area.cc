#include "attack_area.hh"

#include <algorithm>
#include <cstdlib>

namespace hojy::battle {

namespace {

bool inBounds(int width, int height, int x, int y) {
    return width > 0 && height > 0 && x >= 0 && x < width && y >= 0 && y < height;
}

void appendIfInBounds(std::vector<AttackCell> &cells,
                      int width, int height,
                      int x, int y, int distance) {
    if (inBounds(width, height, x, y)) {
        cells.push_back(AttackCell{x, y, distance});
    }
}

}

std::vector<AttackCell> enumerateAttackCells(
    int width, int height,
    int originX, int originY,
    int cursorX, int cursorY,
    int areaType,
    int range,
    int areaRadius,
    AttackDirection direction) {
    std::vector<AttackCell> cells;
    if (width <= 0 || height <= 0) { return cells; }

    range = std::max(0, range);
    areaRadius = std::max(0, areaRadius);
    switch (areaType) {
    case 1: {
        for (int distance = 1; distance <= range; ++distance) {
            int x = originX;
            int y = originY;
            switch (direction) {
            case AttackDirection::Up: y -= distance; break;
            case AttackDirection::Right: x += distance; break;
            case AttackDirection::Down: y += distance; break;
            case AttackDirection::Left: x -= distance; break;
            }
            appendIfInBounds(cells, width, height, x, y, distance);
        }
        break;
    }
    case 2:
        for (int distance = 1; distance <= range; ++distance) {
            appendIfInBounds(cells, width, height,
                             originX, originY - distance, distance);
            appendIfInBounds(cells, width, height,
                             originX, originY + distance, distance);
            appendIfInBounds(cells, width, height,
                             originX - distance, originY, distance);
            appendIfInBounds(cells, width, height,
                             originX + distance, originY, distance);
        }
        break;
    case 3:
        for (int dx = -areaRadius; dx <= areaRadius; ++dx) {
            const auto x = cursorX + dx;
            for (int dy = -areaRadius; dy <= areaRadius; ++dy) {
                const auto y = cursorY + dy;
                appendIfInBounds(cells, width, height, x, y,
                                 std::abs(x - originX) + std::abs(y - originY));
            }
        }
        break;
    default:
        appendIfInBounds(cells, width, height, cursorX, cursorY,
                         std::abs(cursorX - originX) + std::abs(cursorY - originY));
        break;
    }
    return cells;
}

}
