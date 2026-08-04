#include "scene/command.hh"

#include "test_support.hh"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void testGenerationDefersReentrantCommands() {
    hojy::scene::SceneCommandQueue queue;
    std::vector<int> executed;

    queue.push([&] {
        executed.push_back(1);
        queue.push([&] { executed.push_back(3); });
    });
    queue.push([&] { executed.push_back(2); });

    queue.executeGeneration();
    HOJY_CHECK_EQ(executed, std::vector<int>({1, 2}));
    HOJY_CHECK_EQ(queue.empty(), false);

    queue.executeGeneration();
    HOJY_CHECK_EQ(executed, std::vector<int>({1, 2, 3}));
    HOJY_CHECK_EQ(queue.empty(), true);
}

void testGenerationRestoresUnexecutedCommandsAfterFailure() {
    hojy::scene::SceneCommandQueue queue;
    std::vector<int> executed;

    queue.push([&] {
        executed.push_back(1);
        queue.push([&] { executed.push_back(4); });
    });
    queue.push([&] {
        executed.push_back(2);
        throw std::runtime_error("stop");
    });
    queue.push([&] { executed.push_back(3); });

    HOJY_CHECK_THROWS(std::runtime_error, queue.executeGeneration());
    HOJY_CHECK_EQ(executed, std::vector<int>({1, 2}));
    HOJY_CHECK_EQ(queue.empty(), false);

    queue.executeAllGenerations();
    HOJY_CHECK_EQ(executed, std::vector<int>({1, 2, 3, 4}));
    HOJY_CHECK_EQ(queue.empty(), true);
}

}

int main() {
    try {
        testGenerationDefersReentrantCommands();
        testGenerationRestoresUnexecutedCommandsAfterFailure();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
