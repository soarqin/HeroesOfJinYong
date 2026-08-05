#include "input.hh"

namespace hojy::scene {

std::unique_ptr<SceneInputIntent> makeIntent(const core::InputEvent &event) {
    const InputMetadata metadata{event.timestamp, event.device, event.sequence};
    if (event.action == core::InputAction::Text) {
        return std::make_unique<TextIntent>(event.text, metadata);
    }

    InputKey key = InputKey::None;
    switch (event.action) {
    case core::InputAction::Up: key = InputKey::Up; break;
    case core::InputAction::Down: key = InputKey::Down; break;
    case core::InputAction::Left: key = InputKey::Left; break;
    case core::InputAction::Right: key = InputKey::Right; break;
    case core::InputAction::Accept: key = InputKey::Accept; break;
    case core::InputAction::Cancel: key = InputKey::Cancel; break;
    case core::InputAction::Space: key = InputKey::Space; break;
    case core::InputAction::Backspace: key = InputKey::Backspace; break;
    default: break;
    }
    if (key == InputKey::None) {
        return nullptr;
    }
    return std::make_unique<KeyIntent>(key, metadata);
}

void QueuedInputPort::enqueue(std::unique_ptr<SceneInputIntent> intent) {
    if (intent) {
        intents_.push_back(std::move(intent));
    }
}

bool QueuedInputPort::deliverNext(InputConsumer &consumer) {
    if (intents_.empty()) {
        return false;
    }
    auto intent = std::move(intents_.front());
    intents_.pop_front();
    intent->deliver(consumer);
    return true;
}

}
