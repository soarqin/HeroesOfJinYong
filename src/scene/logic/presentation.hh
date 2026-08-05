#pragma once

#include "scene/status_snapshot.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {

/**
 * Value returned by a menu that was requested by an event VM.
 *
 * The continuation token is owned by the logic producer. Presentation code
 * only transports this value back across the command boundary.
 */
struct EventMenuResult final {
    std::uint64_t continuationToken = 0;
    std::int16_t selection = 0;
    bool accepted = false;
};

class EventMenuContinuation {
public:
    virtual ~EventMenuContinuation() = default;
    virtual void submit(EventMenuResult result) = 0;
};

/**
 * Typed request consumed by the presentation layer at the command barrier.
 */
struct EventMenuRequest final {
    std::vector<std::wstring> items;
    std::uint64_t sessionToken = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::uint64_t continuationToken = 0;
    std::shared_ptr<EventMenuContinuation> continuation;
};

/**
 * Value returned by an event-owned transient input surface such as a key or
 * timeout prompt. It deliberately carries no VM or scene object reference.
 */
struct EventInputResult final {
    std::uint64_t continuationToken = 0;
    std::int16_t value = 0;
};

class EventInputContinuation {
public:
    virtual ~EventInputContinuation() = default;
    virtual void submit(EventInputResult result) = 0;
};

class EventOverlaySurface {
public:
    virtual ~EventOverlaySurface() = default;
    virtual void addText(std::int16_t x, std::int16_t y, std::wstring text,
                         std::int16_t color, std::int16_t alternateColor) = 0;
    virtual void addBox(std::int16_t x0, std::int16_t y0,
                        std::int16_t x1, std::int16_t y1) = 0;
    virtual void addSubMapSprite(std::int16_t x, std::int16_t y,
                                 std::int16_t id) = 0;
    virtual void addHeadSprite(std::int16_t x, std::int16_t y,
                               std::int16_t id) = 0;
    virtual void waitForDirectionalInput(
        std::uint64_t token,
        std::shared_ptr<EventInputContinuation> continuation) = 0;
    virtual void waitForConfirmInput(
        std::uint64_t token,
        std::shared_ptr<EventInputContinuation> continuation) = 0;
    virtual void keepAlive() = 0;
};

class EventOverlayOperation {
public:
    virtual ~EventOverlayOperation() = default;
    virtual void apply(EventOverlaySurface &surface) const = 0;
};

class EventOverlayTextOperation final : public EventOverlayOperation {
public:
    EventOverlayTextOperation(std::int16_t x, std::int16_t y,
                              std::wstring text, std::int16_t color,
                              std::int16_t alternateColor)
        : x_(x), y_(y), text_(std::move(text)), color_(color),
          alternateColor_(alternateColor) {}

    void apply(EventOverlaySurface &surface) const override {
        surface.addText(x_, y_, text_, color_, alternateColor_);
    }

private:
    std::int16_t x_ = 0;
    std::int16_t y_ = 0;
    std::wstring text_;
    std::int16_t color_ = 0;
    std::int16_t alternateColor_ = 0;
};

class EventOverlayBoxOperation final : public EventOverlayOperation {
public:
    EventOverlayBoxOperation(std::int16_t x0, std::int16_t y0,
                             std::int16_t x1, std::int16_t y1)
        : x0_(x0), y0_(y0), x1_(x1), y1_(y1) {}

    void apply(EventOverlaySurface &surface) const override {
        surface.addBox(x0_, y0_, x1_, y1_);
    }

private:
    std::int16_t x0_ = 0;
    std::int16_t y0_ = 0;
    std::int16_t x1_ = 0;
    std::int16_t y1_ = 0;
};

class EventOverlaySubMapSpriteOperation final : public EventOverlayOperation {
public:
    EventOverlaySubMapSpriteOperation(std::int16_t x, std::int16_t y,
                                      std::int16_t id)
        : x_(x), y_(y), id_(id) {}

    void apply(EventOverlaySurface &surface) const override {
        surface.addSubMapSprite(x_, y_, id_);
    }

private:
    std::int16_t x_ = 0;
    std::int16_t y_ = 0;
    std::int16_t id_ = -1;
};

