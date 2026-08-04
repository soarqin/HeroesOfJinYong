/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <vector>

namespace hojy::battle {

struct TurnActor {
    int speed = 0;
    int id = -1;
};

int calculateActionSpeed(int baseSpeed, int weaponSpeed = 0, int armourSpeed = 0) noexcept;
int calculateMovementSteps(int actionSpeed, int hurt) noexcept;
std::vector<int> buildTurnOrder(std::vector<TurnActor> actors);
int calcMovementSteps(int speed, int hurt);

/*
 * The original game compares every later actor with the actor in the
 * current slot and swaps immediately when the later actor is faster.
 * This is intentionally not std::stable_sort: immediate swaps also define
 * the original game's ordering for actors with equal speed.
 */
template<typename Iterator, typename SpeedFunc>
void sortActionOrder(Iterator first, Iterator last, SpeedFunc speed) {
    for (auto current = first; current != last; ++current) {
        auto candidate = current;
        for (++candidate; candidate != last; ++candidate) {
            if (speed(*current) < speed(*candidate)) {
                std::iter_swap(current, candidate);
            }
        }
    }
}

/* Sort every original slot first, then filter inactive actors while reversing
 * the result for Warfield's back()-consumed queue. */
template<typename Actor, typename SpeedFunc, typename AliveFunc>
std::vector<Actor> buildRoundQueue(std::vector<Actor> &turnOrder,
                                   SpeedFunc speed, AliveFunc alive) {
    sortActionOrder(turnOrder.begin(), turnOrder.end(), speed);
    std::vector<Actor> queue;
    queue.reserve(turnOrder.size());
    for (auto ite = turnOrder.rbegin(); ite != turnOrder.rend(); ++ite) {
        if (alive(*ite)) { queue.emplace_back(*ite); }
    }
    return queue;
}
bool beginRound(bool &roundStarted);
void resetTurnState(bool &roundStarted, std::function<void()> &pendingAction);
void runPendingAction(std::function<void()> &pendingAction);
void prepareActorActionCode(std::int16_t &actionCode, bool hasPendingAction);
bool shouldResumeAutoAttack(bool movementScheduled);
std::int16_t actionCodeForSkill(std::int16_t currentActionCode, bool preserveCurrent);

}
