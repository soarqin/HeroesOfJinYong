#include "scene/node.hh"

#include "test_support.hh"

#include <iostream>

namespace {

class ProbeNode final : public hojy::scene::Node {
public:
    ProbeNode(hojy::scene::Node *parent, int *prepareCount, int *renderCount)
        : Node(parent, 0, 0, 1, 1),
          prepareCount_(prepareCount),
          renderCount_(renderCount) {}

    void prepareRender() override { ++*prepareCount_; }

    void render() const override { ++*renderCount_; }

private:
    int *prepareCount_;
    int *renderCount_;
};

void testPrepareRunsBeforeReadOnlyRender() {
    int parentPrepare = 0;
    int parentRender = 0;
    int childPrepare = 0;
    int childRender = 0;
    ProbeNode parent(nullptr, &parentPrepare, &parentRender);
    ProbeNode child(&parent, &childPrepare, &childRender);

    parent.dispatchPrepareRender();
    HOJY_CHECK_EQ(parentPrepare, 1);
    HOJY_CHECK_EQ(childPrepare, 1);
    HOJY_CHECK_EQ(parentRender, 0);
    HOJY_CHECK_EQ(childRender, 0);

    parent.dispatchRender();
    parent.dispatchRender();
    HOJY_CHECK_EQ(parentRender, 2);
    HOJY_CHECK_EQ(childRender, 2);
    HOJY_CHECK_EQ(parent.deleteRequested(), false);
    HOJY_CHECK_EQ(child.deleteRequested(), false);
}

}

int main() {
    try {
        testPrepareRunsBeforeReadOnlyRender();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