class EventOverlayHeadSpriteOperation final : public EventOverlayOperation {
public:
    EventOverlayHeadSpriteOperation(std::int16_t x, std::int16_t y,
                                    std::int16_t id)
        : x_(x), y_(y), id_(id) {}

    void apply(EventOverlaySurface &surface) const override {
        surface.addHeadSprite(x_, y_, id_);
    }

private:
    std::int16_t x_ = 0;
    std::int16_t y_ = 0;
    std::int16_t id_ = -1;
};

class EventOverlayDirectionalInputOperation final : public EventOverlayOperation {
public:
    EventOverlayDirectionalInputOperation(
        std::uint64_t token,
        std::shared_ptr<EventInputContinuation> continuation)
        : token_(token), continuation_(std::move(continuation)) {}

    void apply(EventOverlaySurface &surface) const override {
        surface.waitForDirectionalInput(token_, continuation_);
    }

private:
    std::uint64_t token_ = 0;
    std::shared_ptr<EventInputContinuation> continuation_;
};

class EventOverlayConfirmInputOperation final : public EventOverlayOperation {
public:
    EventOverlayConfirmInputOperation(
        std::uint64_t token,
        std::shared_ptr<EventInputContinuation> continuation)
        : token_(token), continuation_(std::move(continuation)) {}

    void apply(EventOverlaySurface &surface) const override {
        surface.waitForConfirmInput(token_, continuation_);
    }

private:
    std::uint64_t token_ = 0;
    std::shared_ptr<EventInputContinuation> continuation_;
};

class EventOverlayKeepAliveOperation final : public EventOverlayOperation {
public:
    void apply(EventOverlaySurface &surface) const override {
        surface.keepAlive();
    }
};

struct EventOverlayRequest final {
    std::uint64_t sessionToken = 0;
    std::shared_ptr<const EventOverlayOperation> operation;
};

struct EventPresentationClearRequest final {
    std::uint64_t sessionToken = 0;
};

struct EventFadeRequest final {
    std::uint64_t sessionToken = 0;
    std::uint64_t continuationToken = 0;
    std::shared_ptr<EventInputContinuation> continuation;
};

/**
 * Lifetime guard for battle presentation callbacks.
 *
 * A battle can be replaced while a popup callback is still queued. The
 * callback keeps only a weak handle and a value token; invalidating the
 * session expires the handle before an old callback can reach battle logic.
 */
class BattlePresentationSession final {
public:
    using Token = std::uint64_t;

    struct State final {
        Token token = 0;
    };

    using Handle = std::weak_ptr<const State>;

    Token begin() {
        const auto next = nextToken();
        auto candidate = std::make_shared<State>(State{next});
        state_ = std::move(candidate);
        return next;
    }

    void invalidate() noexcept {
        state_.reset();
        (void)nextToken();
    }

    [[nodiscard]] Token token() const noexcept {
        return state_ ? state_->token : 0;
    }

    [[nodiscard]] bool matches(Token token) const noexcept {
        return token != 0 && state_ && state_->token == token;
    }

    [[nodiscard]] Handle handle() const noexcept { return state_; }

    [[nodiscard]] static bool matches(const Handle &handle, Token token) noexcept {
        if (token == 0) { return false; }
        const auto state = handle.lock();
        return state && state->token == token;
    }

private:
    Token nextToken() noexcept {
        ++generation_;
        if (generation_ == 0) { generation_ = 1; }
        return generation_;
    }

    Token generation_ = 0;
    std::shared_ptr<const State> state_;
};

/**
 * Value-level continuation used by item and character-selection surfaces.
 * Presentation code only transports the selected value; it never owns world
 * state or invokes a domain operation directly.
 */
struct CharacterSelectionResult final {
    std::uint64_t continuationToken = 0;
    std::int16_t characterId = -1;
    bool accepted = false;
};

class CharacterSelectionContinuation {
public:
    virtual ~CharacterSelectionContinuation() = default;
    virtual void submit(CharacterSelectionResult result) = 0;
};

struct CharacterListRowSnapshot final {
    std::int16_t characterId = -1;
    bool emphasized = false;
    std::wstring name;
    std::wstring valueText;
};

struct CharacterListSnapshot final {
    std::vector<std::wstring> title;
    std::wstring columnTitle;
    std::vector<CharacterListRowSnapshot> rows;
};

