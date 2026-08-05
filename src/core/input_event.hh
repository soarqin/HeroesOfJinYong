#pragma once

#include <cstdint>
#include <string>

namespace hojy::core {

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

}
