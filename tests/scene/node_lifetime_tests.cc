#include "scene/node.hh"
#include "scene/window_command.hh"

#include "test_support.hh"

#include <iostream>
#include <utility>

namespace {

struct Counters {
    int destroyed = 0;
    int updated = 0;
};

class ProbeNode final: public hojy::scene::Node {
public:
    ProbeNode(hojy::scene::Node *parent, Counters &counters, bool removeParent = false):
        Node(parent, 0, 0, 1, 1), counters_(counters), removeParent_(removeParent) {}

    ~ProbeNode() override { ++counters_.destroyed; }

    void update() override {
        ++counters_.updated;
        if (removeParent_ && parent()) {
            parent()->requestDelete();
        }
    }

    void render() const override {}

    void consumeKeyIntent(hojy::scene::InputKey) override { requestDelete(); }

private:
    Counters &counters_;
    bool removeParent_;
};

class RootNode final: public hojy::scene::Node {
public:
    RootNode(): Node(static_cast<hojy::scene::Renderer *>(nullptr), 0, 0, 1, 1) {}
    void render() const override {}

    void clearChildrenDuringPrepare() { removeAllChildren(); }
};

class CommandProbeNode final: public hojy::scene::Node {
public:
    CommandProbeNode(): Node(static_cast<hojy::scene::Node *>(nullptr), 0, 0, 1, 1) {}
    void render() const override {}
};

void testChildDeletionIsDeferredUntilDispatchCompletes() {
    Counters counters;
    RootNode root;
    auto *child = new ProbeNode(&root, counters);
    root.consume(hojy::scene::KeyIntent(hojy::scene::InputKey::Accept));
    HOJY_CHECK_EQ(counters.destroyed, 0);
    root.applyDeferredDeletes();
    HOJY_CHECK_EQ(counters.destroyed, 1);
    (void)child;
}

void testParentDeletionDestroysParentAndChildExactlyOnce() {
    Counters counters;
    RootNode root;
    auto *parent = new ProbeNode(&root, counters);
    new ProbeNode(parent, counters, true);
    root.dispatchUpdate();
    HOJY_CHECK_EQ(counters.destroyed, 0);
    root.applyDeferredDeletes();
    HOJY_CHECK_EQ(counters.destroyed, 2);
}

void testPrepareCleanupRemovesStalePendingDeletePointers() {
    Counters counters;
    RootNode root;
    auto *child = new ProbeNode(&root, counters);
    child->requestDelete();
    root.clearChildrenDuringPrepare();
    HOJY_CHECK_EQ(counters.destroyed, 1);
    root.applyDeferredDeletes();
    HOJY_CHECK_EQ(counters.destroyed, 1);
}

void testOwnedCommandIsSkippedAfterOwnerDeletion() {
    hojy::scene::SceneCommandQueue queue;
    hojy::scene::SceneCommandContext context;
    int executed = 0;
    auto *owner = new CommandProbeNode;
    owner->setCommandSink([&queue](std::unique_ptr<hojy::scene::SceneCommand> command) {
        queue.push(std::move(command));
    });

    hojy::scene::postOwnedSceneCommand(
        owner,
        [&executed](CommandProbeNode &, hojy::scene::SceneCommandContext &) {
            ++executed;
        });
    delete owner;
    queue.executeGeneration(context);

    HOJY_CHECK_EQ(executed, 0);
}

void testOwnerIndependentCommandSurvivesSourceDeletion() {
    hojy::scene::SceneCommandQueue queue;
    hojy::scene::SceneCommandContext context;
    int executed = 0;
    auto *source = new CommandProbeNode;
    source->setCommandSink([&queue](std::unique_ptr<hojy::scene::SceneCommand> command) {
        queue.push(std::move(command));
    });

    hojy::scene::postSceneCommand(
        source,
        [&executed](hojy::scene::SceneCommandContext &) { ++executed; });
    delete source;
    queue.executeGeneration(context);

    HOJY_CHECK_EQ(executed, 1);
}

}

int main() {
    try {
        testChildDeletionIsDeferredUntilDispatchCompletes();
        testParentDeletionDestroysParentAndChildExactlyOnce();
        testPrepareCleanupRemovesStalePendingDeletePointers();
        testOwnedCommandIsSkippedAfterOwnerDeletion();
        testOwnerIndependentCommandSurvivesSourceDeletion();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
