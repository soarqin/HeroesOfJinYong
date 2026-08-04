#include "sdl_input.hh"

#include "util/conv.hh"

#include <SDL.h>

#include <cstdint>
#include <limits>
#include <map>
#include <utility>

namespace hojy::app {

namespace {

std::uint64_t performanceMicros() {
    const auto frequency = SDL_GetPerformanceFrequency();
    const auto counter = SDL_GetPerformanceCounter();
    if (frequency == 0) {
        return static_cast<std::uint64_t>(SDL_GetTicks64()) * 1000ULL;
    }
    if (counter > std::numeric_limits<std::uint64_t>::max() / 1000000ULL) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return counter * 1000000ULL / static_cast<std::uint64_t>(frequency);
}

const std::map<SDL_Scancode, InputAction> &keyboardMap() {
    static const std::map<SDL_Scancode, InputAction> map = {
        {SDL_SCANCODE_UP, InputAction::Up},
        {SDL_SCANCODE_KP_8, InputAction::Up},
        {SDL_SCANCODE_DOWN, InputAction::Down},
        {SDL_SCANCODE_KP_2, InputAction::Down},
        {SDL_SCANCODE_LEFT, InputAction::Left},
        {SDL_SCANCODE_KP_4, InputAction::Left},
        {SDL_SCANCODE_RIGHT, InputAction::Right},
        {SDL_SCANCODE_KP_6, InputAction::Right},
        {SDL_SCANCODE_RETURN, InputAction::Accept},
        {SDL_SCANCODE_KP_ENTER, InputAction::Accept},
        {SDL_SCANCODE_ESCAPE, InputAction::Cancel},
        {SDL_SCANCODE_DELETE, InputAction::Cancel},
        {SDL_SCANCODE_KP_PERIOD, InputAction::Cancel},
        {SDL_SCANCODE_SPACE, InputAction::Space},
        {SDL_SCANCODE_BACKSPACE, InputAction::Backspace},
    };
    return map;
}

const std::map<SDL_GameControllerButton, InputAction> &buttonMap() {
    static const std::map<SDL_GameControllerButton, InputAction> map = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP, InputAction::Up},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, InputAction::Down},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, InputAction::Left},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, InputAction::Right},
        {SDL_CONTROLLER_BUTTON_A, InputAction::Accept},
        {SDL_CONTROLLER_BUTTON_B, InputAction::Cancel},
    };
    return map;
}

void drainRepeats(InputRepeater &repeater, InputQueue &queue,
                  std::uint64_t timestamp) {
    for (auto event : repeater.drainThrough(timestamp)) {
        queue.push(std::move(event));
    }
}

}

void SdlInputCollector::collect(InputQueue &queue) {
    const auto now = performanceMicros();
    drainRepeats(inputRepeater_, queue, now);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_GameControllerOpen(event.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
            break;
        case SDL_CONTROLLERBUTTONDOWN: {
            const auto item = buttonMap().find(SDL_GameControllerButton(event.cbutton.button));
            if (item != buttonMap().end()) {
                inputRepeater_.press(-int(item->first), InputDevice::Controller,
                                     item->second, now);
                drainRepeats(inputRepeater_, queue, now);
            }
            break;
        }
        case SDL_CONTROLLERBUTTONUP: {
            const auto item = buttonMap().find(SDL_GameControllerButton(event.cbutton.button));
            if (item != buttonMap().end()) {
                inputRepeater_.release(-int(item->first));
            }
            break;
        }
        case SDL_TEXTINPUT:
            queue.push(InputEvent{now, InputDevice::Text, InputAction::Text, 0,
                                  util::Utf8Conv::toUnicode(event.text.text)});
            break;
        case SDL_KEYDOWN: {
            if (event.key.repeat) {
                break;
            }
            const auto item = keyboardMap().find(event.key.keysym.scancode);
            if (item != keyboardMap().end()) {
                inputRepeater_.press(int(item->first), InputDevice::Keyboard,
                                     item->second, now);
                drainRepeats(inputRepeater_, queue, now);
            }
            break;
        }
        case SDL_KEYUP: {
            const auto item = keyboardMap().find(event.key.keysym.scancode);
            if (item != keyboardMap().end()) {
                inputRepeater_.release(int(item->first));
            }
            break;
        }
        case SDL_QUIT:
            queue.push(InputEvent{now, InputDevice::System, InputAction::Quit});
            break;
        default:
            break;
        }
    }

    drainRepeats(inputRepeater_, queue, now);
}

}
