#include "mapwithevent.hh"

namespace hojy::scene {

void MapWithEvent::applyInputLogic() {
    if (pendingInputKey_) {
        const auto key = *pendingInputKey_;
        pendingInputKey_.reset();
        if (mapInputMode_ && !currEventPaused_) {
            pendingInputAction_ = mapInputMode_->translate(key);
        }
    }
    auto action = std::move(pendingInputAction_);
    if (!action || currEventPaused_) { return; }
    action->execute(*this);
}

void MapWithEvent::requestMove(InputKey key) {
    static constexpr int directions[] = {
        -1, Map::DirUp, Map::DirDown, Map::DirLeft, Map::DirRight,
        -1, -1, -1, -1,
    };
    const auto index = static_cast<std::size_t>(key);
    if (index >= std::size(directions) || directions[index] < 0) { return; }
    move(static_cast<Map::Direction>(directions[index]));
}

void MapWithEvent::requestInteract() {
    doInteract();
}

void MapWithEvent::requestOpenMenu() {
    postCommand([inSubMap = subMapId_ >= 0](SceneCommandContext &context) {
        context.showMainMenu(inSubMap);
    });
}

}
