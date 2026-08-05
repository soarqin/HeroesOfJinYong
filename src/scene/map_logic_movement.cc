#include "mapwithevent.hh"
#include "window_command.hh"

#include "content/constants.hh"
#include "world/savedata.hh"

#include <limits>
#include <tuple>

namespace hojy::scene {

namespace {

bool validEventStorage(std::int16_t subMapId) {
    if (subMapId < 0) { return false; }
    const auto index = static_cast<std::size_t>(subMapId);
    const auto &save = ::hojy::world::state::gSaveData;
    return index < save.subMapLayerInfo.size()
        && index < save.subMapEventInfo.size();
}

bool validMapCellIndex(int mapWidth, int mapHeight, int x, int y, std::size_t &index) {
    if (mapWidth <= 0 || mapHeight <= 0 || x < 0 || y < 0
        || x >= mapWidth || y >= mapHeight) {
        return false;
    }
    const auto width = static_cast<std::size_t>(mapWidth);
    const auto height = static_cast<std::size_t>(mapHeight);
    const auto cellCount = static_cast<std::size_t>(::hojy::content::SubMapWidth)
        * static_cast<std::size_t>(::hojy::content::SubMapHeight);
    if (width > cellCount || height > cellCount
        || static_cast<std::size_t>(y) >= height
        || static_cast<std::size_t>(x) >= width
        || static_cast<std::size_t>(y) > (std::numeric_limits<std::size_t>::max() -
                                          static_cast<std::size_t>(x)) / width) {
        return false;
    }
    index = static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x);
    return index < cellCount;
}

}

void MapWithEvent::setDirection(Map::Direction dir) {
    if (dir < Map::DirUp || dir > Map::DirDown) { return; }
    if (direction_ == dir) { return; }
    direction_ = dir;
    resetTime();
    currMainCharFrame_ = 0;
    updateMainCharSpriteId();
}

void MapWithEvent::setPosition(int x, int y, bool checkEvent) {
    if (!validMapCoordinate(x, y)) { return; }
    const auto oldX = currX_;
    const auto oldY = currY_;
    const auto oldCameraX = cameraX_;
    const auto oldCameraY = cameraY_;
    const auto oldFrame = currMainCharFrame_;
    const auto oldResting = resting_;
    const auto oldNextMainTexTime = nextMainTexTime_;
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    currMainCharFrame_ = 0;
    resting_ = false;
    const bool moved = tryMove(x, y, checkEvent);
    if (!moved) {
        currX_ = oldX;
        currY_ = oldY;
        cameraX_ = oldCameraX;
        cameraY_ = oldCameraY;
        currMainCharFrame_ = oldFrame;
        resting_ = oldResting;
        nextMainTexTime_ = oldNextMainTexTime;
        return;
    }
    markWorldChanged();
    if (oldX != currX_ || oldY != currY_) {
        markMiniPanelChanged();
    }
    resetTime();
    updateMainCharSpriteId();
}

void MapWithEvent::resetMainCharStance() {
    currMainCharFrame_ = 0;
    resting_ = false;
    resetTime();
    updateMainCharSpriteId();
    markWorldChanged();
}

void MapWithEvent::move(Map::Direction direction) {
    if (direction < Map::DirUp || direction > Map::DirDown
        || !validMapCoordinate(currX_, currY_)) {
        return;
    }
    int x, y;
    direction_ = direction;
    const int oldX = currX_;
    const int oldY = currY_;
    if (!getFaceOffset(x, y) || !tryMove(x, y, true)) {
        return;
    }
    if (oldX != currX_ || oldY != currY_) {
        markWorldChanged();
        markMiniPanelChanged();
    }
    resetTime();
    updateMainCharSpriteId();
}

void MapWithEvent::update() {
    if (eventVm_.legacyActive() && !eventVm_.legacyWaiting()
        && !pendingSubEventWaiting_) {
        continueEvents(false);
    }
    if (checkTime()) {
        updateMainCharSpriteId();
    }
}

void MapWithEvent::doInteract() {
    currEventItem_ = -1;
    int x, y;
    if (!getFaceOffset(x, y)) {
        return;
    }
    checkEvent(0, x, y);
}

void MapWithEvent::onMove() {
    currEventItem_ = -1;
    checkEvent(2, currX_, currY_);
}

