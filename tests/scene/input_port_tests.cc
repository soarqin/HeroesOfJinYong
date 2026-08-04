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

}

int main() {
    try {
        testQueuedInputPortPreservesIntentOrder();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
