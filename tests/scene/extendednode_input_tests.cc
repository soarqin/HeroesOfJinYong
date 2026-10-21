#include "scene/extendednode.hh"
#include "scene/logic/input.hh"
#include "test_support.hh"

#include <iostream>

namespace {

class RootNode final : public hojy::scene::Node {
public:
    RootNode(): Node(static_cast<hojy::scene::Renderer *>(nullptr), 0, 0, 320, 200) {}

    void render() const override {}
};

struct CompletionState {
    int count = 0;
    hojy::scene::ExtendedInputResult last;
    bool destroyed = false;
};

class RecordingSink final : public hojy::scene::ExtendedInputCompletionSink {
public:
    explicit RecordingSink(CompletionState &state): state_(state) {}
    ~RecordingSink() override { state_.destroyed = true; }

    void submit(hojy::scene::ExtendedInputResult result) override {
        ++state_.count;
        state_.last = result;
    }

private:
    CompletionState &state_;
};

void testWaitingOverlayConsumesOneKey() {
    CompletionState state;
    RootNode root;
    auto *overlay = new hojy::scene::ExtendedNode(&root, 0, 0, 320, 200);
    overlay->setWaitForKeyPress();
    overlay->setInputCompletionSink(std::make_unique<RecordingSink>(state));

    hojy::scene::KeyIntent input(hojy::scene::InputKey::Left);
    root.consume(input);
    root.dispatchInputLogic();

    HOJY_CHECK_EQ(state.destroyed, true);
    HOJY_CHECK_EQ(state.count, 1);
    HOJY_CHECK_EQ(state.last.key, hojy::scene::InputKey::Left);
    HOJY_CHECK_EQ(state.last.timedOut, false);
    HOJY_CHECK_EQ(overlay->presentationCleanupRequested(), true);
    root.consume(input);
    root.dispatchInputLogic();
    HOJY_CHECK_EQ(state.count, 1);
    root.dispatchPrepareRender();
    HOJY_CHECK_EQ(overlay->deleteRequested(), true);
    root.applyDeferredDeletes();
}

void testTimedOverlayCompletesOnceAtItsDeadline() {
    CompletionState state;
    RootNode root;
    auto *overlay = new hojy::scene::ExtendedNode(&root, 0, 0, 320, 200);
    overlay->setTimeToClose(10);
    overlay->setInputCompletionSink(std::make_unique<RecordingSink>(state));

    root.setPhaseTime(9999);
    overlay->checkTimeout();
    HOJY_CHECK_EQ(state.count, 0);
    HOJY_CHECK_EQ(state.destroyed, false);
    HOJY_CHECK_EQ(overlay->presentationCleanupRequested(), false);

    root.setPhaseTime(10000);
    overlay->checkTimeout();
    overlay->checkTimeout();
    HOJY_CHECK_EQ(state.count, 1);
    HOJY_CHECK_EQ(state.destroyed, true);
    HOJY_CHECK_EQ(state.last.key, hojy::scene::InputKey::None);
    HOJY_CHECK_EQ(state.last.timedOut, true);
    HOJY_CHECK_EQ(overlay->presentationCleanupRequested(), true);
    root.dispatchPrepareRender();
    HOJY_CHECK_EQ(overlay->deleteRequested(), true);
    root.applyDeferredDeletes();
}

}

int main() {
    try {
        testWaitingOverlayConsumesOneKey();
        testTimedOverlayCompletesOnceAtItsDeadline();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
