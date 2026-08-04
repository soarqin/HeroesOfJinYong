#pragma once

#include <cstddef>
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

class InputConsumer {
public:
    virtual ~InputConsumer() = default;
    virtual void consume(const KeyIntent &intent) = 0;
    virtual void consume(const TextIntent &intent) = 0;
};

class SceneInputIntent {
public:
    virtual ~SceneInputIntent() = default;
    virtual void deliver(InputConsumer &consumer) const = 0;
};

class KeyIntent final : public SceneInputIntent {
public:
    explicit KeyIntent(InputKey key): key_(key) {}

    [[nodiscard]] InputKey key() const { return key_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    InputKey key_;
};

class TextIntent final : public SceneInputIntent {
public:
    explicit TextIntent(std::wstring text): text_(std::move(text)) {}

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
