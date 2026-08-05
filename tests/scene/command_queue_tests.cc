#include "scene/logic/command.hh"

#include "test_support.hh"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void testCommandsAddedDuringExecutionBecomeNextGeneration() {
    hojy::scene::SceneCommandQueue queue;
    hojy::scene::SceneCommandContext context;
    std::vector<int> order;
    queue.push([&](hojy::scene::SceneCommandContext &) {
        order.push_back(1);
        queue.push([&](hojy::scene::SceneCommandContext &) { order.push_back(3); });
    });
    queue.push([&](hojy::scene::SceneCommandContext &) { order.push_back(2); });

    queue.executeGeneration(context);
    HOJY_CHECK_EQ(order.size(), 2U);
    HOJY_CHECK_EQ(order.at(0), 1);
    HOJY_CHECK_EQ(order.at(1), 2);
    HOJY_CHECK_EQ(queue.size(), 1U);

    queue.executeGeneration(context);
    HOJY_CHECK_EQ(order.size(), 3U);
    HOJY_CHECK_EQ(order.at(2), 3);
    HOJY_CHECK_EQ(queue.empty(), true);
}

void testFailedGenerationKeepsUnexecutedCommands() {
    hojy::scene::SceneCommandQueue queue;
    hojy::scene::SceneCommandContext context;
    int completed = 0;
    queue.push([&](hojy::scene::SceneCommandContext &) { throw std::runtime_error("expected"); });
    queue.push([&](hojy::scene::SceneCommandContext &) { ++completed; });

    HOJY_CHECK_THROWS(std::runtime_error, queue.executeGeneration(context));
    HOJY_CHECK_EQ(completed, 0);
    HOJY_CHECK_EQ(queue.size(), 2U);
}

void testTransactionCheckpointDiscardsOnlyAppendedCommands() {
    hojy::scene::SceneCommandQueue queue;
    hojy::scene::SceneCommandContext context;
    std::vector<int> order;
    queue.push([&](hojy::scene::SceneCommandContext &) { order.push_back(1); });
    const auto checkpoint = queue.size();
    queue.push([&](hojy::scene::SceneCommandContext &) { order.push_back(2); });
    queue.push([&](hojy::scene::SceneCommandContext &) { order.push_back(3); });

    queue.discardAfter(checkpoint);
    HOJY_CHECK_EQ(queue.size(), checkpoint);
    queue.executeGeneration(context);
    HOJY_CHECK_EQ(order.size(), std::size_t(1));
    HOJY_CHECK_EQ(order.front(), 1);
}

}

int main() {
    try {
        testCommandsAddedDuringExecutionBecomeNextGeneration();
        testFailedGenerationKeepsUnexecutedCommands();
        testTransactionCheckpointDiscardsOnlyAppendedCommands();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
