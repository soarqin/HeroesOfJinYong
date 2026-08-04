#include "mapwithevent.hh"

#include "window.hh"
#include "world/savedata.hh"

#include <tuple>

namespace hojy::scene {
void MapWithEvent::setDirection(Map::Direction dir) {
    if (direction_ == dir) { return; }
    direction_ = dir;
    resetTime();
    currMainCharFrame_ = 0;
    updateMainCharTexture();
}

void MapWithEvent::setPosition(int x, int y, bool checkEvent) {
    if (x != currX_ || y != currY_) {
        miniPanelDirty_ = true;
    }
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    currMainCharFrame_ = 0;
    resting_ = false;
    drawDirty_ = true;
    bool r = tryMove(x, y, checkEvent);
    resetTime();
    if (r) {
        updateMainCharTexture();
    }
}

void MapWithEvent::move(Map::Direction direction) {
    int x, y;
    direction_ = direction;
    int oldX = currX_, oldY = currY_;
    if (!getFaceOffset(x, y) || !tryMove(x, y, true)) {
        return;
    }
    if (oldX != currX_ || oldY != currY_) {
        miniPanelDirty_ = true;
    }
    resetTime();
    updateMainCharTexture();
}

void MapWithEvent::update() {
    if (eventVm_.legacyActive() && !eventVm_.legacyWaiting()
        && !pendingSubEventWaiting_) {
        continueEvents(false);
    }
    if (checkTime()) {
        updateMainCharTexture();
    }
}

void MapWithEvent::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
        move(Map::DirUp);
        break;
    case KeyRight:
        move(Map::DirRight);
        break;
    case KeyLeft:
        move(Map::DirLeft);
        break;
    case KeyDown:
        move(Map::DirDown);
        break;
    case KeyCancel:
        gWindow->showMainMenu(subMapId_ >= 0);
        break;
    default:
        Map::handleKeyInput(key);
        break;
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
    if (subMapId_ < 0) {
        currEventItem_ = -1;
        return;
    }
    auto &layers = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto eventId = layers[3][y * mapWidth_ + x];
    if (eventId < 0) { return; }

    auto &events = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId_]->events;
    auto evt = events[eventId].event[type];
    if (evt <= 0) { return; }

    resetTime();
    currMainCharFrame_ = 0;
    updateMainCharTexture();

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

void MapWithEvent::renderChar(int deltaY) {
    if (!showChar_ || !mainCharTex_) { return; }
    int dx = currX_ - cameraX_;
    int dy = currY_ - cameraY_;
    int cellDiffY = cellHeight_ / 2;
    int offsetX = (dx - dy) * cellWidth_ / 2;
    int offsetY = (dx + dy) * cellDiffY;
    renderer_->renderTexture(mainCharTex_, x_ + (width_ >> 1) + offsetX * scale_.first / scale_.second,
                             y_ + (height_ >> 1) + (offsetY + cellDiffY - deltaY) * scale_.first /scale_.second, scale_);
}

void MapWithEvent::resetTime() {
    resting_ = false;
    nextMainTexTime_ = gWindow->currTime() + (currMainCharFrame_ > 0 ? 2 : 5) * 1000000ULL;
}

void MapWithEvent::frameUpdate() {
    if (extendedNode_) {
        extendedNode_->checkTimeout();
    }
    if (!moving_.empty()) {
        std::tie(cameraX_, cameraY_) = moving_.back();
        if (movingChar_) {
            if (tryMove(cameraX_, cameraY_, false)) {
                updateMainCharTexture();
            }
            if (currX_ != cameraX_ || currY_ != cameraY_) {
                direction_ = calcDirection(currX_, currY_, cameraX_, cameraY_);
                currX_ = cameraX_;
                currY_ = cameraY_;
            }
        }
        moving_.pop_back();
        drawDirty_ = true;
        if (moving_.empty()) {
            currMainCharFrame_ = 0;
            updateMainCharTexture();
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
        int step = animCurrTex_[i] < animEndTex_[i] ? 1 : -1;
        animCurrTex_[i] += step;
    }
    if (animEventId_[0] < 0) {
        updateMainCharTexture();
    } else {
        for (int i = 0; i < 3; ++i) {
            if (animCurrTex_[i] == 0) { continue; }
            auto &evt = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId_]->events[animEventId_[i]];
            evt.currTex = evt.begTex = evt.endTex = animCurrTex_[i];
            setCellTexture(evt.x, evt.y, 3, animCurrTex_[i] >> 1);
        }
    }
}

bool MapWithEvent::checkTime() {
    if (animEventId_[0] < 0) { return false; }
    auto now = gWindow->currTime();
    if (resting_) {
        if (now < nextMainTexTime_) {
            return false;
        }
        currMainCharFrame_ = (currMainCharFrame_ + 1) % 6;
        nextMainTexTime_ = now + 500 * 1000;
        return true;
    }
    if (now < nextMainTexTime_) {
        return false;
    }
    if (currMainCharFrame_ > 0) {
        currMainCharFrame_ = 0;
        nextMainTexTime_ = now + 5 * 1000000ULL;
    } else {
        currMainCharFrame_ = 0;
        resting_ = true;
        nextMainTexTime_ = now + 500 * 1000;
    }
    return true;
}

}
