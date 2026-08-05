#include "window.hh"

namespace hojy::scene {

namespace {

}

void Window::dispatchInput(const core::InputEvent &event) {
    pendingInputEvents_.push_back(event);
}

}
