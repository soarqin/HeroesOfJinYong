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

class RecordingSink final : public hojy::scene::ExtendedInputCompletionSink {
public:
    void submit(hojy::scene::ExtendedInputResult result) override {
        ++count;
        last = result;
    }

    int count = 0;
    hojy::scene::ExtendedInputResult last;
};

void testWaitingOverlayConsumesOneKey() {
    RootNode root;
    auto *overlay = new hojy::scene::ExtendedNode(&root, 0, 0, 320, 200);
    overlay->setWaitForKeyPress();
    auto sink = std::make_unique<RecordingSink>();
    auto *recording = sink.get();
    overlay->setInputCompletionSink(std::move(sink));

    hojy::scene::KeyIntent input(hojy::scene::InputKey::Left);
    root.consume(input);
    root.dispatchInputLogic();

    HOJY_CHECK_EQ(recording->count, 1);
    HOJY_CHECK_EQ(recording->last.key, hojy::scene::InputKey::Left);
    HOJY_CHECK_EQ(recording->last.timedOut, false);
    HOJY_CHECK_EQ(overlay->presentationCleanupRequested(), true);
    root.dispatchPrepareRender();
    HOJY_CHECK_EQ(overlay->deleteRequested(), true);
    root.applyDeferredDeletes();
}

}

int main() {
    try {
        testWaitingOverlayConsumesOneKey();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
