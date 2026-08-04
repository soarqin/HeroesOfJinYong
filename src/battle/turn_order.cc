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
#include <iterator>

namespace hojy::battle {

int calculateActionSpeed(int baseSpeed, int weaponSpeed, int armourSpeed) noexcept {
    return baseSpeed + weaponSpeed + armourSpeed;
}

int calculateMovementSteps(int actionSpeed, int hurt) noexcept {
    return std::max(0, actionSpeed / 15 - hurt / 40);
}

std::vector<int> buildTurnOrder(std::vector<TurnActor> actors) {
    for (auto current = actors.begin(); current != actors.end(); ++current) {
        for (auto candidate = std::next(current); candidate != actors.end(); ++candidate) {
            if (current->speed < candidate->speed) {
                std::iter_swap(current, candidate);
            }
        }
    }
    std::vector<int> order;
    order.reserve(actors.size());
    for (auto ite = actors.rbegin(); ite != actors.rend(); ++ite) {
        order.push_back(ite->id);
    }
    return order;
}

int calcMovementSteps(int speed, int hurt) {
    return calculateMovementSteps(speed, hurt);
}

bool beginRound(bool &roundStarted) {
    const auto applyRoundEnd = roundStarted;
    roundStarted = true;
    return applyRoundEnd;
}

void resetTurnState(bool &roundStarted, std::function<void()> &pendingAction) {
    roundStarted = false;
    pendingAction = nullptr;
}

void runPendingAction(std::function<void()> &pendingAction) {
    auto action = std::move(pendingAction);
    if (action) { action(); }
}

void prepareActorActionCode(std::int16_t &actionCode, bool hasPendingAction) {
    if (!hasPendingAction) { actionCode = 0; }
}

bool shouldResumeAutoAttack(bool movementScheduled) {
    return movementScheduled;
}

std::int16_t actionCodeForSkill(std::int16_t currentActionCode, bool preserveCurrent) {
    // Support actions 4/5 and request actions 8/9 remain visible while the
    // actor continues through ordinary skill selection or fallback.
    if (currentActionCode == 4 || currentActionCode == 5
        || (preserveCurrent
            && (currentActionCode == 8 || currentActionCode == 9))) {
        return currentActionCode;
    }
    return 2;
}

}
