#include "warfield.hh"

#include "battle/movement.hh"
#include "content/constants.hh"

#include <map>

namespace hojy::scene {

void Warfield::recalcKnowledge() {
    knowledge_[0] = knowledge_[1] = 0;
    for (auto &ci: chars_) {
        if (ci.info.hp > 0 && ci.info.knowledge > ::hojy::content::KnowledgeBarrier) {
            knowledge_[ci.side] += ci.info.knowledge;
        }
    }
}

void Warfield::maskSelectableArea(int steps, int ranges, bool zoecheck) {
    auto *ch = currentActor_;
    if (!ch) { setStage(Idle); return; }
    getSelectableArea(ch, selCells_, steps, ranges, zoecheck);
    int w = mapWidth_;
    for (auto &c: selCells_) {
        auto &ci = cellInfo_[c.first.first + c.first.second * w];
        ci.insideMovingArea = true;
    }
    cursorX_ = ch->x;
    cursorY_ = ch->y;
}

void Warfield::unmaskArea() {
    int w = mapWidth_;
    for (auto c: selCells_) {
        cellInfo_[c.first.first + c.first.second * w].insideMovingArea = false;
    }
    selCells_.clear();
}

void Warfield::getSelectableArea(
        CharInfo *ch,
        std::map<std::pair<int, int>, SelectableCell> &selCells,
        int steps,
        int ranges,
        bool zoecheck) {
    battle::getSelectableArea(
        mapWidth_, mapHeight_, {ch->x, ch->y}, steps, ranges, selCells,
        [this](int x, int y) { return cellInfo_[y * mapWidth_ + x].blocked; },
        [this](int x, int y) { return cellInfo_[y * mapWidth_ + x].charInfo != nullptr; },
        [this, ch, zoecheck](int x, int y) {
            if (!zoecheck) { return false; }
            const auto *other = cellInfo_[y * mapWidth_ + x].charInfo;
            return other != nullptr && other->side == ch->side;
        });
}

}
