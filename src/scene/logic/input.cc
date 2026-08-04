#include "input.hh"

namespace hojy::scene {

void QueuedInputPort::enqueue(std::unique_ptr<SceneInputIntent> intent) {
    if (intent) { intents_.push_back(std::move(intent)); }
}

bool QueuedInputPort::deliverNext(InputConsumer &consumer) {
    if (intents_.empty()) { return false; }

    auto intent = std::move(intents_.front());
    intents_.pop_front();
    intent->deliver(consumer);
    return true;
}

}
