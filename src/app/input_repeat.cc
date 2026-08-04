#include "input_repeat.hh"

#include <limits>

namespace hojy::app {

void InputRepeater::press(int physicalId, InputDevice device, InputAction action,
                          std::uint64_t timestamp) {
    states_[physicalId] = State{device, action,
                                timestamp > std::numeric_limits<std::uint64_t>::max() - InitialDelayMicros
                                    ? std::numeric_limits<std::uint64_t>::max()
                                    : timestamp + InitialDelayMicros};
    queue_.push(InputEvent{timestamp, device, action});
}

void InputRepeater::release(int physicalId) {
    states_.erase(physicalId);
}

void InputRepeater::emitRepeats(std::uint64_t timestamp) {
    for (auto &entry : states_) {
        auto &state = entry.second;
        std::size_t emitted = 0;
        while (state.nextRepeat <= timestamp && emitted < 4096) {
            queue_.push(InputEvent{state.nextRepeat, state.device, state.action});
            ++emitted;
            if (state.nextRepeat > std::numeric_limits<std::uint64_t>::max() - RepeatIntervalMicros) {
                state.nextRepeat = std::numeric_limits<std::uint64_t>::max();
                break;
            }
            state.nextRepeat += RepeatIntervalMicros;
        }
        if (emitted == 4096 && state.nextRepeat <= timestamp) {
            state.nextRepeat = timestamp > std::numeric_limits<std::uint64_t>::max() - RepeatIntervalMicros
                ? std::numeric_limits<std::uint64_t>::max()
                : timestamp + RepeatIntervalMicros;
        }
    }
}

std::vector<InputEvent> InputRepeater::drainThrough(std::uint64_t timestamp) {
    emitRepeats(timestamp);
    return queue_.drainThrough(timestamp);
}

}
