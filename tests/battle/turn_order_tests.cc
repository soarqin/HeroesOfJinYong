#include "battle/turn_order.hh"
#include "test_support.hh"

#include <exception>
#include <iostream>
#include <vector>

namespace {

struct Actor {
    int id;
    int speed;
    int hurt;
    bool alive = true;
};

std::vector<Actor*> makeQueue(std::vector<Actor*> &turnOrder) {
    return hojy::battle::buildRoundQueue(turnOrder,
        [](const Actor *actor) { return actor->speed; },
        [](const Actor *actor) { return actor->alive; });
}

std::vector<int> consume(std::vector<Actor*> queue) {
    std::vector<int> ids;
    while (!queue.empty()) {
        ids.emplace_back(queue.back()->id);
        queue.pop_back();
    }
    return ids;
}

}

int main() {
    try {
        HOJY_CHECK_EQ(hojy::battle::calculateActionSpeed(29, 10, 5), 44);
        HOJY_CHECK_EQ(hojy::battle::calculateMovementSteps(78, 40), 4);
        HOJY_CHECK_EQ(hojy::battle::calculateMovementSteps(14, 99), 0);

        Actor soar{0, 29, 0};
        Actor tian{1, 78, 40};
        std::vector<Actor*> distinctOrder{&soar, &tian};
        HOJY_CHECK_EQ(consume(makeQueue(distinctOrder)), (std::vector<int>{1, 0}));
        HOJY_CHECK_EQ(consume(makeQueue(distinctOrder)), (std::vector<int>{1, 0}));

        Actor equipped{2, hojy::battle::calculateActionSpeed(29, 50, 0), 0};
        std::vector<Actor*> equippedOrder{&tian, &equipped};
        HOJY_CHECK_EQ(consume(makeQueue(equippedOrder)), (std::vector<int>{2, 1}));

        Actor wounded{3, 80, 99};
        Actor healthy{4, 79, 0};
        std::vector<Actor*> hurtOrder{&healthy, &wounded};
        HOJY_CHECK_EQ(consume(makeQueue(hurtOrder)), (std::vector<int>{3, 4}));

        Actor equalA{10, 1, 0};
        Actor equalB{11, 1, 0};
        Actor faster{12, 2, 0};
        std::vector<Actor*> equalOrder{&equalA, &equalB, &faster};
        HOJY_CHECK_EQ(consume(makeQueue(equalOrder)), (std::vector<int>{12, 11, 10}));

        tian.alive = false;
        std::vector<Actor*> deathOrder{&soar, &tian};
        HOJY_CHECK_EQ(consume(makeQueue(deathOrder)), (std::vector<int>{0}));
        soar.alive = false;
        HOJY_CHECK_EQ(makeQueue(deathOrder).size(), 0U);
        soar.alive = true;
        tian.alive = true;

        std::vector<Actor*> waitingOrder{&soar, &tian};
        auto waitingQueue = makeQueue(waitingOrder);
        auto *waitingActor = waitingQueue.back();
        waitingQueue.pop_back();
        waitingQueue.insert(waitingQueue.begin(), waitingActor);
        HOJY_CHECK_EQ(consume(waitingQueue), (std::vector<int>{0, 1}));
        HOJY_CHECK_EQ(consume(makeQueue(waitingOrder)), (std::vector<int>{1, 0}));
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
