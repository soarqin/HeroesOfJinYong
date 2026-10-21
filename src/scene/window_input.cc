#include "window.hh"

namespace hojy::scene {

namespace {

}

void Window::dispatchInput(const core::InputEvent &event) {
    sampledInputEvents_.push_back(event);
}

}
