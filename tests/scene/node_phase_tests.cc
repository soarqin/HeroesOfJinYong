#include "scene/node.hh"
#include "scene/logic/input.hh"

#include "test_support.hh"

#include <iostream>
#include <memory>
#include <vector>

namespace {

class ProbeNode final : public hojy::scene::Node {
public:
    ProbeNode(hojy::scene::Node *parent = nullptr)
        : Node(parent, 0, 0, 1, 1) {}

    void render() const override {}

    int keyCount = 0;
    int textCount = 0;

protected:
    void consumeKeyIntent(hojy::scene::InputKey key) override {
        lastKey = key;
        ++keyCount;
    }

    void consumeTextIntent(const std::wstring &text) override {
        lastText = text;
        ++textCount;
    }

public:
    hojy::scene::InputKey lastKey = hojy::scene::InputKey::None;
    std::wstring lastText;
};

class LogicProbeNode final : public hojy::scene::Node {
public:
    explicit LogicProbeNode(hojy::scene::Node *parent = nullptr)
        : Node(parent, 0, 0, 1, 1) {}

    void render() const override {}

    int consumed = 0;
    int applied = 0;
    bool deleteOnConsume = false;

    void applyInputLogic() override { ++applied; }

protected:
    void consumeKeyIntent(hojy::scene::InputKey) override {
        ++consumed;
        if (deleteOnConsume) { requestDelete(); }
    }
};

class OrderedNode final : public hojy::scene::Node {
public:
    OrderedNode(): Node(static_cast<hojy::scene::Node *>(nullptr), 0, 0, 1, 1) {}
    void render() const override {}

    std::vector<hojy::scene::InputKey> applied;

    void applyInputLogic() override {
        applied.push_back(pending);
        pending = hojy::scene::InputKey::None;
    }

protected:
    void consumeKeyIntent(hojy::scene::InputKey key) override { pending = key; }

private:
    hojy::scene::InputKey pending = hojy::scene::InputKey::None;
};

void testFadeCleanupIsDeferredUntilPreparation() {
    ProbeNode root;
    root.requestFadeCleanup();
    HOJY_CHECK_EQ(root.fadeCleanupRequested(), true);
    root.dispatchPrepareRender();
    HOJY_CHECK_EQ(root.fadeCleanupRequested(), false);
}

void testInputIsConsumedOnlyWhenDelivered() {
    ProbeNode root;
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Accept));
    port.enqueue(std::make_unique<hojy::scene::TextIntent>(L"令狐冲"));

    HOJY_CHECK_EQ(root.keyCount, 0);
    HOJY_CHECK_EQ(root.textCount, 0);
    HOJY_CHECK_EQ(port.deliverNext(root), true);
    HOJY_CHECK_EQ(root.keyCount, 1);
    HOJY_CHECK_EQ(root.lastKey, hojy::scene::InputKey::Accept);
    HOJY_CHECK_EQ(root.textCount, 0);
    HOJY_CHECK_EQ(port.deliverNext(root), true);
    HOJY_CHECK_EQ(root.textCount, 1);
    HOJY_CHECK_EQ(root.lastText, L"令狐冲");
}

void testInputLogicFlushesTheActualLeaf() {
    LogicProbeNode root;
    auto *leaf = new LogicProbeNode(&root);
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Accept));

    HOJY_CHECK_EQ(port.deliverNext(root), true);
    root.dispatchInputLogic();
    HOJY_CHECK_EQ(root.consumed, 0);
    HOJY_CHECK_EQ(root.applied, 0);
    HOJY_CHECK_EQ(leaf->consumed, 1);
    HOJY_CHECK_EQ(leaf->applied, 1);
}

void testDeletedLeafStillOwnsTheInputLogicFlush() {
    LogicProbeNode root;
    auto *leaf = new LogicProbeNode(&root);
    leaf->deleteOnConsume = true;
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Cancel));

    HOJY_CHECK_EQ(port.deliverNext(root), true);
    root.dispatchInputLogic();
    HOJY_CHECK_EQ(root.applied, 0);
    HOJY_CHECK_EQ(leaf->applied, 1);
    root.applyDeferredDeletes();
}

void testMultipleIntentsPreserveFixedLogicOrder() {
    OrderedNode root;
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(hojy::scene::InputKey::Up));
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(hojy::scene::InputKey::Right));
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(hojy::scene::InputKey::Accept));
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(hojy::scene::InputKey::Cancel));

    while (port.deliverNext(root)) {
        root.dispatchInputLogic();
    }
    HOJY_CHECK_EQ(root.applied.size(), 4U);
    HOJY_CHECK_EQ(root.applied[0], hojy::scene::InputKey::Up);
    HOJY_CHECK_EQ(root.applied[1], hojy::scene::InputKey::Right);
    HOJY_CHECK_EQ(root.applied[2], hojy::scene::InputKey::Accept);
    HOJY_CHECK_EQ(root.applied[3], hojy::scene::InputKey::Cancel);
}

void testDeferredDeleteRetargetsNextInputIntent() {
    LogicProbeNode root;
    auto *background = new LogicProbeNode(&root);
    auto *foreground = new LogicProbeNode(&root);
    foreground->deleteOnConsume = true;
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Accept));
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Cancel));

    HOJY_CHECK_EQ(port.deliverNext(root), true);
    root.dispatchInputLogic();
    HOJY_CHECK_EQ(foreground->consumed, 1);
    HOJY_CHECK_EQ(background->consumed, 0);
    root.applyDeferredDeletes();

    HOJY_CHECK_EQ(port.deliverNext(root), true);
    root.dispatchInputLogic();
    HOJY_CHECK_EQ(background->consumed, 1);
    HOJY_CHECK_EQ(background->applied, 1);
}

}

int main() {
    try {
        testInputIsConsumedOnlyWhenDelivered();
        testInputLogicFlushesTheActualLeaf();
        testDeletedLeafStillOwnsTheInputLogicFlush();
        testMultipleIntentsPreserveFixedLogicOrder();
        testDeferredDeleteRetargetsNextInputIntent();
        testFadeCleanupIsDeferredUntilPreparation();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
