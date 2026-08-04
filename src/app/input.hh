#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace hojy::app {

enum class InputDevice {
    Keyboard,
    Controller,
    Text,
    System,
};

enum class InputAction {
    Up,
    Down,
    Left,
    Right,
    Accept,
    Cancel,
    Space,
    Backspace,
    Text,
    Quit,
};

struct InputEvent {
    std::uint64_t timestamp = 0;
    InputDevice device = InputDevice::Keyboard;
    InputAction action = InputAction::Accept;
    int value = 0;
    std::wstring text;
    std::uint64_t sequence = 0;
};

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
