#include "core/input_event.hh"

#include "test_support.hh"

#include <iostream>

int main() {
    try {
        const hojy::core::InputEvent event{
            123,
            hojy::core::InputDevice::Keyboard,
            hojy::core::InputAction::Left};
        HOJY_CHECK_EQ(event.timestamp, 123ULL);
        HOJY_CHECK_EQ(event.device, hojy::core::InputDevice::Keyboard);
        HOJY_CHECK_EQ(event.action, hojy::core::InputAction::Left);
        HOJY_CHECK_EQ(event.sequence, 0ULL);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
