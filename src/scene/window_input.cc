#include "window.hh"

#include <SDL.h>

namespace hojy::scene {

namespace {

Node::Key toNodeKey(app::InputAction action) {
    switch (action) {
    case app::InputAction::Up: return Node::KeyUp;
    case app::InputAction::Down: return Node::KeyDown;
    case app::InputAction::Left: return Node::KeyLeft;
    case app::InputAction::Right: return Node::KeyRight;
    case app::InputAction::Accept: return Node::KeyOK;
    case app::InputAction::Cancel: return Node::KeyCancel;
    case app::InputAction::Space: return Node::KeySpace;
    case app::InputAction::Backspace: return Node::KeyBackspace;
    default: return Node::KeyNone;
    }
}

}

void Window::dispatchInput(const app::InputEvent &event) {
    const bool wasProcessing = processingStage_;
    processingStage_ = true;
    if (event.action == app::InputAction::Quit) {
        quitRequested_ = true;
    } else if (event.action == app::InputAction::Text) {
        auto *node = popup_ ? popup_ : map_;
        if (node) { node->doTextInput(event.text); }
    } else {
        const auto key = toNodeKey(event.action);
        if (key != Node::KeyNone) {
            auto *node = popup_ ? popup_ : map_;
            if (node) { node->doHandleKeyInput(key); }
        }
    }
    processingStage_ = wasProcessing;
    if (!wasProcessing) {
        applyDeferredNodes();
        applyDeferredCommands();
    }
}

void Window::beginInput() {
    SDL_StartTextInput();
}

void Window::setInputRect(int x, int y, int w, int h) {
    SDL_Rect rect{x, y, w, h};
    SDL_SetTextInputRect(&rect);
}

void Window::endInput() {
    SDL_StopTextInput();
}

}
