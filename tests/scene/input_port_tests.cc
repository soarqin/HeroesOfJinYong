#include "scene/logic/input.hh"

#include "test_support.hh"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

class RecordingConsumer final : public hojy::scene::InputConsumer {
public:
    void consume(const hojy::scene::KeyIntent &intent) override {
        keys.push_back(intent.key());
    }

    void consume(const hojy::scene::TextIntent &intent) override {
        texts.push_back(intent.text());
    }

    std::vector<hojy::scene::InputKey> keys;
    std::vector<std::wstring> texts;
};

void testQueuedInputPortPreservesIntentOrder() {
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Left));
    port.enqueue(std::make_unique<hojy::scene::TextIntent>(L"令狐冲"));

    RecordingConsumer consumer;
    HOJY_CHECK_EQ(port.size(), 2U);
    HOJY_CHECK_EQ(port.deliverNext(consumer), true);
    HOJY_CHECK_EQ(port.deliverNext(consumer), true);
    HOJY_CHECK_EQ(consumer.keys.at(0), hojy::scene::InputKey::Left);
    HOJY_CHECK_EQ(consumer.texts.at(0), L"令狐冲");
    HOJY_CHECK_EQ(port.empty(), true);
    HOJY_CHECK_EQ(port.deliverNext(consumer), false);
}

void testCoreEventMappingPreservesMetadata() {
    const hojy::core::InputEvent event{
        42,
        hojy::core::InputDevice::Controller,
        hojy::core::InputAction::Right,
        7,
        L"",
        99};
    auto intent = hojy::scene::makeIntent(event);
    HOJY_CHECK_EQ(static_cast<bool>(intent), true);
    HOJY_CHECK_EQ(intent->metadata().timestamp, 42ULL);
    HOJY_CHECK_EQ(intent->metadata().device,
                  hojy::core::InputDevice::Controller);
    HOJY_CHECK_EQ(intent->metadata().sequence, 99ULL);

    RecordingConsumer consumer;
    intent->deliver(consumer);
    HOJY_CHECK_EQ(consumer.keys.at(0), hojy::scene::InputKey::Right);
}

}

int main() {
    try {
        testQueuedInputPortPreservesIntentOrder();
        testCoreEventMappingPreservesMetadata();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
