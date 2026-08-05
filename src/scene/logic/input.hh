#pragma once

#include "core/input_event.hh"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>

namespace hojy::scene {

enum class InputKey {
    None,
    Up,
    Down,
    Left,
    Right,
    Accept,
    Cancel,
    Space,
    Backspace,
};

struct InputMetadata final {
    std::uint64_t timestamp = 0;
    core::InputDevice device = core::InputDevice::Keyboard;
    std::uint64_t sequence = 0;
};

class KeyIntent;
class TextIntent;

class InputConsumer {
public:
    virtual ~InputConsumer() = default;
    virtual void consume(const KeyIntent &intent) = 0;
    virtual void consume(const TextIntent &intent) = 0;
};

class SceneInputIntent {
public:
    explicit SceneInputIntent(InputMetadata metadata = {})
        : metadata_(metadata) {}
    virtual ~SceneInputIntent() = default;

    [[nodiscard]] const InputMetadata &metadata() const noexcept {
        return metadata_;
    }
    virtual void deliver(InputConsumer &consumer) const = 0;

private:
    InputMetadata metadata_;
};

class KeyIntent final : public SceneInputIntent {
public:
    explicit KeyIntent(InputKey key, InputMetadata metadata = {})
        : SceneInputIntent(metadata), key_(key) {}

    [[nodiscard]] InputKey key() const noexcept { return key_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    InputKey key_;
};

class TextIntent final : public SceneInputIntent {
public:
    explicit TextIntent(std::wstring text, InputMetadata metadata = {})
        : SceneInputIntent(metadata), text_(std::move(text)) {}

    [[nodiscard]] const std::wstring &text() const noexcept { return text_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    std::wstring text_;
};

class InputPort {
public:
    virtual ~InputPort() = default;
    virtual void enqueue(std::unique_ptr<SceneInputIntent> intent) = 0;
};

[[nodiscard]] std::unique_ptr<SceneInputIntent> makeIntent(
    const core::InputEvent &event);

class QueuedInputPort final : public InputPort {
public:
    void enqueue(std::unique_ptr<SceneInputIntent> intent) override;
    [[nodiscard]] bool empty() const noexcept { return intents_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return intents_.size(); }
    bool deliverNext(InputConsumer &consumer);

private:
    std::deque<std::unique_ptr<SceneInputIntent>> intents_;
};

}
