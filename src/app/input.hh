#pragma once

#include "core/input_event.hh"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace hojy::app {

using InputDevice = core::InputDevice;
using InputAction = core::InputAction;
using InputEvent = core::InputEvent;

class InputQueue final {
public:
    void push(InputEvent event);
    [[nodiscard]] bool empty() const { return events_.empty(); }
    [[nodiscard]] std::size_t size() const { return events_.size(); }

    [[nodiscard]] std::optional<InputEvent> pop();
    [[nodiscard]] std::vector<InputEvent> drainThrough(std::uint64_t timestamp);

private:
    std::deque<InputEvent> events_;
    std::uint64_t nextSequence_ = 0;
};

}
