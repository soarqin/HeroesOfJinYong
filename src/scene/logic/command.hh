#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>

namespace hojy::scene {

class SceneCommand {
public:
    virtual ~SceneCommand() = default;
    virtual void execute() = 0;
};

class FunctionSceneCommand final : public SceneCommand {
public:
    explicit FunctionSceneCommand(std::function<void()> function)
        : function_(std::move(function)) {}

    void execute() override {
        if (function_) { function_(); }
    }

private:
    std::function<void()> function_;
};

class SceneCommandQueue final {
public:
    void push(std::unique_ptr<SceneCommand> command);
    void push(std::function<void()> function);

    void executeGeneration();
    void executeAllGenerations();

    [[nodiscard]] bool empty() const { return commands_.empty(); }
    [[nodiscard]] std::size_t size() const { return commands_.size(); }

private:
    std::deque<std::unique_ptr<SceneCommand>> commands_;
};

}