void MapWithEvent::checkEvent(int type, int x, int y) {
    std::size_t cellIndex = 0;
    if (!validEventStorage(subMapId_) || !validMapCellIndex(mapWidth_, mapHeight_, x, y, cellIndex)
        || type < 0 || type >= 3
        || static_cast<std::size_t>(subMapId_) >=
            ::hojy::world::state::gSaveData.subMapLayerInfo.size()
        || static_cast<std::size_t>(subMapId_) >=
            ::hojy::world::state::gSaveData.subMapEventInfo.size()) {
        currEventItem_ = -1;
        return;
    }
    auto &layers = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId_]->data;
    const auto eventId = layers[3][cellIndex];
    if (eventId < 0 || eventId >= ::hojy::content::SubMapEventCount) {
        currEventItem_ = -1;
        return;
    }

    auto &events = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId_]->events;
    const auto evt = events[eventId].event[type];
    if (evt <= 0) {
        currEventItem_ = -1;
        return;
    }

    resetTime();
    currMainCharFrame_ = 0;
    updateMainCharSpriteId();
    currEventId_ = eventId;
    runEvent(evt);
}

bool MapWithEvent::getFaceOffset(int &x, int &y) {
    x = currX_;
    y = currY_;
    switch (direction_) {
    case DirUp:
        if (y > 0) { --y; return true; }
        break;
    case DirRight:
        if (x < mapWidth_ - 1) { ++x; return true; }
        break;
    case DirLeft:
        if (x > 0) { --x; return true; }
        break;
    case DirDown:
        if (y < mapHeight_ - 1) { ++y; return true; }
        break;
    }
    return false;
}

void MapWithEvent::resetTime() {
    resting_ = false;
    nextMainTexTime_ = phaseTime() + (currMainCharFrame_ > 0 ? 2 : 5) * 1000000ULL;
}

void MapWithEvent::frameUpdate() {
    if (eventTimeoutContinuationToken_ != 0
        && phaseTime() >= eventTimeoutDeadline_) {
        const auto token = eventTimeoutContinuationToken_;
        const auto session = eventSessionToken();
        eventTimeoutContinuationToken_ = 0;
        eventTimeoutDeadline_ = 0;
        auto state = eventContinuationState_;
        postSceneCommand(this, [state = std::move(state), session, token](
                                  SceneCommandContext &context) {
            context.clearEventPresentation({session});
            if (state && state->owner
                && state->owner->isCurrentEventSession(session)) {
                state->owner->applyEventInputContinuation(token, 0, false, 0);
            }
        });
    }
    if (!moving_.empty()) {
        std::tie(cameraX_, cameraY_) = moving_.back();
        if (movingChar_) {
            if (tryMove(cameraX_, cameraY_, false)) {
                updateMainCharSpriteId();
                if (currX_ != cameraX_ || currY_ != cameraY_) {
                    direction_ = calcDirection(currX_, currY_, cameraX_, cameraY_);
                    currX_ = cameraX_;
                    currY_ = cameraY_;
                }
            } else {
                moving_.clear();
                movingChar_ = false;
                currMainCharFrame_ = 0;
                updateMainCharSpriteId();
                continueEvents(false);
                return;
            }
        }
        moving_.pop_back();
        markWorldChanged();
        if (moving_.empty()) {
            currMainCharFrame_ = 0;
            updateMainCharSpriteId();
            continueEvents(false);
        }
    }
    if (animCurrTex_[0] == 0) { return; }
    if (animCurrTex_[0] == animEndTex_[0]) {
        for (int i = 0; i < 3; ++i) {
            animEventId_[i] = 0;
            animCurrTex_[i] = 0;
            animEndTex_[i] = 0;
        }
        continueEvents(false);
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (animCurrTex_[i] == 0 || animCurrTex_[i] == animEndTex_[i]) { continue; }
        const int step = animCurrTex_[i] < animEndTex_[i] ? 1 : -1;
        animCurrTex_[i] += step;
    }
    if (animEventId_[0] < 0 || !validEventStorage(subMapId_)) {
        updateMainCharSpriteId();
    } else {
        for (int i = 0; i < 3; ++i) {
            if (animCurrTex_[i] == 0 || animEventId_[i] < 0
                || animEventId_[i] >= ::hojy::content::SubMapEventCount) {
                continue;
            }
            auto &evt = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId_]
                ->events[animEventId_[i]];
            evt.currTex = evt.begTex = evt.endTex = animCurrTex_[i];
            setCellSpriteId(evt.x, evt.y, 3, animCurrTex_[i] >> 1);
        }
    }
}

bool MapWithEvent::checkTime() {
    if (animEventId_[0] < 0) { return false; }
    const auto now = phaseTime();
    if (resting_) {
        if (now < nextMainTexTime_) { return false; }
        currMainCharFrame_ = (currMainCharFrame_ + 1) % 6;
        nextMainTexTime_ = now + 500 * 1000ULL;
        return true;
    }
    if (now < nextMainTexTime_) { return false; }
    if (currMainCharFrame_ > 0) {
        currMainCharFrame_ = 0;
        nextMainTexTime_ = now + 5 * 1000000ULL;
    } else {
        currMainCharFrame_ = 0;
        resting_ = true;
        nextMainTexTime_ = now + 500 * 1000ULL;
    }
    return true;
}

}
