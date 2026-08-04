#include "scene/node.hh"

#include "test_support.hh"

#include <iostream>

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

    void render() override {}

    void handleKeyInput(Key) override { requestDelete(); }

private:
    Counters &counters_;
    bool removeParent_;
};

class RootNode final: public hojy::scene::Node {
public:
    RootNode(): Node(static_cast<hojy::scene::Renderer *>(nullptr), 0, 0, 1, 1) {}
    void render() override {}
};

void testChildDeletionIsDeferredUntilDispatchCompletes() {
    Counters counters;
    RootNode root;
    auto *child = new ProbeNode(&root, counters);
    root.dispatchKeyInput(hojy::scene::Node::KeyOK);
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

}

int main() {
    try {
        testChildDeletionIsDeferredUntilDispatchCompletes();
        testParentDeletionDestroysParentAndChildExactlyOnce();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
