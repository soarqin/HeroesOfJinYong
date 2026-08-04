#include "warfield.hh"

#include "battle/combat_rules.hh"
#include "battle/turn_order.hh"
#include "effect.hh"
#include "window.hh"
#include "menu.hh"
#include "statusview.hh"
#include "world/action.hh"
#include "world/savedata.hh"
#include "world/strings.hh"
#include "core/config.hh"

#include <algorithm>
#include <fmt/xchar.h>
#include <tuple>

namespace hojy::scene {
void Warfield::frameUpdate() {
    switch (stage_) {
    case Idle:
        nextAction();
        break;
    case Moving: {
        if (movingPath_.empty() || !currentActor_) {
            movingPath_.clear();
            if (currentActor_ && battle::shouldContinueAfterMovement(
                    currentActor_->side == 0 && !autoControl_,
                    static_cast<bool>(pendingAutoAction_), resumeAutoAttack_)) {
                stage_ = Idle;
            } else if (currentActor_) {
                endTurn(currentActor_);
            } else {
                stage_ = Idle;
            }
            break;
        }
        int x, y;
        std::tie(x, y) = movingPath_.back();
        if (x == cameraX_ && y == cameraY_) {
            movingPath_.pop_back();
            if (movingPath_.empty()) {
                if (battle::shouldContinueAfterMovement(
                        currentActor_->side == 0 && !autoControl_,
                        static_cast<bool>(pendingAutoAction_), resumeAutoAttack_)) {
                    stage_ = Idle;
                } else {
                    endTurn(currentActor_);
                }
                break;
            }
            std::tie(x, y) = movingPath_.back();
        }
        movingPath_.pop_back();
        auto &ci = cellInfo_[cameraX_ + cameraY_ * mapWidth_];
        auto &newci = cellInfo_[x + y * mapWidth_];
        auto *charInfo = ci.charInfo;
        if (charInfo != currentActor_ || newci.charInfo) {
            movingPath_.clear();
            endTurn(currentActor_);
            break;
        }
        if (x < cameraX_) {
            charInfo->direction = DirLeft;
        } else if (x > cameraX_) {
            charInfo->direction = DirRight;
        } else if (y < cameraY_) {
            charInfo->direction = DirUp;
        } else if (y > cameraY_) {
            charInfo->direction = DirDown;
        }
        const battle::BattleCell from{
            static_cast<std::int16_t>(cameraX_),
            static_cast<std::int16_t>(cameraY_),
        };
        --charInfo->steps;
        newci.charInfo = charInfo;
        ci.charInfo = nullptr;
        charInfo->x = x;
        charInfo->y = y;
        cameraX_ = x;
        cameraY_ = y;
        drawDirty_ = true;
        if (const auto actor = participantIndex(charInfo)) {
            if (!recordBattleAction(battle::BattleAction{
                    *actor,
                    battle::MoveAction{
                        from,
                        battle::BattleCell{
                            static_cast<std::int16_t>(x),
                            static_cast<std::int16_t>(y),
                        },
                    },
                })) {
                break;
            }
        }
        if (movingPath_.empty()) {
            if (battle::shouldContinueAfterMovement(
                    currentActor_->side == 0 && !autoControl_,
                    static_cast<bool>(pendingAutoAction_), resumeAutoAttack_)) {
                stage_ = Idle;
            } else {
                endTurn(currentActor_);
            }
        }
        break;
    }
    case Acting: {
        if (!currentActor_) {
            stage_ = Idle;
            clearActionState(false);
            break;
        }
        fightTexIdx_ = std::min(fightTexIdx_ + 1, fightTexCount_ - 1);
        if (fightFrame_ == 0) {
            const ::hojy::world::state::SkillData *skillInfo;
            if (actId_ > 0 && (skillInfo = ::hojy::world::state::gSaveData.skillInfo[actId_]) != nullptr) {
                gWindow->playAtkSound(skillInfo->soundId);
            } else {
                gWindow->playAtkSound(0);
            }
        } else if (fightFrame_ == 3) {
            gWindow->playEffectSound(effectId_);
        }
        ++fightFrame_;
        if (++effectTexIdx_ >= int(gEffect[effectId_].size()) + 3) {
            auto *actor = currentActor_;
            auto postFunc = [this, actor]() {
                if (currentActor_ != actor) { return; }
                if (--attackTimesLeft_ > 0) {
                    auto *ch = actor;
                    const auto *skill = ::hojy::world::state::gSaveData.skillInfo[actId_];
                    if (!skill) {
                        actIndex_ = actId_ = -1;
                        actLevel_ = 0;
                        actItemSlot_ = -1;
                        skillLevelup_ = false;
                        effectId_ = -1;
                        effectTexIdx_ = -1;
                        fightTexIdx_ = -1;
                        fightTexCount_ = 0;
                        fightFrame_ = 0;
                        attackTimesLeft_ = 0;
                        fightTex_ = nullptr;
                        endTurn(actor);
                        return;
                    }
                    actLevel_ = battle::calcRepeatedSkillLevel(
                        skill->reqMp, actLevel_, ch->info.mp);
                    if (actLevel_ >= 0) {
                        startActAction();
                    } else {
                        actIndex_ = actId_ = -1;
                        actItemSlot_ = -1;
                    }
                } else {
                    actIndex_ = actId_ = -1;
                    actItemSlot_ = -1;
                }
                if (actIndex_ < 0) {
                    skillLevelup_ = false;
                    actLevel_ = 0;
                    effectId_ = -1;
                    effectTexIdx_ = -1;
                    fightTexIdx_ = -1;
                    fightTexCount_ = 0;
                    fightFrame_ = 0;
                    attackTimesLeft_ = 0;
                    fightTex_ = nullptr;
                    endTurn(actor);
                }
            };
            if (skillLevelup_) {
                skillLevelup_ = false;
                stage_ = PoppingUp;
                const auto *skill = ::hojy::world::state::gSaveData.skillInfo[actId_];
                auto *ch = actor;
                auto *msgBox = new MessageBox(this, 0, height_ / 3, width_, 60);
                msgBox->popup({fmt::format(GETTEXT(81), GETSKILLNAME(actId_),
                                           ch->info.skillLevel[actIndex_] / 100 + 1)}, MessageBox::PressToCloseThis);
                msgBox->setCloseHandler([this, actor, postFunc]() {
                    if (currentActor_ != actor) { return; }
                    stage_ = Acting;
                    postFunc();
                });
            } else {
                postFunc();
            }
        }
        drawDirty_ = true;
        break;
    }
    default:
        break;
    }
}

void Warfield::nextAction() {
    currentActor_ = nullptr;
    CharInfo *ch = nullptr;
    for (;;) {
        if (charQueue_.empty()) {
            if (round_ > 0) {
                for (auto &ci: chars_) {
                    const auto inactive = ci.x < 0 || ci.y < 0;
                    ::hojy::world::state::actRoundEndDrain(&ci.info, inactive);
                    if (const auto actor = participantIndex(&ci)) {
                        if (!recordBattleAction(battle::BattleAction{
                                *actor, battle::RoundEndAction{inactive},
                            })) {
                            return;
                        }
                    }
                }
                if (checkWarEnd()) { return; }
            }
            ++round_;
            charQueue_ = battle::buildRoundQueue(
                turnOrder_,
                [](const CharInfo *actor) { return actor->info.speed; },
                [](const CharInfo *actor) { return actor->info.hp > 0; });
            for (auto *actor: charQueue_) {
                actor->steps = battle::calculateMovementSteps(
                    actor->info.speed, actor->info.hurt);
                actor->initialSteps = actor->steps;
            }
            if (charQueue_.empty()) {
                checkWarEnd();
                return;
            }
        }
        ch = charQueue_.back();
        if (ch->info.hp <= 0) {
            charQueue_.pop_back();
            continue;
        }
        break;
    }
    currentActor_ = ch;
    battle::prepareActorActionCode(
        ch->actionCode,
        static_cast<bool>(pendingAutoAction_) || resumeAutoAttack_);
    cameraX_ = ch->x;
    cameraY_ = ch->y;
    drawDirty_ = true;
    auto *sv = dynamic_cast<StatusView*>(statusPanel_);
    if (sv) {
        auto windowBorder = core::config.windowBorder();
        sv->show(&ch->info, false, true);
        sv->forceUpdate();
        sv->setPosition(ch->side == 1 ? windowBorder * 4
                                      : (width_ - windowBorder * 4 - sv->width()),
                       height_ * 2 / 5 - sv->height() / 2);
    }
    if (ch->side == 1 || autoControl_) {
        autoAction();
    } else {
        lastMenuIndex_ = 0;
        playerMenu();
    }
}

void Warfield::doRest(CharInfo *expectedActor) {
    auto *ch = currentActor_;
    if (!ch || (expectedActor && expectedActor != ch)) { return; }
    if (ch->info.hp <= 0) {
        endTurn(ch);
        return;
    }
    const auto moved = battle::hasMoved(ch->initialSteps, ch->steps);
    ::hojy::world::state::actRest(&ch->info, moved, battleRandom_);
    if (const auto actor = participantIndex(ch)) {
        if (!recordBattleAction(battle::BattleAction{
                *actor, battle::RestAction{moved},
            })) {
            return;
        }
    }
    endTurn(ch);
}

void Warfield::endTurn(CharInfo *expectedActor) {
    auto *ch = currentActor_;
    if (!ch || (expectedActor && expectedActor != ch)) { return; }
    const auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
    if (ite != charQueue_.end()) {
        charQueue_.erase(ite);
    }
    currentActor_ = nullptr;
    pendingAutoAction_ = nullptr;
    movingPath_.clear();
    resumeAutoAttack_ = false;
    clearActionState(false);
    for (auto &ci: chars_) {
        if (!battle::shouldClearDeadPosition(ci.info.hp, ci.x, ci.y)) {
            continue;
        }
        if (ci.x < mapWidth_ && ci.y < mapHeight_) {
            auto &cell = cellInfo_[ci.x + ci.y * mapWidth_];
            if (cell.charInfo == &ci) { cell.charInfo = nullptr; }
        }
        ci.x = ci.y = -1;
        drawDirty_ = true;
    }
    if (checkWarEnd()) { return; }
    stage_ = Idle;
}

bool Warfield::checkWarEnd() {
    if (battleEngine_.status() == battle::EngineStatus::Active
        && battleParticipants_.size() == chars_.size()) {
        syncBattleParticipantsToWorking();
        battleEngine_.reconcile(battleInventorySnapshot());
        syncBattleParticipantsFromWorking();
        if (battleEngine_.status() == battle::EngineStatus::Finished) {
            won_ = battleEngine_.snapshot().won;
            endWar();
            return true;
        }
    }
    int aliveCount[2] = {0, 0};
    for (const auto &ci: chars_) {
        if (ci.info.hp > 0) { ++aliveCount[ci.side]; }
    }
    if (aliveCount[1] == 0) {
        won_ = true;
        endWar();
        return true;
    }
    if (aliveCount[0] == 0) {
        won_ = false;
        endWar();
        return true;
    }
    return false;
}

}
