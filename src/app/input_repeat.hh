#pragma once

#include "input.hh"

#include <cstdint>
#include <map>

namespace hojy::app {

class InputRepeater final {
public:
    static constexpr std::uint64_t InitialDelayMicros = 180000;
    static constexpr std::uint64_t RepeatIntervalMicros = 20000;

    void press(int physicalId, InputDevice device, InputAction action,
               std::uint64_t timestamp);
    void release(int physicalId);

    [[nodiscard]] std::vector<InputEvent> drainThrough(std::uint64_t timestamp);
    [[nodiscard]] bool empty() const { return states_.empty() && queue_.empty(); }

private:
    struct State {
        InputDevice device = InputDevice::Keyboard;
        InputAction action = InputAction::Accept;
        std::uint64_t nextRepeat = 0;
    };

    void emitRepeats(std::uint64_t timestamp);

    std::map<int, State> states_;
    InputQueue queue_;
};

}