struct ItemViewEntrySnapshot final {
    std::int16_t itemId = -1;
    std::int16_t count = 0;
    std::wstring displayText;
    std::wstring description;
    std::vector<std::wstring> requirementLines;
    std::vector<std::wstring> effectLines;
    std::wstring requirementTitle;
    std::wstring effectTitle;
};

struct CharacterSelectionRequest final {
    CharacterListSnapshot characters;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::uint64_t continuationToken = 0;
    std::shared_ptr<CharacterSelectionContinuation> continuation;
};

struct ItemMessageResult final {
    std::uint64_t continuationToken = 0;
    bool accepted = false;
};

class ItemMessageContinuation {
public:
    virtual ~ItemMessageContinuation() = default;
    virtual void submit(ItemMessageResult result) = 0;
};

struct ItemMessageRequest final {
    std::vector<std::wstring> text;
    std::uint8_t popupType = 0;
    std::uint64_t continuationToken = 0;
    std::shared_ptr<ItemMessageContinuation> continuation;
};

/** Value-only requests emitted by battle logic and consumed by the scene
 * presentation adapter at the fixed-logic command barrier. */
enum class BattlePresentationStage : std::uint8_t {
    Any = 0,
    PlayerMenu,
    DirectionSelection,
    SkillLevelUp,
    ItemResult,
    ItemSelection,
    StatusSelection,
    FinishMessages,
};

[[nodiscard]] inline bool isConcreteBattlePresentationStage(
        BattlePresentationStage stage) noexcept {
    return stage != BattlePresentationStage::Any;
}

[[nodiscard]] inline bool isValidBattlePresentationRequest(
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage) noexcept {
    return sessionToken != 0 && actionGeneration != 0
        && isConcreteBattlePresentationStage(expectedStage);
}

struct BattleDirectionSelectionRequest final {
    std::uint64_t sessionToken = 0;
    std::int16_t actorId = -1;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
    std::wstring prompt;
};

struct BattleSkillLevelUpRequest final {
    std::uint64_t sessionToken = 0;
    std::int16_t actorId = -1;
    std::int16_t skillId = -1;
    std::int16_t skillIndex = -1;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
    std::wstring message;
};

struct BattleItemChange final {
    std::int16_t property = 0;
    std::int16_t value = 0;
};

struct BattleItemResultRequest final {
    std::uint64_t sessionToken = 0;
    std::int16_t actorId = -1;
    std::int16_t itemId = -1;
    std::vector<BattleItemChange> changes;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
    std::vector<std::wstring> messages;
};

struct BattleMenuEntrySnapshot final {
    int actionId = -1;
    std::wstring label;
};

struct BattleSkillEntrySnapshot final {
    int skillIndex = -1;
    std::wstring label;
};

struct BattleMenuRequest final {
    std::uint64_t sessionToken = 0;
    std::int16_t actorId = -1;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
    std::vector<BattleMenuEntrySnapshot> entries;
    std::vector<BattleSkillEntrySnapshot> skills;
    std::wstring noSkillMessage;
    int initialIndex = 0;
};

struct BattleItemSelectionRequest final {
    std::uint64_t sessionToken = 0;
    std::int16_t actorId = -1;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
    std::vector<ItemViewEntrySnapshot> items;
};

struct BattleStatusSelectionRequest final {
    std::uint64_t sessionToken = 0;
    std::int16_t actorId = -1;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
    CharacterListSnapshot characters;
    std::vector<CharacterStatusSnapshot> statuses;
};

struct BattleFinishMessagesRequest final {
    std::uint64_t sessionToken = 0;
    std::vector<std::pair<int, std::wstring>> messages;
    bool won = false;
    bool instantDie = false;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
};

struct BattleEndRequest final {
    std::uint64_t sessionToken = 0;
    bool won = false;
    bool instantDie = false;
    std::uint64_t actionGeneration = 0;
    BattlePresentationStage expectedStage = BattlePresentationStage::Any;
};

/**
 * Terminal battle-engine failure.  This request intentionally carries only
 * the Window-owned session token: the Warfield presentation session is
 * invalidated before the command reaches the scene transition barrier.
 */
struct BattleAbortRequest final {
    std::uint64_t sessionToken = 0;
    bool instantDie = false;
};

}
