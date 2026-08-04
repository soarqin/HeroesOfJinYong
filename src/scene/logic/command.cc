#include "command.hh"

namespace hojy::scene {

void SceneCommandQueue::push(std::unique_ptr<SceneCommand> command) {
    if (command) { commands_.push_back(std::move(command)); }
}

void SceneCommandQueue::push(std::function<void()> function) {
    if (function) {
        push(std::make_unique<FunctionSceneCommand>(std::move(function)));
    }
}

void SceneCommandQueue::executeGeneration() {
    if (commands_.empty()) { return; }

    auto generation = std::move(commands_);
    commands_.clear();

    std::size_t next = 0;
    try {
        for (; next < generation.size();) {
            auto command = std::move(generation[next]);
            ++next;
            command->execute();
        }
    } catch (...) {
        std::deque<std::unique_ptr<SceneCommand>> pending;
        for (; next < generation.size(); ++next) {
            pending.push_back(std::move(generation[next]));
        }
        while (!commands_.empty()) {
            pending.push_back(std::move(commands_.front()));
            commands_.pop_front();
        }
        commands_ = std::move(pending);
        throw;
    }
}

void SceneCommandQueue::executeAllGenerations() {
    while (!commands_.empty()) {
        executeGeneration();
    }
}

}
