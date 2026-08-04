#include "scene/logic/input.hh"

#include "test_support.hh"

#include <iostream>
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

void testIntentsRetainSourceMetadata() {
    const hojy::scene::InputIntentMetadata metadata{
        123, hojy::core::InputDevice::Controller, 7};
    const hojy::scene::KeyIntent intent(hojy::scene::InputKey::Accept,
                                        metadata);

    HOJY_CHECK_EQ(intent.timestamp(), 123ULL);
    HOJY_CHECK_EQ(intent.device(), hojy::core::InputDevice::Controller);
    HOJY_CHECK_EQ(intent.sequence(), 7ULL);
}

}

int main() {
    try {
        testQueuedInputPortPreservesIntentOrder();
        testIntentsRetainSourceMetadata();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
