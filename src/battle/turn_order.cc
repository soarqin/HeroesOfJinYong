/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "turn_order.hh"

#include <algorithm>

namespace hojy::battle {

int calculateActionSpeed(int baseSpeed, int weaponSpeed, int armourSpeed) noexcept {
    return baseSpeed + weaponSpeed + armourSpeed;
}

int calculateMovementSteps(int actionSpeed, int hurt) noexcept {
    return std::max(0, actionSpeed / 15 - hurt / 40);
}

}
