#pragma once

#include <vector>

namespace hojy::battle {

enum class AttackDirection {
    Up,
    Right,
    Down,
    Left,
};

struct AttackCell {
    int x = 0;
    int y = 0;
    int distance = 0;
};

/*
 * Enumerate the cells touched by a skill in the same order as the DOS
 * routine.  The actor position is (originX, originY); cursorX/cursorY is the
 * selected target cell for single-cell and area skills.
 */
std::vector<AttackCell> enumerateAttackCells(
    int width, int height,
    int originX, int originY,
    int cursorX, int cursorY,
    int areaType,
    int range,
    int areaRadius,
    AttackDirection direction);

}
