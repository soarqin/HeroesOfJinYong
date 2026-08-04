#include "battle/turn_order.hh"
#include "test_support.hh"

#include <functional>
#include <iostream>

namespace {

struct QueueActor {
    int id;
    int speed;
    bool alive;
};

std::vector<int> consume(std::vector<QueueActor *> queue) {
    std::vector<int> ids;
    while (!queue.empty()) {
        ids.push_back(queue.back()->id);
        queue.pop_back();
    }
    return ids;
}

}

int main() {
    try {
        const auto order = hojy::battle::buildTurnOrder({
            {10, 0}, {20, 1}, {20, 2}, {5, 3}});
        HOJY_CHECK_EQ(order.size(), 4U);
        HOJY_CHECK_EQ(order[0], 3);
        HOJY_CHECK_EQ(order[1], 0);
        HOJY_CHECK_EQ(order[2], 2);
        HOJY_CHECK_EQ(order[3], 1);

        const auto unsorted = hojy::battle::buildTurnOrder({
            {5, 0}, {90, 1}, {10, 2}, {1, 3}});
        HOJY_CHECK_EQ(unsorted[0], 3);
        HOJY_CHECK_EQ(unsorted[1], 0);
        HOJY_CHECK_EQ(unsorted[2], 2);
        HOJY_CHECK_EQ(unsorted[3], 1);

        HOJY_CHECK_EQ(hojy::battle::calcMovementSteps(60, 0), 4);
        HOJY_CHECK_EQ(hojy::battle::calcMovementSteps(60, 80), 2);
        HOJY_CHECK_EQ(hojy::battle::calcMovementSteps(10, 80), 0);

        bool roundStarted = false;
        HOJY_CHECK_EQ(hojy::battle::beginRound(roundStarted), false);
        HOJY_CHECK_EQ(roundStarted, true);
        HOJY_CHECK_EQ(hojy::battle::beginRound(roundStarted), true);

        std::function<void()> pendingAction = [] {};
        hojy::battle::resetTurnState(roundStarted, pendingAction);
        HOJY_CHECK_EQ(roundStarted, false);
        HOJY_CHECK_EQ(static_cast<bool>(pendingAction), false);
        HOJY_CHECK_EQ(hojy::battle::beginRound(roundStarted), false);

        int pendingCalls = 0;
        pendingAction = [&]() {
            ++pendingCalls;
            pendingAction = [&]() { ++pendingCalls; };
        };
        hojy::battle::runPendingAction(pendingAction);
        HOJY_CHECK_EQ(pendingCalls, 1);
        HOJY_CHECK_EQ(static_cast<bool>(pendingAction), true);
        hojy::battle::runPendingAction(pendingAction);
        HOJY_CHECK_EQ(pendingCalls, 2);
        HOJY_CHECK_EQ(static_cast<bool>(pendingAction), false);

        std::int16_t actionCode = 8;
        hojy::battle::prepareActorActionCode(actionCode, true);
        HOJY_CHECK_EQ(actionCode, 8);
        hojy::battle::prepareActorActionCode(actionCode, false);
        HOJY_CHECK_EQ(actionCode, 0);

        HOJY_CHECK_EQ(hojy::battle::shouldResumeAutoAttack(true), true);
        HOJY_CHECK_EQ(hojy::battle::shouldResumeAutoAttack(false), false);

        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(8, true), 8);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(9, true), 9);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(4, true), 4);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(5, true), 5);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(4, false), 4);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(5, false), 5);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(0, true), 2);
        HOJY_CHECK_EQ(hojy::battle::actionCodeForSkill(8, false), 2);

        QueueActor liveA{0, 0, true};
        QueueActor liveB{1, 0, true};
        QueueActor deadFast{2, 1, false};
        std::vector<QueueActor *> allActors{&liveA, &liveB, &deadFast};
        auto queue = hojy::battle::buildRoundQueue(
            allActors,
            [](const QueueActor *actor) { return actor->speed; },
            [](const QueueActor *actor) { return actor->alive; });
        HOJY_CHECK_EQ(consume(std::move(queue)), (std::vector<int>{1, 0}));
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
