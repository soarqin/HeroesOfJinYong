#pragma once

#include "node.hh"

#include <type_traits>
#include <utility>

namespace hojy::scene {

/** Queue a scene service request for the fixed-update command barrier. */
template<typename Function>
void postSceneCommand(Node *node, Function function) {
    if (!node) { return; }
    node->postCommand(std::make_unique<FunctionSceneCommand>(
        [function = std::move(function)](SceneCommandContext &context) mutable {
            function(context);
        }));
}

/** Queue a request that is valid only while its originating node survives. */
template<typename Owner, typename Function>
void postOwnedSceneCommand(Owner *owner, Function function) {
    static_assert(std::is_base_of<Node, Owner>::value,
                  "owned scene command owner must derive from Node");
    if (!owner) { return; }
    auto lifetime = owner->lifetimeHandle();
    owner->postCommand(std::make_unique<FunctionSceneCommand>(
        [lifetime = std::move(lifetime), function = std::move(function)](
            SceneCommandContext &context) mutable {
            auto state = lifetime.lock();
            if (!state || !state->owner) { return; }
            function(*static_cast<Owner *>(state->owner), context);
        }));
}

/** Queue a move-only command while retaining its originating node lifetime. */
template<typename Owner>
class OwnedSceneCommand final : public SceneCommand {
public:
    OwnedSceneCommand(typename Node::LifetimeHandle lifetime,
                      std::unique_ptr<SceneCommand> command)
        : lifetime_(std::move(lifetime)), command_(std::move(command)) {}

    void execute(SceneCommandContext &context) override {
        auto state = lifetime_.lock();
        if (!state || !state->owner || !command_) {
            command_.reset();
            return;
        }
        command_->execute(context);
        command_.reset();
    }

private:
    typename Node::LifetimeHandle lifetime_;
    std::unique_ptr<SceneCommand> command_;
};

// Keep move-only derived commands on the command path instead of attempting
// to treat their unique_ptr as a callable for the generic overload above.
template<typename Owner, typename Command>
void postOwnedSceneCommand(Owner *owner,
                           std::unique_ptr<Command> command) {
    static_assert(std::is_base_of<Node, Owner>::value,
                  "owned scene command owner must derive from Node");
    static_assert(std::is_base_of<SceneCommand, Command>::value,
                  "owned scene command must derive from SceneCommand");
    if (!owner || !command) { return; }
    owner->postCommand(std::make_unique<OwnedSceneCommand<Owner>>(
        owner->lifetimeHandle(),
        std::unique_ptr<SceneCommand>(std::move(command))));
}

template<typename Owner>
void postOwnedSceneCommand(Owner *owner, std::unique_ptr<SceneCommand> command) {
    static_assert(std::is_base_of<Node, Owner>::value,
                  "owned scene command owner must derive from Node");
    if (!owner || !command) { return; }
    owner->postCommand(std::make_unique<OwnedSceneCommand<Owner>>(
        owner->lifetimeHandle(), std::move(command)));
}

}
