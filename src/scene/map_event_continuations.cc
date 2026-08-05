#include "mapwithevent.hh"

#include "window_command.hh"

#include <memory>
#include <utility>

namespace hojy::scene {
namespace {

class QueuedEventInputContinuation final : public EventInputContinuation {
public:
    QueuedEventInputContinuation(
        std::weak_ptr<MapWithEvent::EventContinuationState> state,
        std::uint64_t token, std::int32_t destination, bool writesMemory)
        : state_(std::move(state)), token_(token), destination_(destination),
          writesMemory_(writesMemory) {}

    void submit(EventInputResult result) override {
        auto state = state_.lock();
        if (!state || !state->owner || result.continuationToken != token_) { return; }
        auto *map = state->owner;
        const auto token = token_;
        const auto destination = destination_;
        const auto value = result.value;
        const auto writesMemory = writesMemory_;
        postSceneCommand(map, [state = std::move(state), token, destination,
                               value, writesMemory](SceneCommandContext &) {
            if (state->owner) {
                state->owner->applyEventInputContinuation(
                    token, value, writesMemory, destination);
            }
        });
    }

private:
    std::weak_ptr<MapWithEvent::EventContinuationState> state_;
    std::uint64_t token_ = 0;
    std::int32_t destination_ = 0;
    bool writesMemory_ = false;
};

class QueuedEventMenuContinuation final : public EventMenuContinuation {
public:
    QueuedEventMenuContinuation(
        std::weak_ptr<MapWithEvent::EventContinuationState> state,
        std::uint64_t token, std::int32_t destination)
        : state_(std::move(state)), token_(token), destination_(destination) {}

    void submit(EventMenuResult result) override {
        auto state = state_.lock();
        if (!state || !state->owner || result.continuationToken != token_) { return; }
        auto *map = state->owner;
        const auto token = token_;
        const auto destination = destination_;
        postSceneCommand(map, [state = std::move(state), token, destination,
                               result](SceneCommandContext &) {
            if (state->owner) {
                state->owner->applyEventMenuContinuation(token, destination, result);
            }
        });
    }

private:
    std::weak_ptr<MapWithEvent::EventContinuationState> state_;
    std::uint64_t token_ = 0;
    std::int32_t destination_ = 0;
};

}

std::shared_ptr<EventInputContinuation>
MapWithEvent::createEventInputContinuation(
        std::uint64_t token, std::int32_t destination, bool writesMemory) {
    return std::make_shared<QueuedEventInputContinuation>(
        eventContinuationState_, token, destination, writesMemory);
}

std::shared_ptr<EventMenuContinuation>
MapWithEvent::createEventMenuContinuation(
        std::uint64_t token, std::int32_t destination) {
    return std::make_shared<QueuedEventMenuContinuation>(
        eventContinuationState_, token, destination);
}

void MapWithEvent::postEventOverlay(
        std::shared_ptr<const EventOverlayOperation> operation) {
    const auto sessionToken = eventSessionToken();
    if (!operation || sessionToken == 0) { return; }
    postSceneCommand(this, [sessionToken, operation = std::move(operation)](
                               SceneCommandContext &context) mutable {
        context.showEventOverlay({sessionToken, std::move(operation)});
    });
}

}
