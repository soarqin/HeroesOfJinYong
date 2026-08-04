#include "input.hh"

#include <algorithm>

namespace hojy::app {

void InputQueue::push(InputEvent event) {
    event.sequence = nextSequence_++;
    const auto position = std::upper_bound(
        events_.begin(), events_.end(), event,
        [](const InputEvent &left, const InputEvent &right) {
            if (left.timestamp != right.timestamp) {
                return left.timestamp < right.timestamp;
            }
            return left.sequence < right.sequence;
        });
    events_.insert(position, std::move(event));
}

std::optional<InputEvent> InputQueue::pop() {
    if (events_.empty()) { return std::nullopt; }
    auto event = std::move(events_.front());
    events_.pop_front();
    return event;
}

std::vector<InputEvent> InputQueue::drainThrough(std::uint64_t timestamp) {
    std::vector<InputEvent> ready;
    while (!events_.empty() && events_.front().timestamp <= timestamp) {
        ready.emplace_back(std::move(events_.front()));
        events_.pop_front();
    }
    return ready;
}

}
