#include "warfield.hh"

#include "battle/combat_rules.hh"
#include "battle/movement.hh"
#include "charlistmenu.hh"
#include "itemview.hh"
#include "menu.hh"
#include "statusview.hh"
#include "window.hh"
#include "world/action.hh"
#include "world/savedata.hh"
#include "world/strings.hh"
#include "core/config.hh"
#include "content/constants.hh"

#include <algorithm>
#include <map>
#include <vector>

namespace hojy::scene {
void Warfield::recalcKnowledge() {
    knowledge_[0] = knowledge_[1] = 0;
    for (auto &ci: chars_) {
        if (ci.info.hp > 0 && ci.info.knowledge > ::hojy::content::KnowledgeBarrier) {
            knowledge_[ci.side] += ci.info.knowledge;
        }
    }
}

void Warfield::playerMenu() {
    stage_ = PlayerMenu;
    auto windowBorder = core::config.windowBorder();
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    auto *menu = new MenuTextList(this, windowBorder * 4, windowBorder * 4, width_ - windowBorder * 8, height_ - windowBorder * 8);
    std::vector<std::wstring> n;
    std::vector<int> menuIndices;
    n.reserve(10);
    menuIndices.reserve(10);
    auto &info = ch->info;
    if (ch->steps > 0 && info.stamina > 5) {
        n.emplace_back(GETTEXT(82)); menuIndices.emplace_back(0);
    }
    if (info.stamina > 10) {
        n.emplace_back(GETTEXT(83)); menuIndices.emplace_back(1);
        if (info.poison >= 20) {
            n.emplace_back(GETTEXT(84)); menuIndices.emplace_back(2);
        }
    }
    if (info.stamina > 50) {
        if (info.depoison >= 20) {
            n.emplace_back(GETTEXT(85));
            menuIndices.emplace_back(3);
        }
        if (info.medic >= 20) {
            n.emplace_back(GETTEXT(86));
            menuIndices.emplace_back(4);
        }
    }
    n.emplace_back(GETTEXT(87)); menuIndices.emplace_back(5);
    if (charQueue_.size() > 1) {
        n.emplace_back(GETTEXT(88)); menuIndices.emplace_back(6);
    }
    n.emplace_back(GETTEXT(89)); menuIndices.emplace_back(7);
    n.emplace_back(GETTEXT(90)); menuIndices.emplace_back(8);
    n.emplace_back(GETTEXT(91)); menuIndices.emplace_back(9);
    menu->popup(n, lastMenuIndex_);
    menu->setHandler([this, menu, menuIndices, ch]() {
        auto index = menu->currIndex();
        if (index < 0 || index >= menuIndices.size()) { return; }
        lastMenuIndex_ = index;
        switch (menuIndices[index]) {
        case 0:
            maskSelectableArea(ch->steps, 0);
            stage_ = MoveSelecting;
            drawDirty_ = true;
            break;
        case 1:
            if (ch->info.skillId[1] > 0) {
                std::vector<std::wstring> items;
                std::vector<int> indices;
                for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
                    auto skillId = ch->info.skillId[i];
                    if (skillId <= 0) { continue; }
                    const auto *skillInfo = ::hojy::world::state::gSaveData.skillInfo[skillId];
                    if (!skillInfo) { continue; }
                    auto skillLevel =
                        ::hojy::world::state::calcRealSkillLevel(skillInfo->reqMp,
                                                std::clamp<std::int16_t>(ch->info.skillLevel[i] / 100, 0, 9),
                                                ch->info.mp);
                    if (skillLevel < 0) { continue; }
                    indices.emplace_back(i);
                    items.emplace_back(GETSKILLNAME(skillId));
                }
                if (!items.empty()) {
                    auto windowBorder = core::config.windowBorder();
                    auto *submenu = new MenuTextList(menu, menu->x() + menu->width() + windowBorder, windowBorder * 4,
                                                     width_ - menu->x() + menu->width() - windowBorder, height_ - windowBorder * 8);
                    submenu->popup(items);
                    submenu->setHandler([this, menu, submenu, indices]() {
                        if (tryUseSkill(indices[submenu->currIndex()])) {
                            menu->requestDelete();
                        } else {
                            submenu->requestDelete();
                        }
                    });
                    return;
                }
            } else {
                if (tryUseSkill(0)) {
                    menu->requestDelete();
                    return;
                }
            }
            {
                auto *msgBox = new MessageBox(this, 0, height_ / 3, width_, 60);
                msgBox->popup({GETTEXT(115)}, MessageBox::PressToCloseThis);
            }
            return;
        case 2:
            if (tryUseSkill(-3)) {
                menu->requestDelete();
            }
            return;
        case 3:
            if (tryUseSkill(-2)) {
                menu->requestDelete();
            }
            return;
        case 4:
            if (tryUseSkill(-1)) {
                menu->requestDelete();
            }
            return;
        case 5: {
            auto windowBorder = core::config.windowBorder();
            auto *iv = new ItemView(this, windowBorder * 4, windowBorder * 4, gWindow->width() - windowBorder * 4, gWindow->height() - windowBorder * 4);
            iv->setCharInfo(&ch->info);
            iv->show(true, [this, ch](std::int16_t itemId) {
                if (currentActor_ != ch) { return; }
                if (itemId < 0) {
                    endTurn(ch);
                } else {
                    actIndex_ = itemId;
                    actId_ = -4;
                    actLevel_ = 0;
                    actItemSlot_ = -1;
                    attackTimesLeft_ = 1;
                    maskSelectableArea(0, battle::calcTechniqueRange(ch->info.throwing));
                    stage_ = AttackSelecting;
                    drawDirty_ = true;
                }
            });
            iv->setCloseHandler([this, ch]() {
                if (currentActor_ == ch) { playerMenu(); }
            });
            menu->requestDelete();
            return;
        }
        case 6:
            if (currentActor_ != ch) { return; }
            if (const auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
                ite != charQueue_.end()) {
                charQueue_.erase(ite);
            }
            charQueue_.insert(charQueue_.begin(), ch);
            currentActor_ = nullptr;
            pendingAutoAction_ = nullptr;
            stage_ = Idle;
            break;
        case 7: {
            std::vector<std::int16_t> idlist;
            for (auto &c: chars_) {
                idlist.emplace_back(c.side == 1 ? -c.id : c.id);
            }
            auto *svmenu = new CharListMenu(this, 0, 0, gWindow->width(), gWindow->height());
            svmenu->init({GETTEXT(59)}, idlist, {CharListMenu::LEVEL},
                         [this](std::int16_t charId) {
                             auto *sv = new StatusView(this, 0, 0, 0, 0);
                             bool found = false;
                             for (auto &p: chars_) {
                                 if (p.id == charId && p.side == 0) {
                                     sv->show(&p.info, false);
                                     found = true;
                                     break;
                                 }
                             }
                             if (!found) {
                                 sv->show(charId);
                             }
                             sv->makeCenter(width_, height_, x_, y_);
                         }, nullptr);
            svmenu->makeCenter(width_, height_ * 4 / 5, x_, y_);
            return;
        }
        case 8:
            doRest();
            break;
        case 9:
            autoControl_ = true;
            stage_ = Idle;
            break;
        default:
            return;
        }
        menu->requestDelete();
    }, []()->bool {
        return false;
    });
}

void Warfield::maskSelectableArea(int steps, int ranges, bool zoecheck) {
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
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

void Warfield::getSelectableArea(CharInfo *ch, std::map<std::pair<int, int>, SelectableCell> &selCells, int steps, int ranges, bool zoecheck) {
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
