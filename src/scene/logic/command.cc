#include "command.hh"

namespace hojy::scene {

void SceneCommandQueue::push(std::unique_ptr<SceneCommand> command) {
    if (command) {
        commands_.push_back(std::move(command));
    }
}

void SceneCommandQueue::push(std::function<void(SceneCommandContext &)> function) {
    if (function) {
        push(std::make_unique<FunctionSceneCommand>(std::move(function)));
    }
}

void SceneCommandQueue::executeGeneration(SceneCommandContext &context) {
    if (commands_.empty()) {
        return;
    }

    auto generation = std::move(commands_);
    commands_.clear();
    std::size_t index = 0;
    try {
        for (; index < generation.size(); ++index) {
            if (generation[index]) {
                generation[index]->execute(context);
            }
        }
    } catch (...) {
        std::deque<std::unique_ptr<SceneCommand>> pending;
        for (std::size_t i = index; i < generation.size(); ++i) {
            if (generation[i]) {
                pending.push_back(std::move(generation[i]));
            }
        }
        while (!commands_.empty()) {
            pending.push_back(std::move(commands_.front()));
            commands_.pop_front();
        }
        commands_ = std::move(pending);
        throw;
    }
}

void SceneCommandQueue::discardAfter(std::size_t checkpoint) noexcept {
    if (checkpoint >= commands_.size()) { return; }
    while (commands_.size() > checkpoint) {
        commands_.pop_back();
    }
}

}
