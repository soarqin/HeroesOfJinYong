#pragma once

#include "core/input_event.hh"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <utility>

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

class KeyIntent;
class TextIntent;

struct InputIntentMetadata {
    std::uint64_t timestamp = 0;
    core::InputDevice device = core::InputDevice::Keyboard;
    std::uint64_t sequence = 0;
};

class InputConsumer {
public:
    virtual ~InputConsumer() = default;
    virtual void consume(const KeyIntent &intent) = 0;
    virtual void consume(const TextIntent &intent) = 0;
};

class SceneInputIntent {
public:
    explicit SceneInputIntent(InputIntentMetadata metadata = {})
        : metadata_(metadata) {}
    virtual ~SceneInputIntent() = default;

    virtual void deliver(InputConsumer &consumer) const = 0;

    [[nodiscard]] const InputIntentMetadata &metadata() const {
        return metadata_;
    }
    [[nodiscard]] std::uint64_t timestamp() const {
        return metadata_.timestamp;
    }
    [[nodiscard]] core::InputDevice device() const {
        return metadata_.device;
    }
    [[nodiscard]] std::uint64_t sequence() const {
        return metadata_.sequence;
    }

private:
    InputIntentMetadata metadata_;
};

class KeyIntent final : public SceneInputIntent {
public:
    explicit KeyIntent(InputKey key, InputIntentMetadata metadata = {})
        : SceneInputIntent(metadata), key_(key) {}
    KeyIntent(InputKey key, std::uint64_t timestamp, core::InputDevice device,
              std::uint64_t sequence)
        : KeyIntent(key, {timestamp, device, sequence}) {}

    [[nodiscard]] InputKey key() const { return key_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    InputKey key_;
};

class TextIntent final : public SceneInputIntent {
public:
    explicit TextIntent(std::wstring text, InputIntentMetadata metadata = {})
        : SceneInputIntent(metadata), text_(std::move(text)) {}
    TextIntent(std::wstring text, std::uint64_t timestamp,
               core::InputDevice device, std::uint64_t sequence)
        : TextIntent(std::move(text), {timestamp, device, sequence}) {}

    [[nodiscard]] const std::wstring &text() const { return text_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    std::wstring text_;
};

class InputPort {
public:
    virtual ~InputPort() = default;
    virtual void enqueue(std::unique_ptr<SceneInputIntent> intent) = 0;
};

class QueuedInputPort final : public InputPort {
public:
    void enqueue(std::unique_ptr<SceneInputIntent> intent) override;

    [[nodiscard]] bool empty() const { return intents_.empty(); }
    [[nodiscard]] std::size_t size() const { return intents_.size(); }
    bool deliverNext(InputConsumer &consumer);

private:
    std::deque<std::unique_ptr<SceneInputIntent>> intents_;
};

}
